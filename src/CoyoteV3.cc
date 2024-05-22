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
#include "CoyoteV3.hh"


using namespace NimBLE::COYOTE;

NimBLE::COYOTE::Device::V3::V3(Device* parent)
: Vx(parent)
{

}


void
NimBLE::COYOTE::Device::V3::setMaxPower(uint8_t A, uint8_t B)
{

}


void
NimBLE::COYOTE::Device::V3::run()
{
}


NimBLE::COYOTE::Device::V3::Channel::Channel(Device *parent, const char* name)
    : COYOTE::Channel(parent, name)
{
}


void
NimBLE::COYOTE::Device::V3::Channel::setWaveform(const COYOTE::V2::Waveform& wave, uint8_t power)
{
}

void
NimBLE::COYOTE::Device::V3::Channel::start(long secs)
{
}

void
NimBLE::COYOTE::Device::V3::Channel::stop()
{
}

void
NimBLE::COYOTE::Device::V3::Channel::startNewWaveform()
{
}

void
NimBLE::COYOTE::Device::V3::Channel::sendNextSegment()
{
}