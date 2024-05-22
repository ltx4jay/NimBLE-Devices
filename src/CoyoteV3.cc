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


class V3Channel : public Channel
{
public:
    V3Channel(Device* parent, const char* name);

    struct Playing
    {
        SemaphoreHandle_t            mutex;
        bool                         run;
        V3::Waveform                 wave;
        V3::Waveform::const_iterator iter;
        unsigned int                 nLeft;
    } mPlaying;

    virtual void setWaveform(const V2::Waveform& wave, uint8_t power = 0) override;
    virtual void setWaveform(const V3::Waveform& wave, uint8_t power = 0) override;
    virtual void start(long secs = 0) override;
    virtual void stop() override;

    bool powerUpdateReq(uint8_t& pow) override;

    void getNextSegment(uint8_t &freq, uint8_t &intensity);
};


NimBLE::COYOTE::Device::V3::V3(const char* uniqueName, const char* macAddr)
: Device(uniqueName, "47L121000", macAddr)
, mCharac(nullptr)
, mNextSerial(0x10)
, mPendingSerial(0x00)
{
}


bool
NimBLE::COYOTE::Device::V3::initDevice()
{
    auto pSvc = mClient->getService("00000000-1000-8000-0080-5f9b34fb180a");
    if (pSvc == NULL) {
        ESP_LOGE(getName(), "Cannot find battery service.\n");
        notifyEvent(ERROR);
        return false;
    }

    // auto firmware = pSvc->getCharacteristic("955A1501-0FE2-F5AA-A094-84B8D4F3E8AD");
    // uint16_t fw = * ((uint16_t*) firmware->readValue().data());
    // ESP_LOGI(getName(), "Firmware %02x.%02x", fw & 0x00FF, fw >> 8);

    auto battery  = pSvc->getCharacteristic("00000000-1000-8000-0080-5f9b34fb1500");
    if (battery != nullptr) battery->subscribe(true, std::bind(&Device::notifyBattery, this,
                                                               std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
    
    pSvc = mClient->getService("00000000-1000-8000-0080-5f9b34fb180c");
    if (pSvc == NULL) {
        ESP_LOGE(getName(), "Cannot find power service.\n");
        notifyEvent(ERROR);
        return false;
    }
  
    mCharac = pSvc->getCharacteristic("00000000-1000-8000-0080-5f9b34fb150a");

    auto resp  = pSvc->getCharacteristic("00000000-1000-8000-0080-5f9b34fb150b");
    if (resp != nullptr) resp->subscribe(true, std::bind(&Device::V3::notifyResp, this,
                                                         std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));

    mChannel[0] = new V3Channel(this, "A");
    mChannel[1] = new V3Channel(this, "B");

    return true;
}

void
NimBLE::COYOTE::Device::V3::setMaxPower(uint8_t A, uint8_t B)
{
    if (A > 200) A = 200;
    if (B > 200) B = 200;

    getChannelA().mMaxPower = A;
    getChannelB().mMaxPower = B;
}

void
NimBLE::COYOTE::Device::V3::run()
{
    auto xLastWakeTime = xTaskGetTickCount();

    uint8_t msg[20];

    // Set max power (200) and balance parameters (32, 32)
    mCharac->writeValue((uint8_t*) "\xBF\xC8\xC8\x20\x20\x20\x20", 7, false);
    
    while (1) {
        msg[0] = 0xB0;

        // No change in power
        msg[1] = 0x00;
        msg[2] = 0x00;
        msg[3] = 0x00;

        uint8_t powA;
        uint8_t powB;
        bool    newPowerA = getChannelA().powerUpdateReq(powA);
        bool    newPowerB = getChannelB().powerUpdateReq(powB);

        // DG Labs recommends only one pending power change request
        if (!mPendingSerial) {
            // Always use absolute values
            if (newPowerA) {
                msg[1] |= mNextSerial | 0x0C;
                msg[2] = powA;
            }
            if (newPowerB) {
                msg[1] |= mNextSerial | 0x03;
                msg[3] = powB;
            }
            mPendingSerial = msg[1] >> 4;
            if (mPendingSerial) {
                mNextSerial += 0x10;
                if (mNextSerial == 0x00) mNextSerial = 0x10;
            }
        }

        ESP_LOGI(getName(), "Set power to A:%d->%d  B:%d->%d",
                 (uint16_t)getChannelA().getPower(), powA,
                 (uint16_t)getChannelB().getPower(), powB);

        int k = 4;
        for (auto& it : mChannel) {
            for (unsigned i = 0; i < 4; i++) {
                ((V3Channel*) it)->getNextSegment(msg[k], msg[k+4]);
                k++;
            }
        }

        mCharac->writeValue(msg, sizeof(msg), false);
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(100));
    }
}

void
NimBLE::COYOTE::Device::V3::notifyResp(NimBLERemoteCharacteristic* pRemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify)
{
    if (pData[0] == 0xB1) {
        if (pData[1] != mPendingSerial) {
            ESP_LOGE(getName(), "Unexpected response serial number 0x%02x instead of 0x%02x.", pData[1], mPendingSerial);
            return;
        }
        mChannel[0]->mPower = pData[2];
        mChannel[1]->mPower = pData[3];

        mPendingSerial = 0x00;
        return;
    }

    if (pData[0] == 0xBE) {
        ESP_LOGI(getName(), "MaxPow: %d %d   Balance Params: %d/%d %d/%d", pData[1], pData[2], pData[3], pData[5], pData[4], pData[6]);
        return;
    }

    ESP_LOGE(getName(), "Unexpected 0x%02x response received.", pData[0]);
}

NimBLE::COYOTE::V3::WaveVal::WaveVal(uint8_t freq, uint16_t intensity, uint16_t rpt)
    : xy(freq)
    , z(intensity)
    , repeat(rpt)
{
    if (xy <  10) xy = 10;
    if (xy > 240) xy = 240;
    if (z  > 100)  z = 100;
    if (rpt == 0) rpt = 1;
}


uint8_t V2freqToV3(uint32_t x, uint32_t y)
{
    uint32_t xy = x + y;

    if (xy <= 10)  return 10;
    if (xy <= 100) return xy;
    if (xy <= 600) return (xy - 100) / 5 + 100;
    if (xy <= 1000) return (xy - 600) / 10 + 200;
    return 240;
}

NimBLE::COYOTE::V3::WaveVal::WaveVal(V2::WaveVal v2)
    : xy(V2freqToV3(v2.x, v2.y))
    , z(v2.z * 5)
    , repeat(v2.repeat * 4)
{}


unsigned int
NimBLE::COYOTE::V3::WaveVal::duration() const
{
    if (xy <= 100) return xy;
    if (xy <= 200) return (xy - 100) * 5 + 100;
    if (xy <= 240) return (xy - 200) * 10 + 600;
    return 1000;
}


uint16_t
NimBLE::COYOTE::V3::WaveVal::getRepeat() const
{
    return repeat;
}


uint8_t
NimBLE::COYOTE::V3::WaveVal::getFreq() const
{
    return xy;
}


uint8_t
NimBLE::COYOTE::V3::WaveVal::getInt() const
{
    return z;
}


void
V3Channel::setWaveform(const V2::Waveform& wave, uint8_t power)
{
    SemLockGuard lk(mPlaying.mutex);

    mPlaying.wave  = {};
    for (auto it : wave) mPlaying.wave.push_back(it);
    mPlaying.iter  = mPlaying.wave.end();
    mPlaying.nLeft = 0;

    setPower(power);
}

void
V3Channel::setWaveform(const V3::Waveform& wave, uint8_t power)
{
    SemLockGuard lk(mPlaying.mutex);

    mPlaying.wave  = wave;
    mPlaying.iter  = mPlaying.wave.end();
    mPlaying.nLeft = 0;

    setPower(power);
}


void
V3Channel::start(long secs)
{
    NimBLE::COYOTE::Channel::start(secs);

    if (mPlaying.wave.size() == 0 || mPlaying.run) return;

    SemLockGuard lk(mPlaying.mutex);

    mPlaying.iter  = mPlaying.wave.end();
    mPlaying.nLeft = 0;
    mPlaying.run   = true;
}


void
V3Channel::stop()
{
    NimBLE::COYOTE::Channel::stop();

    SemLockGuard lk(mPlaying.mutex);
    
    mPlaying.run = false;
}


void
V3Channel::getNextSegment(uint8_t &freq, uint8_t &intensity)
{
    freq      = 255;
    intensity = 255;
    if (!mPlaying.run) return;

    if (mPlaying.nLeft == 0) {
        if (mPlaying.iter == mPlaying.wave.end()) {
            mPlaying.iter = mPlaying.wave.begin();
        }

        if (mPlaying.iter == mPlaying.wave.end()) return;

        mPlaying.nLeft = (((mPlaying.iter->duration() - 1) / 100) + 1) * mPlaying.iter->getRepeat();
    }

    freq      = mPlaying.iter->getFreq();
    intensity = mPlaying.iter->getInt();

    mPlaying.nLeft--;
    if (mPlaying.nLeft == 0) mPlaying.iter++;
}