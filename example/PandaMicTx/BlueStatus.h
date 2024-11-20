#ifndef _BLUE_STATUS_H
#define _BLUE_STATUS_H

class BlueStatus
{
public:
  enum ConnectionState
  {
    DISCONNECTED,
    CONNECTING,
    CONNECTED
  };

  enum MediaState
  {
    INACTIVE,
    ACTIVE
  };

private:
  static BlueStatus *BStatus;
public:
  bool avrcConnectionState = false;
  ConnectionState connectionState = ConnectionState::DISCONNECTED;
  MediaState mediaState;

  BlueStatus()
  {
  }

  ~BlueStatus()
  {
    if (this == BlueStatus::BStatus)
    {
      this->stop();
    }
  }

  bool start()
  {
    connectionState = ConnectionState::CONNECTING;
    connectionState = ConnectionState::CONNECTED;
    
    mediaState = MediaState::ACTIVE;
    return true;
  }

  void stop()
  {
    if (connectionState == ConnectionState::CONNECTED)
    {
      connectionState=ConnectionState::DISCONNECTED;
      mediaState = MediaState::INACTIVE;
    }
  }

  void pause()
  {
    connectionState = ConnectionState::CONNECTING;
  }

  void resume()
  {
    connectionState = ConnectionState::CONNECTED;
  }

};


BlueStatus *BlueStatus::BStatus;

#endif