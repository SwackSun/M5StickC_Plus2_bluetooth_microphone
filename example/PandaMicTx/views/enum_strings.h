#ifndef _ENUM_STRINGS_H
#define _ENUM_STRINGS_H

#include <string>
#include "HFSession.h"

std::string enumToString(HFSession::ConnectionState e)
{
  switch (e)
  {
  case HFSession::ConnectionState::DISCONNECTED:
    return "Disconnected";
  case HFSession::ConnectionState::CONNECTING:
    return "Connecting";
  case HFSession::ConnectionState::CONNECTED:
    return "Connected";
  }
}

std::string enumToString(HFSession::MediaState e)
{
  switch (e)
  {
  case HFSession::MediaState::INACTIVE:
    return "Inactive";
  case HFSession::MediaState::ACTIVE:
    return "Active";
  }
}

// enum ConnectionState
// {
//   DISCONNECTED,
//   CONNECTING,
//   CONNECTED
// };

// enum MediaState
// {
//   INACTIVE,
//   ACTIVE
// };

#endif
