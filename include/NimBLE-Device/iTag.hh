//
// NimBLE-Device convenience class for iTAG device
// 
// Copyright (c) 2024 ltx4jay@yahoo.com
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

namespace iTag {


class Device : public NimBLE::InterestingDevice
{
public:
    //
    // An iTag device with the specified unique name, BLE device name, and MAC address
    //
    Device(const char* uniqueName, const char* macAddr, uint8_t addrType = 0);

    virtual ~Device();

    //
    // Alarm state
    //
    typedef enum {OFF, ON, HIGH} AlarmSetting_t;

    //
    // Control the alarm
    //
    void setAlarm(AlarmSetting_t level);

private:
    bool doInitDevice()             override;
    void serviceLoop(long nowInMs)  override;

    void notifyBattery(NimBLERemoteCharacteristic* pRemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify);

    NimBLERemoteCharacteristic* mAlarmChr;
};

}
}

