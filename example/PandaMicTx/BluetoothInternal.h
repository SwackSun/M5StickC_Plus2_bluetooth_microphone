#ifndef _BLUETOOTH_INTERNAL_H
#define _BLUETOOTH_INTERNAL_H

#include <functional>

typedef std::function<int32_t(uint8_t *data, int32_t len)> data_callback_t;

#endif
