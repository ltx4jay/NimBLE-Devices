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

#include <functional>
#include <stdint>
#include <string>
#include <vector>


namespace NimBLE {


//
// Base class for a NimBLE device of interest
//
class InterestingDevice : public NimBLEClientCallbacks
{
public:
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
    // Returns true if this device was found
    //
    bool wasFound();

    //
    // Initialize all found devices, returning true if everything succeeded
    //
    static bool initFoundDevices();

    //
    // Connect, probe, and initialize the device.
    // Returns true if successful.
    //
    virtual bool initDevice() = 0;

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
    // If no MAC address is specified, the first scanned device with a matching BLE device name will be used.
    //
    InterestingDevice(const char* name, const char* bleName, const char* macAddr = NULL);

    bool connect(bool refresh = true);

private:
    static std::vector<InterestingDevice*> sAllDevices;
    
    std::string         mUniqueName;
    std::string         mDeviceName;
    const NimBLEAddress mAddress;
    bool                isConnected;

    //
    // These are default implementations for NimBLE callbacks
    //
    virtual void onConnect(NimBLEClient* pClient)  override
        {
        }
    
    virtual void onDisconnect(NimBLEClient* pClient, int reason)  override
        {
        }

    bool onConnParamsUpdateRequest(NimBLEClient* pClient, const ble_gap_upd_params* params)  override
        {
            return true;
        }
};


}
