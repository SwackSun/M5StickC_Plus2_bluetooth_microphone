#ifndef _ENUM_STRINGS_H
#define _ENUM_STRINGS_H

#include <string>
#include <BlueStatus.h>

std::string enumToString(BlueStatus::ConnectionState e)
{
  switch (e)
  {
  case BlueStatus::ConnectionState::DISCONNECTED:
    return "Disconnected";
  case BlueStatus::ConnectionState::CONNECTING:
    return "Connecting";
  case BlueStatus::ConnectionState::CONNECTED:
    return "Connected";
  }
}

std::string enumToString(BlueStatus::MediaState e)
{
  switch (e)
  {
  case BlueStatus::MediaState::INACTIVE:
    return "Inactive";
  case BlueStatus::MediaState::ACTIVE:
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
