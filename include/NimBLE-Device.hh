// 
// Copyright (c) 2023 ltx4jay@yahoo.com
//
// Licensed under MIT License
//
// The code is provided as-is, with no warranties of any kind. Not suitable for any purpose.
// Provided as an example and exercise in BLE development only.
//

#pragma once

#include "NimBLEDevice.h"
#include "freertos/FreeRTOS.h"

#include <functional>
#include <cstdint>
#include <string>
#include <vector>


class SemLockGuard
{
    SemaphoreHandle_t mMutex;
    
public:
    SemLockGuard(SemaphoreHandle_t mutex)
        : mMutex(mutex)
        {
            xSemaphoreTakeRecursive(mMutex, portMAX_DELAY);
        }

    ~SemLockGuard()
        {
            xSemaphoreGiveRecursive(mMutex);
        }
};


namespace NimBLE {


//
// Base class for a NimBLE device of interest
//
class InterestingDevice : public NimBLEClientCallbacks
{
public:
    //
    // Add a device of interest to the pool of interesting devices
    // Returns true if the specified name is unique.
    //
    static bool addToDevicePool(InterestingDevice* dev, bool mustFind = false);

    //
    // Check if the advertised device matches any of the not-yet-found interesting devices.
    // Returns a pointer to the device if the advertised devices matches, or NULL otherwise.
    //
    // A device matches if its name and MAC address (if specified) match.
    //
    static InterestingDevice* foundDevice(NimBLEAdvertisedDevice* dev);

    //
    // Return all interesting devices, whether found or not.
    //
    static const std::vector<InterestingDevice*>& getDevices();

    //
    // Return all found interesting devices
    //
    static std::vector<InterestingDevice*> getFoundDevices();

    //
    // Return a pointer to an interesting device by given unique name (not BLE device name)
    // Returns NULL if the specified device does not exist.
    //
    static InterestingDevice* getByName(const char* name);

    //
    // Returns true if all must-find devices were found
    //
    static bool allFound();

    //
    // Return unique name
    //
    const char* getName() const;

    //
    // Subscribe to device events
    //
    typedef enum {ERROR, FOUND, START_CONNECT, CONNECTED, START_INIT, INIT, DISCONNECTED} Events_t;
    void subscribeEvents(std::function<void(uint8_t)> fct);

    //
    // Returns true if this device was found
    //
    bool wasFound();

    //
    // Initialize all not-yet-initialized found devices, returning true if everything succeeded
    //
    static bool initFoundDevices();

    //
    // Connect, probe, and initialize the device.
    // Returns true if successful.
    //
    bool initDevice();

    //
    // Returns true of the device is fully connected and ready to use
    //
    bool isConnected();

    //
    // Subscribe to the battery level notifications (optional)
    //
    void subscribeBatteryLevel(std::function<void(uint8_t percent)> fct);

    //
    // Service all found devices unless they have explicitly opted out of this global service call.
    // To be called as often as possible if all found devices to be serviced using a single thread
    //
    static void serviceAllDevices(long nowInMs);

    //
    // Opt-out of the serviceAllDevices() function.
    // User is responsible for explicitly calling serviceLoop()
    //
    bool disableAutoService();

    //
    // Service this device, if it was found.
    //
    virtual void serviceLoop(long nowInMs) = 0;

    //
    // Delete this device.
    //
    virtual ~InterestingDevice();

    //
    // Delete all known interesting devices
    //
    static void reset();
    

protected:
    NimBLEAdvertisedDevice* mDev;
    NimBLEClient*           mClient;

    //
    // Create an interesting device with the given unique name, BLE device name, and MAC address
    //
    InterestingDevice(const char* name, const char* bleName, const char* macAddr, uint8_t addrType = 0);

    //
    // Change the MAC address
    //
    void changeAddress(const char* macAddr);

    //
    // Notify of an event
    //
    virtual void notifyEvent(uint8_t event);

    //
    // Notify a battery level
    //
    virtual void notifyBatteryLevel(uint8_t percent);
    
    bool connect(bool refresh = true);
    virtual void doDisconnect();

    //
    // For debugging: create an image of a byte string
    //
    const char* image(const uint8_t *msg, unsigned int len);

    
private:
    static std::vector<InterestingDevice*> sAllDevices;
    
    std::string         mUniqueName;
    std::string         mDeviceName;
    NimBLEAddress       mAddress;
    bool                mMustFind;
    bool                mFound;
    bool                mConnected;
    bool                mInit;
    bool                mService;

    std::function<void(uint8_t)> mEventCb;
    std::function<void(uint8_t)> mBatteryCb;

    bool doConnect(bool refresh, int attempt = 1);
    virtual bool doInitDevice() = 0;

    //
    // These are default implementations for NimBLE callbacks
    //
    virtual void onConnect(NimBLEClient* pClient)  override
        {
        }
    
    virtual void onDisconnect(NimBLEClient* pClient, int reason)  override
        {
            notifyEvent(DISCONNECTED);
            InterestingDevice::doDisconnect();
        }

    bool onConnParamsUpdateRequest(NimBLEClient* pClient, const ble_gap_upd_params* params)  override
        {
            return true;
        }
};


}
