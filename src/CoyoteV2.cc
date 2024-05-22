//
// Implementation of NimBLE-Device convenience API for a Dungeon Labs Coyote V2.0
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
// Provided as an example and exercise in BLE mParentelopment only.
//

#include "NimBLE-Device/Coyote.hh"
#include "CoyoteV2.hh"


using namespace NimBLE::COYOTE;

NimBLE::COYOTE::Device::V2::V2(Device* parent)
: Vx(parent)
{
    auto pSvc = parent->mClient->getService("955A180A-0FE2-F5AA-A094-84B8D4F3E8AD");

    auto firmware = pSvc->getCharacteristic("955A1501-0FE2-F5AA-A094-84B8D4F3E8AD");
    uint16_t fw = * ((uint16_t*) firmware->readValue().data());
    ESP_LOGI(parent->getName(), "Firmware %02x.%02x", fw & 0x00FF, fw >> 8);

    auto battery  = pSvc->getCharacteristic("955A1500-0FE2-F5AA-A094-84B8D4F3E8AD");
    if (battery != nullptr) battery->subscribe(true, std::bind(&Device::V2::notifyBattery, this,
                                                               std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
    
    pSvc = parent->mClient->getService("955A180B-0FE2-F5AA-A094-84B8D4F3E8AD");
    if (pSvc == NULL) {
        ESP_LOGE(parent->getName(), "Cannot find power service.\n");
        parent->notifyEvent(ERROR);
        return;
    }
  
    auto cfgChar      = pSvc->getCharacteristic("955A1507-0FE2-F5AA-A094-84B8D4F3E8AD");
    mPower.charac   = pSvc->getCharacteristic("955A1504-0FE2-F5AA-A094-84B8D4F3E8AD");
    if (cfgChar == nullptr || mPower.charac == nullptr) {
        ESP_LOGE(parent->getName(), "Cannot find all power generator characteristics.\n");
        parent->notifyEvent(ERROR);
        return;
    }
    parent->mChannel[0] = new Device::V2::Channel(parent, "A", pSvc->getCharacteristic("955A1506-0FE2-F5AA-A094-84B8D4F3E8AD"));
    parent->mChannel[1] = new Device::V2::Channel(parent, "B", pSvc->getCharacteristic("955A1505-0FE2-F5AA-A094-84B8D4F3E8AD"));

    struct CFGval {
        uint32_t   step    :  8;
        uint32_t   maxPwr  : 11;
        uint32_t   rsvd    : 13;
    };
    
    auto cfg = *((CFGval*) cfgChar->readValue().data());
    mPower.step = cfg.step;
    mPower.max  = cfg.maxPwr / cfg.step;
    ESP_LOGI("ESTIM", "Power = %d / %d -> %d", cfg.maxPwr, cfg.step, mPower.max);
    mPower.charac->subscribe(true, std::bind(&Device::V2::notifyPower, this,
                                             std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_3));
}


NimBLE::COYOTE::Device::V2::Channel::Channel(Device *parent, const char* name, NimBLERemoteCharacteristic *charac)
    : COYOTE::Channel(parent, name)
    , mChar(charac)
    , mPlaying{xSemaphoreCreateRecursiveMutex(), false, false}
{
}

void
NimBLE::COYOTE::Device::V2::notifyBattery(NimBLERemoteCharacteristic* pRemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify)
{
    uint8_t battery = pData[0];
    ESP_LOGI(mParent->getName(), "Battery Level = %d%%", battery);

    if (mParent->mBatteryCb) mParent->mBatteryCb(battery);
}


struct PowerVal {
    uint32_t  B    : 11;
    uint32_t  A    : 11;
    uint32_t  rsvd : 10;

    PowerVal(uint16_t a = 0, uint16_t b = 0)
        : B(b)
        , A(a)
        , rsvd(0)
        {}

    // Values are sent in little-endian order
    operator uint8_t*() const
        {
            return (uint8_t*) this;
        }
};


void NimBLE::COYOTE::Device::V2::notifyPower(NimBLERemoteCharacteristic* pRemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify)
{
    auto pow = (PowerVal*) pData;

    mParent->getChannelA().updatePower(pow->A/mPower.step);
    mParent->getChannelB().updatePower(pow->B/mPower.step);

    ESP_LOGI(mParent->getName(), "Power Setting  A:%3d -> %d   B:%3d -> %d",
             mParent->getChannelA().mPower, mParent->getChannelA().mSetPower,
             mParent->getChannelB().mPower, mParent->getChannelB().mSetPower);
}


NimBLE::COYOTE::V2::WaveVal::WaveVal(uint8_t X, uint16_t Y, uint8_t Z, uint16_t rpt)
    : x(X)
    , y(Y)
    , z(Z)
    , rsvd(0)
    , repeat(rpt)
{
}


NimBLE::COYOTE::V2::WaveVal::WaveVal(const std::vector<uint8_t> vals, uint16_t rpt)
    : x(0)
    , y(0)
    , z(0)
    , rsvd(0)
    , repeat(rpt)
{
    auto b = (uint8_t*) this;
    for (auto it : vals) *b++ = it;
}


unsigned int
NimBLE::COYOTE::V2::WaveVal::duration() const
{
    unsigned int d = x + y;

    // Minimum duration is 10ms
    if (d < 10) d = 10;
    
    return d;
}


NimBLE::COYOTE::V2::WaveVal::operator uint8_t*() const
{
    return (uint8_t*) this;
}


void
NimBLE::COYOTE::Device::V2::Channel::setWaveform(const COYOTE::V2::Waveform& wave, uint8_t power)
{
    SemLockGuard lk(mPlaying.mutex);
    
    mPlaying.wave  = wave;
    mPlaying.iter  = mPlaying.wave.end();
    mPlaying.nLeft = 0;
    mPlaying.start = true;

    setPower(power);
}


void
NimBLE::COYOTE::Device::V2::Channel::start(long secs)
{
    NimBLE::COYOTE::Channel::start(secs);

    if (mPlaying.wave.size() == 0 || mPlaying.run) return;

    SemLockGuard lk(mPlaying.mutex);

    mPlaying.iter  = mPlaying.wave.end();
    mPlaying.nLeft = 0;
    mPlaying.start = true;
    mPlaying.run   = true;
}

void
NimBLE::COYOTE::Device::V2::Channel::stop()
{
    NimBLE::COYOTE::Channel::stop();

    SemLockGuard lk(mPlaying.mutex);
    
    mPlaying.start = false;
    mPlaying.run   = false;
}


void
NimBLE::COYOTE::Device::V2::Channel::startNewWaveform()
{
    SemLockGuard lk(mPlaying.mutex);

    if (!mPlaying.start) return;

    sendNextSegment();
    mPlaying.start = false;
}


void
NimBLE::COYOTE::Device::V2::Channel::sendNextSegment()
{
    if (mPlaying.nLeft == 0) {
        if (mPlaying.iter == mPlaying.wave.end()) {
            mPlaying.iter = mPlaying.wave.begin();
        }

        mPlaying.nLeft = (((mPlaying.iter->duration() - 1) / 100) + 1) * mPlaying.iter->repeat;
    }
    if (mPlaying.nLeft == 0) return;
    
    // ESP_LOGI("ESTIM", "Segment: %d %d %d x %d (%d)", mPlaying.iter->x, mPlaying.iter->y, mPlaying.iter->z, mPlaying.iter->repeat, mPlaying.nLeft);
    mChar->writeValue(*mPlaying.iter, 3, false);

    mPlaying.nLeft--;
    if (mPlaying.nLeft == 0) mPlaying.iter++;
}


void
NimBLE::COYOTE::Device::V2::setMaxPower(uint8_t A, uint8_t B)
{
    if (A > mPower.max) A = mPower.max;
    if (B > mPower.max) B = mPower.max;

    mParent->getChannelA().mMaxPower = A;
    mParent->getChannelB().mMaxPower = B;
}

bool
NimBLE::COYOTE::Device::V2::Channel::powerUpdateReq(uint8_t& pow)
{
    pow = mSetPower;
    return mPower != mSetPower;
}

void
NimBLE::COYOTE::Device::V2::run()
{
    auto xLastWakeTime = xTaskGetTickCount();

    unsigned int nextPowerUpdate = 0;
    while (1) {
        // Update power every second
        if (nextPowerUpdate == 0) {

            uint8_t powA;
            uint8_t powB;
            bool    newPower = mParent->getChannelA().powerUpdateReq(powA) ||
                            mParent->getChannelB().powerUpdateReq(powB);

            // Only update power if there was a change requested
            if (!newPower) return;

            ESP_LOGI(mParent->getName(), "Set power to A:%d->%d  B:%d->%d",
                     (uint16_t)mParent->getChannelA().getPower(), powA,
                     (uint16_t)mParent->getChannelB().getPower(), powB);

            PowerVal pwr(powA * mPower.step, powB * mPower.step);

            mPower.charac->writeValue(pwr, 3, false);

            nextPowerUpdate = 10;
        } else {
            nextPowerUpdate--;
        }

        //
        // Send a waveform segment every 100ms to keep the generator alive.
        // To make sure the segment is present, we sent the first segment
        // at t=0, then we send the subsequent ones at t=50, 150, 250, etcc
        //

        // Check if a new waveform was started (t=0)
        for (auto& it : mParent->mChannel) it->startNewWaveform();
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(50));

        for (auto& it : mParent->mChannel) it->sendNextSegment();
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(50));
    }
}