#include "MicDriver.h"
#include "arduinoFFT.h"

#define PIN_CLK 0
#define PIN_DATA 34

bool InitI2SMicroPhone()
{
    esp_err_t err = ESP_OK;
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_PDM),
        // .sample_rate = 44100,
        .sample_rate = 48000,
        .bits_per_sample =
            I2S_BITS_PER_SAMPLE_16BIT, // is fixed at 12bit, stereo, MSB
        .channel_format = I2S_CHANNEL_FMT_ALL_RIGHT,
#if ESP_IDF_VERSION > ESP_IDF_VERSION_VAL(4, 1, 0)
        .communication_format =
            I2S_COMM_FORMAT_STAND_I2S, // Set the format of the communication.
#else                                      // 设置通讯格式
        .communication_format = I2S_COMM_FORMAT_I2S,
#endif
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 2,
        .dma_buf_len = 128,
    };

    i2s_pin_config_t pin_config;
#if (ESP_IDF_VERSION > ESP_IDF_VERSION_VAL(4, 3, 0))
    pin_config.mck_io_num = I2S_PIN_NO_CHANGE;
#endif
    pin_config.bck_io_num = I2S_PIN_NO_CHANGE;
    pin_config.ws_io_num = PIN_CLK;
    pin_config.data_out_num = I2S_PIN_NO_CHANGE;
    pin_config.data_in_num = PIN_DATA;

    err += i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
    err += i2s_set_pin(I2S_NUM_0, &pin_config);
    // err += i2s_set_clk(I2S_NUM_0, 44100, I2S_BITS_PER_SAMPLE_16BIT,
    //                 I2S_CHANNEL_MONO);
    err += i2s_set_clk(I2S_NUM_0, 48000, I2S_BITS_PER_SAMPLE_16BIT,
                        I2S_CHANNEL_MONO);
    // i2s_set_clk(0)

    if (err != ESP_OK)
    {
        return false;
    }
    else
    {
        return true;
    }
}

void MicDriver::mic_init()
{
    printf("mic init %s\n", InitI2SMicroPhone() ? "ok" : "failed");
}
    
void MicDriver::mic_get_raw(uint8_t *p_buf, uint32_t sz)
{
    size_t bytes_read = 0;
    
    /* Read mic */
    i2s_read(I2S_NUM_0, (void *)p_buf, sz, &bytes_read, (100 / portTICK_RATE_MS));
    printf("i2s_read bytes: %d\n",bytes_read);
}

void MicDriver::mic_get_fft(uint8_t *p_buf, uint32_t sz)
{
    arduinoFFT _FFT;

    /*
    These values can be changed in order to evaluate the functions
    */
    const uint16_t _samples = 256; // This value MUST ALWAYS be a power of 2
    const double _samplingFrequency = 48000;

    /*
    These are the input and output vectors
    Input vectors receive computed results from FFT
    */
    double *_vReal;
    double *_vReal_old;
    double *_vImag;
    int16_t *_rawData;

    /* Alloc buffer */
    _vReal = new double[_samples];
    _vReal_old = new double[_samples]();
    _vImag = new double[_samples];
    _rawData = new int16_t[_samples];

    size_t bytes_read = 0;
    while (1)
    {

        /* Read mic */
        i2s_read(I2S_NUM_0, (void *)_rawData, (sizeof(int16_t) * _samples), &bytes_read, (100 / portTICK_RATE_MS));

        /* Copy data */
        for (int i = 0; i < _samples; i++)
        {
            _vReal[i] = (double)_rawData[i];
            _vImag[i] = 0.0;
        }

        /* FFT */
        _FFT = arduinoFFT(_vReal, _vImag, _samples, _samplingFrequency);
        _FFT.Windowing(FFT_WIN_TYP_HAMMING, FFT_FORWARD);
        _FFT.Compute(FFT_FORWARD);
        _FFT.ComplexToMagnitude();
        printf("fft done\n");

    }

    /* Free buffer */
    printf("free mic buffer\n");
    // delete [] mic_buffer;
    delete[] _rawData;
    delete[] _vReal;
    delete[] _vImag;
    delete[] _vReal_old;
}

