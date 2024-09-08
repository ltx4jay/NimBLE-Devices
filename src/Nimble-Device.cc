// 
// Copyright (c) 2023 ltx4jay@yahoo.com
//
// Licensed under MIT License
//
// The code is provided as-is, with no warranties of any kind. Not suitable for any purpose.
// Provided as an example and exercise in BLE development only.
//

#include "NimBLE-Device.hh"
#include <sys/_intsup.h>

using namespace NimBLE;



std::vector<InterestingDevice*> InterestingDevice::sAllDevices;


InterestingDevice::InterestingDevice(const char* name, const char* bleName, const char* macAddr, uint8_t addrType)
    : NimBLEClientCallbacks()
    , mDev(NULL)
    , mClient(NULL)
    , mUniqueName(name)
    , mDeviceName(bleName)
    , mAddress(macAddr, addrType)
    , mMustFind(false)
    , mFound(false)
    , mConnected(false)
    , mInit(false)
    , mService(true)
    , mEventCb()
{
}


InterestingDevice::~InterestingDevice()
{
    if (mClient != NULL && mClient->isConnected()) mClient->disconnect();
}

bool
InterestingDevice::addToDevicePool(InterestingDevice* dev, bool mustFind)
{
    for (auto it : sAllDevices) {
        if (it->mUniqueName == dev->mUniqueName) return false;
    }

    sAllDevices.push_back(dev);
    dev->mMustFind = mustFind;

    return true;
}


InterestingDevice*
InterestingDevice::foundDevice(NimBLEAdvertisedDevice* dev)
{
    std::string macAddr = dev->getAddress().toString();

    ESP_LOGD("NimBLE-Device", "Found \"%s\" (%s)", dev->getName().c_str(), macAddr.c_str());

    for (auto it : sAllDevices) {
        if (it->mFound) continue;

        if (it->mAddress != macAddr) continue;
        if (it->mDeviceName != "" && dev->getName() != "" && it->mDeviceName != dev->getName()) continue;

        it->mDev   = dev;
        it->mFound = true;

        ESP_LOGI("NimBLE-Device", "FOUND \"%s\" (%s)", dev->getName().c_str(), macAddr.c_str());
        it->notifyEvent(FOUND);

        return it;
    }

    return NULL;
}


const std::vector<InterestingDevice*>&
InterestingDevice::getDevices()
{
    return sAllDevices;
}


std::vector<InterestingDevice*>
InterestingDevice::getFoundDevices()
{
    std::vector<InterestingDevice*> devs;
    for (auto it : sAllDevices) {
        if (it->mFound) devs.push_back(it);
    }
    return devs;
}


InterestingDevice*
InterestingDevice::getByName(const char* name)
{
    for (auto it : sAllDevices) {
        if (it->mUniqueName == name) return it;
    }
    return NULL;
}


bool
InterestingDevice::allFound()
{
    for (auto it : sAllDevices) {
        if (it->mMustFind && !it->mInit) return false;
    }
    return true;
}


const char*
InterestingDevice::getName() const
{
    return mUniqueName.c_str();
}


void
InterestingDevice::subscribeEvents(std::function<void(uint8_t)> fct)
{
    mEventCb = fct;
}


void
InterestingDevice::notifyEvent(uint8_t event)
{
    if (mEventCb) mEventCb(event);
}


void
InterestingDevice::subscribeBatteryLevel(std::function<void(uint8_t)> fct)
{
    mBatteryCb = fct;
}


void
InterestingDevice::notifyBatteryLevel(uint8_t percent)
{
    ESP_LOGI(getName(), "Battery Level = %d%%", percent);
    if (mBatteryCb) mBatteryCb(percent);
}


bool
InterestingDevice::wasFound()
{
    return mFound;
}


bool
InterestingDevice::initFoundDevices()
{
    // Connect before probing for fast-fail
    for (auto it : sAllDevices) {
        if (!it->mFound) continue;

        if (!it->connect()) {
            ESP_LOGI(it->getName(), "InterestingDevice connect() failed!");
            return false;
        }
    }
    for (auto it : sAllDevices) {
        if (!it->mFound) continue;
        if (!it->initDevice()) return false;
    }
    return true;
}


bool
InterestingDevice::initDevice()
{
    if (mInit) return true;
    
    if (!connect()) {
        ESP_LOGE(getName(), "Cannot connect device");
        return false;
    }

    notifyEvent(START_INIT);
    
    mInit = doInitDevice();

    notifyEvent(INIT);

    ESP_LOGI(mUniqueName.c_str(), "Ready!");

    return mInit;
}


bool
InterestingDevice::isConnected()
{
    return mInit;
}


bool
InterestingDevice::doConnect(bool refresh)
{
    if (!mClient->connect(refresh)) {
        ESP_LOGI(mUniqueName.c_str(), "connect() failed!");
        return false;
    }

    return true;
}


bool
InterestingDevice::connect(bool refresh)
{
    ESP_LOGI(mUniqueName.c_str(), "Connecting...");
    if (mConnected) return true;
                
    notifyEvent(START_CONNECT);
    
    /** Check if we have a client we should reuse first **/
    if (NimBLEDevice::getClientListSize()) {
        /** Special case when we already know this device, we send false as the
         *  second argument in connect() to prevent refreshing the service database.
         *  This saves considerable time and power.
         */
        mClient = NimBLEDevice::getClientByPeerAddress(mAddress);
        if (mClient) {
            if (doConnect(false)) {
                notifyEvent(CONNECTED);
                mConnected = true;
                return true;
            }
        } else {
            /** We don't already have a client that knows this device,
             *  we will check for a client that is disconnected that we can use.
             */
            mClient = NimBLEDevice::getDisconnectedClient();
        }
    }
    if (mClient == NULL) {
        mClient = NimBLEDevice::createClient(mAddress);
    }
    mClient->setClientCallbacks(this, false);
            
    /** Set how long we are willing to wait for the connection to complete (milliseconds), default is 30. */
    mClient->setConnectTimeout(3000);
        
    if (!doConnect(refresh)) {
        /** Created a client but failed to connect, don't need to keep it as it has no data */
        // This hangs...
        // NimBLEDevice::deleteClient(mClient);
        ESP_LOGI(mUniqueName.c_str(), "connect() failed!");
        notifyEvent(ERROR);
        return false;
    }
    
    if (!mClient->isConnected()) {
        ESP_LOGI(mUniqueName.c_str(), "Not connected!");
        notifyEvent(ERROR);
        return false;
    }

    mConnected = true;
    ESP_LOGI(mUniqueName.c_str(), "Connected!");

    notifyEvent(CONNECTED);
    
    return true;
}


void
InterestingDevice::serviceAllDevices(long nowInMs)
{
    for (auto it : sAllDevices) {
        if (it->mInit && it->mService) it->serviceLoop(nowInMs);
    }
}


bool
InterestingDevice::disableAutoService()
{
    mService = false;

    return true;
}


void
InterestingDevice::reset()
{
}


void
InterestingDevice::doDisconnect()
{
    mConnected = false;
    mInit      = false;
}


static char
itoh(uint8_t i)
{
    i &= 0x0f;
    if (i < 10) return '0' + i;
    return 'a' + i - 10;
}


const char*
InterestingDevice::image(const uint8_t *msg, unsigned int len)
{
    static char buf[1024];

    auto maxLen = sizeof(buf) / 2 - 1;
    if (len >= maxLen) len = maxLen;

    auto p = buf;
    auto d = msg;

    while (len) {
        *p++ = itoh(*d >> 4);
        *p++ = itoh(*d++);
        len--;
    }

    *p = '\0';

    return buf;
}
