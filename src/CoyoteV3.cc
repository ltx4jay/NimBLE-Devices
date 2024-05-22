//
// Implementation of NimBLE-Device convenience API for a Dungeon Labs Coyote V3.0
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

#include "NimBLE-Device/Coyote.hh"


using namespace NimBLE::COYOTE;

NimBLE::COYOTE::Device::V3::V3(const char* uniqueName, const char* macAddr)
: Device(uniqueName, "47L121000", macAddr)
{
}


bool
NimBLE::COYOTE::Device::V2::initDevice()
{
    return false;
}

void
NimBLE::COYOTE::Device::V3::setMaxPower(uint8_t A, uint8_t B)
{

}


void
NimBLE::COYOTE::Device::V3::run()
{
}


class V3Channel : public Channel
{
public:
    V3Channel(Device* parent, const char* name);

    struct Playing
    {
        SemaphoreHandle_t            mutex;
        bool                         start;
        bool                         run;
        V2::Waveform                 wave;
        V2::Waveform::const_iterator iter;
        unsigned int                 nLeft;
    } mPlaying;

    virtual void setWaveform(const V2::Waveform& wave, uint8_t power = 0) override;
    virtual void start(long secs = 0) override;
    virtual void stop() override;

    bool powerUpdateReq(uint8_t& pow) override;

    void startNewWaveform();
    void sendNextSegment();
};


void
V3Channel::setWaveform(const V2::Waveform& wave, uint8_t power)
{
}

void
V3Channel::start(long secs)
{
}

void
V3Channel::stop()
{
}

void
V3Channel::startNewWaveform()
{
}

void
V3Channel::sendNextSegment()
{
}