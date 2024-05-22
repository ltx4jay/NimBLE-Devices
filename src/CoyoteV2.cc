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


using namespace NimBLE::COYOTE;

class NimBLE::COYOTE::V2Channel : public Channel
{
public:
    V2Channel(Device* parent, const char* name, NimBLERemoteCharacteristic* charac);

    NimBLERemoteCharacteristic* mChar;

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

    void startNewWaveform();
    void sendNextSegment();
};


NimBLE::COYOTE::Device::V2::V2(const char* uniqueName, const char* macAddr)
: Device(uniqueName, "D-LAB ESTIM01", macAddr)
{
}


bool
NimBLE::COYOTE::Device::V2::initDevice()
{
    auto pSvc = mClient->getService("955A180A-0FE2-F5AA-A094-84B8D4F3E8AD");
    if (pSvc == NULL) {
        ESP_LOGE(getName(), "Cannot find battery service.\n");
        notifyEvent(ERROR);
        return false;
    }

    auto firmware = pSvc->getCharacteristic("955A1501-0FE2-F5AA-A094-84B8D4F3E8AD");
    uint16_t fw = * ((uint16_t*) firmware->readValue().data());
    ESP_LOGI(getName(), "Firmware %02x.%02x", fw & 0x00FF, fw >> 8);

    auto battery  = pSvc->getCharacteristic("955A1500-0FE2-F5AA-A094-84B8D4F3E8AD");
    if (battery != nullptr) battery->subscribe(true, std::bind(&Device::notifyBattery, this,
                                                               std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
    
    pSvc = mClient->getService("955A180B-0FE2-F5AA-A094-84B8D4F3E8AD");
    if (pSvc == NULL) {
        ESP_LOGE(getName(), "Cannot find power service.\n");
        notifyEvent(ERROR);
        return false;
    }
  
    auto cfgChar      = pSvc->getCharacteristic("955A1507-0FE2-F5AA-A094-84B8D4F3E8AD");
    mPower.charac   = pSvc->getCharacteristic("955A1504-0FE2-F5AA-A094-84B8D4F3E8AD");
    if (cfgChar == nullptr || mPower.charac == nullptr) {
        ESP_LOGE(getName(), "Cannot find all power generator characteristics.\n");
        notifyEvent(ERROR);
        return false;
    }
    mChannel[0] = new NimBLE::COYOTE::V2Channel(this, "A", pSvc->getCharacteristic("955A1506-0FE2-F5AA-A094-84B8D4F3E8AD"));
    mChannel[1] = new NimBLE::COYOTE::V2Channel(this, "B", pSvc->getCharacteristic("955A1505-0FE2-F5AA-A094-84B8D4F3E8AD"));

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

    return true;
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

    getChannelA().updatePower(pow->A/mPower.step);
    getChannelB().updatePower(pow->B/mPower.step);

    ESP_LOGI(getName(), "Power Setting  A:%3d -> %d   B:%3d -> %d",
             getChannelA().mPower, getChannelA().mSetPower,
             getChannelB().mPower, getChannelB().mSetPower);
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
            bool    newPower = getChannelA().powerUpdateReq(powA) ||
                            getChannelB().powerUpdateReq(powB);

            // Only update power if there was a change requested
            if (!newPower) return;

            ESP_LOGI(getName(), "Set power to A:%d->%d  B:%d->%d",
                     (uint16_t) getChannelA().getPower(), powA,
                     (uint16_t) getChannelB().getPower(), powB);

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
        for (auto& it : mChannel) ((NimBLE::COYOTE::V2Channel*) it)->startNewWaveform();
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(50));

        for (auto& it : mChannel) ((NimBLE::COYOTE::V2Channel*) it)->sendNextSegment();
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(50));
    }
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


uint16_t
NimBLE::COYOTE::V2::WaveVal::getRepeat() const
{
    return repeat;
}


NimBLE::COYOTE::V2::WaveVal::operator uint8_t*() const
{
    return (uint8_t*) this;
}


NimBLE::COYOTE::V2Channel::V2Channel(Device *parent, const char* name, NimBLERemoteCharacteristic *charac)
    : Channel(parent, name)
    , mChar(charac)
    , mPlaying{xSemaphoreCreateRecursiveMutex(), false, false}
{
}


void
NimBLE::COYOTE::V2Channel::setWaveform(const V2::Waveform& wave, uint8_t power)
{
    SemLockGuard lk(mPlaying.mutex);
    
    mPlaying.wave  = wave;
    mPlaying.iter  = mPlaying.wave.end();
    mPlaying.nLeft = 0;
    mPlaying.start = true;

    setPower(power);
}


void
NimBLE::COYOTE::V2Channel::start(long secs)
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
NimBLE::COYOTE::V2Channel::stop()
{
    NimBLE::COYOTE::Channel::stop();

    SemLockGuard lk(mPlaying.mutex);
    
    mPlaying.start = false;
    mPlaying.run   = false;
}


void
NimBLE::COYOTE::V2Channel::startNewWaveform()
{
    SemLockGuard lk(mPlaying.mutex);

    if (!mPlaying.start) return;

    sendNextSegment();
    mPlaying.start = false;
}


void
NimBLE::COYOTE::V2Channel::sendNextSegment()
{
    if (mPlaying.nLeft == 0) {
        if (mPlaying.iter == mPlaying.wave.end()) {
            mPlaying.iter = mPlaying.wave.begin();
        }
        if (mPlaying.iter == mPlaying.wave.end()) return;

        mPlaying.nLeft = (((mPlaying.iter->duration() - 1) / 100) + 1) * mPlaying.iter->getRepeat();
    }
    
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

    getChannelA().mMaxPower = A;
    getChannelB().mMaxPower = B;
}