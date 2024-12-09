#include "NimBLE-Device/iTag.hh"


NimBLE::iTag::Device::Device(const char* uniqueName, const char* macAddr, uint8_t addrType)
    : NimBLE::InterestingDevice(uniqueName, "iTAG", macAddr, addrType)
{
}


NimBLE::iTag::Device::~Device()
{
}


bool
NimBLE::iTag::Device::doInitDevice()
{
    mClient->discoverAttributes();    

    NimBLERemoteService *pSvc = nullptr;

    pSvc = mClient->getService(NimBLEUUID((uint16_t) 0x180F));
    if (pSvc == nullptr) {
        ESP_LOGE(getName(), "Cannot find battery service.");
        return false;
    }
    auto battery = pSvc->getCharacteristic("00002A19-0000-1000-8000-00805F9B34FB");
    if (battery == nullptr) {
        ESP_LOGE(getName(), "Cannot find battery characteristics.");
        return false;
    }
    battery->subscribe(true, std::bind(&NimBLE::iTag::Device::notifyBattery, this,
                                       std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
    
    pSvc = mClient->getService(NimBLEUUID((uint16_t) 0x1802));
    if (pSvc == nullptr) {
        ESP_LOGE(getName(), "Cannot find alarm service.");
        return false;
    }
    mAlarmChr = pSvc->getCharacteristic("00002A06-0000-1000-8000-00805F9B34FB");
    if (mAlarmChr == nullptr) {
        ESP_LOGE(getName(), "Cannot find alarm characteristics.");
        return false;
    }

    return true;
}


void
NimBLE::iTag::Device::serviceLoop(long nowInMs)
{
}


void
NimBLE::iTag::Device::notifyBattery(NimBLERemoteCharacteristic* pRemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify)
{
    NimBLE::InterestingDevice::notifyBatteryLevel(pData[0]);
}


void
NimBLE::iTag::Device::setAlarm(AlarmSetting_t level)
{
    if (mAlarmChr == nullptr) return;

    mAlarmChr->writeValue(level);
}
