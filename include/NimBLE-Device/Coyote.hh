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

#pragma once


#include "NimBLE-Device.hh"


namespace NimBLE {

namespace COYOTE {

//
// One waveform segment
//
struct WaveVal {
public:
    //
    // Construct a waveform segment with the specified X/Y/Z values, optionally repeated the specified number of times
    //
    WaveVal(uint8_t X, uint16_t Y, uint8_t Z, uint16_t rpt = 1);

    //
    // Construct a waveform segment from snooped values (note: in transmit order)
    //
    WaveVal(const std::vector<uint8_t> vals, uint16_t rpt = 1);

private:
    uint32_t   x    :  5;
    uint32_t   y    : 10;
    uint32_t   z    :  5;
    uint32_t   rsvd : 12;

    uint16_t   repeat;
};


//
// A waveform is a sequence of waveform segments
//
typedef std::vector<WaveVal>  Waveform;


//
// One e-stim channel, A or B
//
class Channel
{
public:
    //
    // Set output power
    //
    void setPower(uint8_t val);

    //
    // Increment/Decrement output power
    //
    void incrementPower(int delta);

    //
    // Get the current output power, as reported by the device
    //
    uint8_t getPower();

    //
    // Subscribe to power change updates (optional)
    //
    void subscribePower(std::function<void(uint8_t, void*)> fct, void* user);

    //
    // Play the specified waveform, at the specified power.
    // If power is not specified, use current power
    //
    void setWaveform(const Waveform& wave, uint8_t power = 0);

    //
    // Start playing the waveform (if any) for the specified number of secs (forever if 0)
    //
    void start(long secs = 0);

    //
    // Stop/Pause playing the waveform (if any)
    //
    void stop();
    

private:
    Channel();

    friend class Device;
};


//
// A Coyote device
//
class Device : public NimBLE::InterestingDevice
{
public:
    //
    // A Coyote device with the specified unique name and optional MAC address
    // If no address is specified, the first one found will match
    //
    Device(const char* uniqueName, const char* macAddr = NULL);
    virtual ~Device();

    //
    // Connect and initialize the device
    //
    bool initDevice()  override;

    //
    // Set the absolute maximum power levels that can be set on each channel.
    // Default is 100
    //
    void setMaxPower(uint8_t A, uint8_t B);

    //
    // Get a reference to each channel interface
    //
    Channel& getChannelA;
    Channel& getChannelB;

    //
    // Subscribe to battery update. Argument is battery level in % (optional)
    //
    void subscribeBattery(std::function<void(uint8_t, void*)> fct, void* user);

    //
    // Service the Coyote
    //
    bool serviceLoop(long nowInMs)  override;

};


//
// Pre-defined DG Labs waveforms
//
namespace DGLABS {

const Waveform GrainTouch = {WaveVal(std::vector<uint8_t>({0xE1, 0x03, 0x00})),
                             WaveVal(std::vector<uint8_t>({0xE1, 0x03, 0x0A})),
                             WaveVal(std::vector<uint8_t>({0xA1, 0x04, 0x0A})),
                             WaveVal(std::vector<uint8_t>({0xC1, 0x05, 0x0A})),
                             WaveVal(std::vector<uint8_t>({0x01, 0x07, 0x00})),
                             WaveVal(std::vector<uint8_t>({0x21, 0x01, 0x0A})),
                             WaveVal(std::vector<uint8_t>({0x61, 0x01, 0x0A})),
                             WaveVal(std::vector<uint8_t>({0xA1, 0x01, 0x0A})),
                             WaveVal(std::vector<uint8_t>({0x01, 0x02, 0x00})),
                             WaveVal(std::vector<uint8_t>({0x01, 0x02, 0x0A})),
                             WaveVal(std::vector<uint8_t>({0x81, 0x02, 0x0A})),
                             WaveVal(std::vector<uint8_t>({0x21, 0x03, 0x0A}))};

//
// High-frequency waform that is modulated when using audio source
//
const Waveform AudioBase = {WaveVal(1, 9, 16)};

}


//
// Some other interesting waveforms. Contributions welcome.
//

namespace LTX4JAY {

const Waveform IntenseVibration = {WaveVal(1, 9, 22)};

const Waveform SlowWave = {WaveVal(1, 26, 8, 2),
                           WaveVal(1, 24, 10),
                           WaveVal(1, 22, 12),
                           WaveVal(1, 20, 14),
                           WaveVal(1, 18, 16),
                           WaveVal(1, 16, 18),
                           WaveVal(1, 16, 22),
                           WaveVal(1, 16, 24),
                           WaveVal(1, 12, 24, 2),
                           WaveVal(1, 16, 24),
                           WaveVal(1, 16, 22),
                           WaveVal(1, 16, 18),
                           WaveVal(1, 18, 16),
                           WaveVal(1, 20, 14),
                           WaveVal(1, 22, 12),
                           WaveVal(1, 24, 10)};


const Waveform MediumWave = {WaveVal(1, 9, 4, 2),
                             WaveVal(1, 9, 6),
                             WaveVal(1, 9, 10),
                             WaveVal(1, 9, 12),
                             WaveVal(1, 9, 17),
                             WaveVal(1, 9, 20, 5),
                             WaveVal(1, 9, 20),
                             WaveVal(1, 9, 17),
                             WaveVal(1, 9, 12),
                             WaveVal(1, 9, 10),
                             WaveVal(1, 9, 6)};

}

}
}

