//
// NimBLE-Device convenience base class for character input device (keyboard, mouse, button)
// 
// Copyright (c) 2023 ltx4jay@yahoo.com
//
// Licensed under MIT License
//
//
// The code is provided as-is, with no warranties of any kind. Not suitable for any purpose.
// Provided as an example and exercise in BLE development only.
//

#pragma once


#include "NimBLE-Device.hh"

namespace NimBLE {

namespace Keyboard {

//
// (Virtual) Base class for button devices
//
class Device : public NimBLE::InterestingDevice
{
public:
    //
    // A button device with the specified unique name, BLE device name, and optional MAC address
    // If no address is specified, the first one found will match
    //
    Device(const char* uniqueName, const char* bleName, const char* macAddr);

    virtual ~Device();

    //
    // Optionally remap the physical device keys
    //
    virtual void setKeyMap(const uint8_t map[]);       // Remap all the keys (e.g. map[0x20] = 0x32 remaps device key 0x20 to Keyboard key 0x32)
    virtual void setKeyMap(uint8_t from, uint8_t to);  // Remap a single key

    //
    // Check if the specified key/button is currently pressed.
    //
    bool isPressed(uint8_t key);

    //
    // Types of key/button events
    //
    typedef enum {PRESSED, RELEASED} Event_t;

    //
    // Subscribe to key/button events (optional)
    //
    void subscribe(std::function<void(uint8_t key, Event_t e)> fct);

protected:
    //
    // Report a key/button event
    //
    void publish(uint8_t key, Event_t e);

private:
    uint8_t      mKeyMap[256];
    bool         mPressed[256];

    struct Listener {
        std::function<void(uint8_t key, Event_t e)> fct;
    };
    std::vector<Listener> mListeners;
};

}
}

