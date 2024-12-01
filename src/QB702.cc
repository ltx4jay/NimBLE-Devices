#include "NimBLE-Device/QB702.hh"


NimBLE::QB702::Device::Device(const char* uniqueName, const char* macAddr)
    : NimBLE::Keyboard::Device(uniqueName, "QB702", macAddr, 1)
{
}


NimBLE::QB702::Device::~Device()
{
}


bool
NimBLE::QB702::Device::doInitDevice()
{
    mClient->discoverAttributes();    

    auto pSvc = mClient->getService(NimBLEUUID((uint16_t) 0x0001));
    if (pSvc == NULL) {
        ESP_LOGE(getName(), "Cannot find button service.");
        return false;
    }
    
    auto pChr = pSvc->getCharacteristic(NimBLEUUID((uint16_t) 0x0003));
    if (pChr == nullptr || !pChr->canNotify()) {
        ESP_LOGE(getName(), "Cannot find button characteristic.");
        return false;
    }

    pChr->subscribe(true, std::bind(&NimBLE::QB702::Device::notifyButton, this,
                                    std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
    
    return true;
}


void
NimBLE::QB702::Device::serviceLoop(long nowInMs)
{
}


void
NimBLE::QB702::Device::notifyButton(NimBLERemoteCharacteristic* pRemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify)
{
    // The counter value is in pData[6..7] in big endian order
    // ESP_LOGI("QB702", "-> %s", image(pData, length));    

    // Translate button press to a SPC
    NimBLE::Keyboard::Device::publish(' ', NimBLE::Keyboard::Device::PRESSED);
}
