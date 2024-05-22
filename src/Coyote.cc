//
// NimBLE-Device convenience API for a Dungeon Labs Coyote
// https://dungeon-lab.com/home.php
//
// WARNING: USE AT YOUR OWN RISK
//
// 
// Copyright (c) 2023 ltx4jay@yahoo.com
//
// Licensed under MIT License
//
// The code is provided as-is, with no warranties of any kind. Not suitable for any purpose.
// Provided as an example and exercise in BLE development only.
//

#include "NimBLE-Device/Coyote.hh"


using namespace NimBLE::COYOTE;


NimBLE::COYOTE::Channel::Channel(Device *parent, const char* name)
    : mDevice(parent)
    , mName(name)
    , mMaxPower(100)
    , mSetPower(0)
    , mPower()
    , mSafeMode(true)
{
}


std::string
NimBLE::COYOTE::Channel::getName()
{
    return std::string(mDevice->getName()) + "." + mName;
}


void
NimBLE::COYOTE::Channel::setPower(uint8_t val, bool unsafe)
{
    if (val > mMaxPower) {
        ESP_LOGI(getName().c_str(), "Rejecting power setting %d > MAX.", val);
        return;
    }

    mSafeMode = !unsafe;

    if (mSafeMode) {
        // If we ask for too much of a jump, it's probably a bug
        if (mSetPower < val && val > 50 && mSetPower - val > 10) {
            ESP_LOGI(getName().c_str(), "(1) Rejecting power setting %d -> %d.", mSetPower, val);
            return;
        }
        // If we ask for too much of a jump, it's probably a bug
        if (mPower < val && val > 50 && mPower - val > 10) {
            ESP_LOGI(getName().c_str(), "(2) Rejecting power setting %d -> %d.", mSetPower, val);
            return;
        }
    }
    
    mSetPower = val;
}


void
NimBLE::COYOTE::Channel::incrementPower(int delta)
{
    if (delta < 0 && -delta > mPower) setPower(0);
    else setPower(mSetPower + delta);

    ESP_LOGI(getName().c_str(), "Incremented power by %d: %d", delta, mSetPower);
}


uint8_t
NimBLE::COYOTE::Channel::getPower()
{
    return mPower;
}


void
NimBLE::COYOTE::Channel::subscribePower(std::function<void(uint8_t)> fct)
{
    mPowerCb = fct;
}


void
NimBLE::COYOTE::Channel::updatePower(uint8_t power)
{
    if (mPowerCb) mPowerCb(power);
    mPower = power;
}


void
NimBLE::COYOTE::Channel::start(long secs)
{
}


void
NimBLE::COYOTE::Channel::stop()
{
}


NimBLE::COYOTE::Device::Device(const char* uniqueName, const char* bleName, const char* macAddr)
    : InterestingDevice(uniqueName, bleName, macAddr)
    , mChannel{nullptr, nullptr}
{
}


NimBLE::COYOTE::Device::~Device()
{
    if (mChannel[0] != nullptr) delete mChannel[0];
    if (mChannel[1] != nullptr) delete mChannel[1];
}


bool
NimBLE::COYOTE::Device::doInitDevice()
{
    mClient->discoverAttributes();
    if (!initDevice()) return false;

    ESP_LOGI(getName(), "Connected!");
    xTaskCreate(&runTask, getName(), 8192, this, 5, &mTaskHandle);

    return true;
}


NimBLE::COYOTE::Channel&
NimBLE::COYOTE::Device::getChannelA()
{
    return *mChannel[0];
}


NimBLE::COYOTE::Channel&
NimBLE::COYOTE::Device::getChannelB()
{
    return *mChannel[1];
}


void
NimBLE::COYOTE::Device::subscribeBattery(std::function<void(uint8_t)> fct)
{
    mBatteryCb = fct;
}

void
NimBLE::COYOTE::Device::notifyBattery(NimBLERemoteCharacteristic* pRemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify)
{
    uint8_t battery = pData[0];
    ESP_LOGI(getName(), "Battery Level = %d%%", battery);

    if (mBatteryCb) mBatteryCb(battery);
}


void
NimBLE::COYOTE::Device::runTask(void *pvParameter)
{
    auto dev = (NimBLE::COYOTE::Device*) pvParameter;
    
    auto xLastWakeTime = xTaskGetTickCount();

    // There is no point in starting right away... let's wait 1 sec
    vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(1000));

    dev->run();
}


void
NimBLE::COYOTE::Device::serviceLoop(long nowInMs)
{
}