# EnvironmentMeter
SPL/sound level and environment meter. Made to be executed on STM32F4XX "Black Pill" devices.

# Required/recommended sensors
* DFRobot Gravity Analog Sound Level Meter (SEN0232)
* DFRobot Fermion 2.0" 320x240 IPS TFT LCD Display (DFR0664)
  * Any ST7789-based screen should do.
* BME280 temperature, humidity, and atmospheric pressure sensor
* Grove Light Sensor

# Required libraries
*  Adafruit ST7789 v1.9.3
*  Adafruit GFX    v1.11.1
*  BlueDot BME280  v1.0.9

# Development environment
This project was developed using Visual Studio Community Edition and the Visual Micro Arduino IDE for Visual Studio add-in. Code should be usable as-is in Arduino IDE.

# Note for UNO R3:
It might not work since this uses quite a bit of memory. Libraries were changed so it *might* just be small enough to run on such a limited device. It's untested, though.

#  Note for MEGA2560:
SCLK, MOSI, and MISO pins are 52, 51 and 50 respectively. Or use the ICSP header.

# Note for STM32F4XX:
CS, RES, and DC absolutely need to be on pins PA13, PA14, and PA15 respectively. No other pins seems to work. Any of the SPI pins should work for the other connections. I used pins PA5, PA6, and PA7 for SCLK, MISO, and MOSI connections.

Note that the STM32F4xx is 5V-compatible, but has a *3.3V logic*. Reference voltage is **-->3.3V<--**, not 5V.

# Wiring "diagram" for STM32F411CEU6 "Black Pill" devices (using board pins labels):
* ST7789 TFT screen:
  * `VCC`   ->  `5V`
  * `GND`   ->  `GND`
  * `SCLK`  ->  `A5`
  * `MOSI`  ->  `A7`
  * `MISO`  ->  `A6`
  * `CS`    ->  `A13`
  * `RES`   ->  `A14`
  * `DC`    ->  `A15`
  * `BLK`   ->  Not connected
  * `SDCS`  ->  Not connected
 
* Gravity Analog Sound Level Meter
  * `VCC`   ->  `3V3`
  * `GND`   ->  `GND`
  * `SIG`   ->  `A1`

* BME280
  * `VCC`       ->  `3V3`
  * `GND`       ->  `GND`
  * `SDA/MOSI`  ->  `B7`
  * `SCL/SCK`   ->  `B6`
  * `ADDR/MISO` ->  Not connected
  * `CS`        ->  Not connected
 
* Grove Light Sensor
  * `VCC`       ->  `3V3`
  * `GND`       ->  `GND`
  * `SIG`       ->  `A2`
