# Imagotag 2.6/2.2 BWR GL120 hacking

## Chips used

- [CC2510 - MCU + WiFi - 8051](../../raw/master/doc/cc2510.pdf)
- [NT3H2111 - NFC](../../raw/master/doc/NT3H2111_2211.pdf)
- [GDEW026Z39 - 2.6" EPD display](../../raw/master/doc/GDEW026Z39.pdf)
- or [GDEW0213Z16 - 2.13" EPD display](../../raw/master/doc/GDEW026Z39.pdf)
- both displays use [IL0373](../../raw/master/doc/IL0373.pdf) driver
- [TPS61071 - boost converter](../../raw/master/doc/tps61071.pdf) - used (only?) for white LED 
- mysterious 8 pin QFN chip (maybe some security chip?)

## CC2510 connections

### Port 0
- P0:0 - EPD power enable (active low via a P channel fet)
- P0:1 - EPD CS (SPI interface chip select)
- P0:2 - debug port
- P0:3 - EPD SDI (SPI interface serial data in)
- P0:4 - SDA to NFC chip
- P0:5 - EPD CLK (SPI interface clock)
- P0:6 - SCL to NFC chip
- P0:7 - *not connected*

### Port 1
- P1:0 - NFC VCC (seems to power NFC chip and **mysterious chip - pins 3,7,8**)
- P1:1 - NFC FD (field detection)
- P1:2 - EPD DC (data/command select for display)
- P1:3 - EPD BUSY (detect if display busy)
- P1:4 - **mysterious chip pin 1**
- P1:5 - **mysterious chip pin 6**
- P1:6 - **mysterious chip pin 5**
- P1:7 - **mysterious chip pin 3**

### Port 2
- P2:0 - EPD Reset (display reset)
- P2:1 - debug data and white LED
- P2:2 - debug clock and LED boost chip enable (TPS61071)
