// ===========================================================================
//  camera_pins.h  —  Camera pin map for the ESP32-S3-CAM
// ---------------------------------------------------------------------------
//  These are the pins that connect the ESP32-S3 to the on-board camera.
//
//  >>> CONFIRMED <<<  This map is taken from the FORIOT ESP32-S3-CAM silkscreen
//  pinout and has been verified on real hardware, so there's no need to guess.
//  If you use a different ESP32-S3-CAM board, double-check these against its
//  own pinout before flashing.
// ===========================================================================
#pragma once

#define PWDN_GPIO_NUM    -1   // not connected on this board
#define RESET_GPIO_NUM   -1   // not connected on this board
#define XCLK_GPIO_NUM    15
#define SIOD_GPIO_NUM     4   // camera I2C data (SCCB SDA)
#define SIOC_GPIO_NUM     5   // camera I2C clock (SCCB SCL)

#define Y9_GPIO_NUM      16
#define Y8_GPIO_NUM      17
#define Y7_GPIO_NUM      18
#define Y6_GPIO_NUM      12
#define Y5_GPIO_NUM      10
#define Y4_GPIO_NUM       8
#define Y3_GPIO_NUM       9
#define Y2_GPIO_NUM      11

#define VSYNC_GPIO_NUM    6
#define HREF_GPIO_NUM     7
#define PCLK_GPIO_NUM    13
