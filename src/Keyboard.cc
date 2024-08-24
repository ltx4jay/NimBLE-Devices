#include "NimBLE-Device/Keyboard.hh"


NimBLE::Keyboard::Device::Device(const char* uniqueName, const char* bleName, const char* macAddr, uint8_t addrType)
    : NimBLE::InterestingDevice(uniqueName, bleName, macAddr, addrType)
    , mListeners()
{
    for (uint16_t i = 0; i < 256; i++) mKeyMap[i] = i;
    bzero(mPressed, sizeof(mPressed));
}


NimBLE::Keyboard::Device::~Device()
{
}


void
NimBLE::Keyboard::Device::setKeyMap(const uint8_t map[])
{
    memcpy(mKeyMap, map, sizeof(mKeyMap));
}


void
NimBLE::Keyboard::Device::setKeyMap(uint8_t from, uint8_t to)
{
    mKeyMap[from] = to;
}


bool
NimBLE::Keyboard::Device::isPressed(uint8_t key)
{
    return mPressed[key];
}


void
NimBLE::Keyboard::Device::subscribe(std::function<void(uint8_t key, Event_t e)> fct)
{
    mListeners.push_back({fct});
}


void
NimBLE::Keyboard::Device::publish(uint8_t key, Event_t e)
{
    mPressed[key] = (e == PRESSED);
    for (auto& it : mListeners) it.fct(key, e);
}
