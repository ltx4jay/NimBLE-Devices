//
// NimBLE-Device convenience base class for shutter buttons
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


namespace Nimble {

namespace ShutterButton {

//
// (Virtual) Base class for shutter buttons
//
class Device : public NimBLE::InterestingDevice
{
public:
    //
    // A shutter button device with the specified unique name, BLE device name, and optional MAC address
    // If no address is specified, the first one found will match
    //
    Device(const char* uniqueName, const char* bleName, const char* macAddr);

    virtual ~Device();

    //
    // Set the maximum button press time for a click
    // Anything longer is considered a long press and not a click.
    // Default is 0.5 secs
    //
    void setMaximumClickTime(unsigned int msecs);

    //
    // Returns the number of times the button was clicked since we last checked
    // Note: A click is a press + release event
    //
    unsigned int getNumClicks();
    unsigned int getNumClicks(bool &lastOneWasLongPress);

    //
    // Check if the button is currently pressed.
    // Returns how long (in msecs) it has been pressed or 0 if it is not currently pressed.
    //
    unsigned long isPressed();

    //
    // Subscribe to the button event (optional)
    // The argument is true if the button was pressed, false if the button was released
    //
    void subscribeButton(std::function<void(bool, void*)> fct, void* user);

protected:
    //
    // Report a button event
    //
    void buttonEvent(bool isPressed);
};

}
}

