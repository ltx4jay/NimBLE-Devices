//
// NimBLE-Device convenience API for a Dungeon Labs Coyote V2.0
// https://dungeon-lab.com/home.php
//
// WARNING: USE AT YOUR OWN RISK
//
// 
// Copyright (c) 2023-2024 ltx4jay@yahoo.com
//
// Licensed under MIT License
//
// The code is provided as-is, with no warranties of any kind. Not suitable for any purpose.
// Provided as an example and exercise in BLE development only.
//

#pragma once


#include "NimBLE-Device/Coyote.hh"

#include <freertos/FreeRTOS.h>
#include <string>

namespace NimBLE {

namespace COYOTE {

//
// A Coyote V2.0 device
//
class Device::V2 : public Device::Vx
{
public:
    V2(Device* parent);

    struct {
        uint16_t                      step;
        uint16_t                      max;
        NimBLERemoteCharacteristic   *charac;
    } mPower;

    void setMaxPower(uint8_t A, uint8_t B) override;
    void run() override;

private:
    void notifyBattery(NimBLERemoteCharacteristic* pRemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify);
    void notifyPower(NimBLERemoteCharacteristic* pRemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify);

    class Channel : public COYOTE::Channel
    {
        Channel(Device* parent, const char* name, NimBLERemoteCharacteristic* charac);

        NimBLERemoteCharacteristic* mChar;

        struct Playing
        {
            SemaphoreHandle_t                    mutex;
            bool                                 start;
            bool                                 run;
            COYOTE::V2::Waveform                 wave;
            COYOTE::V2::Waveform::const_iterator iter;
            unsigned int                         nLeft;
        } mPlaying;

        virtual void setWaveform(const COYOTE::V2::Waveform& wave, uint8_t power = 0) override;
        virtual void start(long secs = 0) override;
        virtual void stop() override;

        bool powerUpdateReq(uint8_t &pow) override;

        void startNewWaveform();
        void sendNextSegment();

        friend class Device;
    };
};


}
}