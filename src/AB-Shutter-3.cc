#include "NimBLE-Device/AB-Shutter-3.hh"


NimBLE::AB_Shutter3::Device::Device(const char* uniqueName, const char* macAddr)
    : NimBLE::Keyboard::Device(uniqueName, "AB Shutter3", macAddr)
{
}


NimBLE::AB_Shutter3::Device::~Device()
{
}


bool
NimBLE::AB_Shutter3::Device::doInitDevice()
{
    mClient->discoverAttributes();    

    NimBLERemoteService *pSvc = nullptr;

    pSvc = mClient->getService(NimBLEUUID((uint16_t) 0x1812));
    if (pSvc == NULL) {
        ESP_LOGE(getName(), "Cannot find button service.");
        return false;
    }
    
    /*
     * The button is on the SECOND instance of the 0x2A4D characteristic!
     * Subscribe them all...
     */
    for (auto it : *(pSvc->getCharacteristics())) {
        if (!it->canNotify()) continue;
        if (it->getUUID() != (uint16_t) 0x2A4D) continue;
        
        it->subscribe(true, std::bind(&NimBLE::AB_Shutter3::Device::notifyButton, this,
                                      std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
    }
    
    return true;
}


void
NimBLE::AB_Shutter3::Device::serviceLoop(long nowInMs)
{
}


void
NimBLE::AB_Shutter3::Device::notifyButton(NimBLERemoteCharacteristic* pRemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify)
{
    // 00 00 -> off
    // 02 00 -> ON

    // Translate button press to a SPC
    NimBLE::Keyboard::Device::publish(' ', (pData[0] == 0x02) ? NimBLE::Keyboard::Device::PRESSED : NimBLE::Keyboard::Device::RELEASED);
}
