//
// NimBLE-Device convenience API for an QB702 counter ring
// https://www.aliexpress.us/item/2251832787319182.html
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


#include "Keyboard.hh"


namespace NimBLE {

namespace QB702 {

class Device : public NimBLE::Keyboard::Device
{
public:
    //
    // A QB702 counter ring device with the specified unique name and optional MAC address
    // If no address is specified, the first one found will match
    //
    Device(const char* uniqueName, const char* macAddr);

    virtual ~Device();

    bool doInitDevice()             override;
    void serviceLoop(long nowInMs)  override;

private:
    void notifyButton(NimBLERemoteCharacteristic* pRemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify);
};

}
}

