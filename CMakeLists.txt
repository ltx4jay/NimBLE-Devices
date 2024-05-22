cmake_minimum_required(VERSION 3.12.4)

idf_component_register(
  INCLUDE_DIRS
    "include"
  REQUIRES
    esp-nimble-cpp
  SRCS
    "src/Nimble-Device.cc"
    "src/AB-Shutter-3.cc"
    "src/Coyote.cc"
    "src/CoyoteV2.cc"
    "src/CoyoteV3.cc"
    "src/Switch.cc"
    "src/Keyboard.cc"
)
