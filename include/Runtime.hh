// 
// Copyright (c) 2023 ltx4jay@yahoo.com
//
// Licensed under MIT License
//
// The code is provided as-is, with no warranties of any kind. Not suitable for any purpose.
// Provided as an example and exercise in BLE development only.
//

#pragma once

//
// Abstract run-time functions.
//
// A platform-specific implementation must be provided
//

namespace RUNTIME {


//
// Return the current time, in msecs
//
long nowInMs();


//
// Mutex meeting the Lockable requirements
//
class Mutex {
public:
    void lock();
    bool try_lock();
    void unlock();
};

    
}
