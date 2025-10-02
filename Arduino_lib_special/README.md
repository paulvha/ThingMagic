# Sparkfun M6e/M7e Nano special libraries

## ===========================================================

Based on different projects and request from different users this
folder contains update to the original Sparkfun Library. The following additions have been done by paulvha

* ************************************************************************************
* ** special for setting powermode() and getTemp(). (February 2020)
* ** added example10 to demonstrate powersaving mode (February  2020)
* ** added example11 and example12 to demonstrate the getTemp() (May 2020)
* ** added support for stream reading temperature (update library and example11) (May 2020)
* ** added readUserDataRegion() and writeUserDataRegion() to read and write starting specific address and length (May2020)
* ** added example13 an example14 to demonstate the new xxxxUserDataRegion calls (May2020)
* ** added example15 to demonstrate new feature : reading all the memory banks in one command (August 2020)
* ** added example16 to demonstrate new feature : reading a single memory bank and EPC in continuous mode (September 2020)
* ** added example17 to demonstrate new feature : reading and writting Procotol Word (PC) and EPC (February 2021)
* ** update example13 and example14 to demonstrate new feature : to demonstate the USER new xxxxDataRegion calls (Feb2021)
* ** added example18 and example19 to demonstrate new feature : to demonstate the EPC new xxxxDataRegion calls (Feb2021)
* ** update to example15 and correct TID memory offset (Feb2021)
* ** updated examples with better Setupnano() (May2021)
* ** updated library for an apperently needed delay on an ESP32 (may2021)
* ** update code due to warnings/errors discovered by optimised  /stricter compilers (Jan2022)
* ** update code to resolve a bug where scanning was not shown in the examples(Jan2022)
* ** added extra call, structure  and example20 to select a specific TAG and read data from a specified bank, specified offset and length (Dec2022)
* ** change EPC[12] to TMR_EPC[12] due to conflict in ESP32  2.0.6 library (march 2023)
* ** Tested on UNO-R4 Wifi (see below) (Oct2023)
* ** added example21: does the same as example1 but added name lookup based on the EPC read (nov2023)
* ** updated SRC-library for the M7E support and aligned with recent Sparkfun changes (June2024)
* ** added getTagPhase()call and example22 to demonstrate (Sept 2024)
* ** July2025:
* ** added 5 new GEN2 calls: setGen2Session(), setGen2RFmode(), setGen2RFTarget(), setGen2Encoding(), setGen2Q()
* ** added example30 and example31 to demonstrate the new functions.
* ** updated all examples and parts of source code for stability
* ** update to setPowerMode(uint8_t mode) to enable SLEEP on M7E
* ************************************************************************************

## Nano M7E replaces M6E (June 2024))
ThingMagic/Jadak is phasing out the M6E range and provides a new range M7E. The M7E HECTO is the replacement for the M6E NANO and is backward compatible.
The M7E promises a lower popwer consumption and some other changes all of them have minor impact on the Sparkfun implementation.<br>
Sparkfun has launched an update to the earlier library (V1.2.0)), with mainly cosmetic changes.
<br>There 2 changes to call out :
- 1. By default the library assumes your board is an M6E_Nano and in case you set for region REGION_NORTHAMERICA it will use REGION_NORTHAMERICA2 as is how it was. If you really want to use REGION_NORTHAMERICA with an M7E, change nano.begin(NanoSerial) to nano.begin(NanoSerial,ThingMagic_M7E_HECTO);
- 2. This library was already supporting setting the GPIO's on the M6E Nano since August 2019. Starting the new Sparkfun library they have added this option as well with different calls. I have included those in this library, included their example9. I also kept the earlier ones for backward compatibility.

None of your existing sketches needs a change in order to work with the updated library.

## Nano M6E on UNO-R4 (October 2023)
Note: This has been tested on an UNO-R4 Wifi and works with the following remarks.

When selecting for “Serial1” as the NanoSerial communication port and connecting the M6E with the switch “UART HW”. It will work without modifications. It will use pin 0 as RX and pin 1 as TX.

Two CHALLENGES come when selecting SOFTWARESERIAL as the NanoSerial communication port. Make sure to set the switch to "UART-SW".
The following modifications are needed :

### Challenge-1:
This is within the sketch within the function “setupNano(long baudRate)” . The SoftwareSerial library of the UNO R4 does have a boolean function:  while (!NanoSerial).
WorkAround: Replace the “while (!NanoSerial);” with a “delay(1000);”

### Challenge-2:
A problem with UNO-R4 the SoftwareSerial in the library (version 1.0.4) prevents a change a baudrate after an initial baudrate has been set.
In the function setupNano() it first tries to connect with the wanted baudrate. If that fails it will assume the M6E is still at the default 115200. It will set the NanoSerial to 115200, send an instruction to the M6E to set the wanted baudrate, and reset NanoSerial to the wanted baudrate. Now it will attempt again. As changing the baudrate does not work it will fail to connect.
WorkAround: In setup() call as setupNano(115200).<br>
Update: A solution to this problem in SoftwareSerial has been posted on ![ArduinoForum]( https://forum.arduino.cc/t/softwareserial-no-baudrate-change-possible/1179988/6)

## Below the orginal README

SparkFun Simultaneous RFID Tag Reader Library
===========================================================

![SparkFun Simultaneous RFID Tag Reader - NANO M6E](https://cdn.sparkfun.com//assets/parts/1/1/9/1/6/14066-06a.jpg)

[*SparkFun Simultaneous RFID Tag Reader - NANO M6E (SEN-14066)*](https://www.sparkfun.com/products/14066)

This is a powerful shield capable of reading multiple RFID tags at the same time. It can also write to tags. Works with UHF Gen2 RFID tags.

Documentation
--------------

* **[Hookup Guide](https://learn.sparkfun.com/tutorials/simultaneous-rfid-tag-reader-hookup-guide)** - Hookup guide for the RFID shield.
* **[Installing an Arduino Library Guide](https://learn.sparkfun.com/tutorials/installing-an-arduino-library)** - Basic information on how to install an Arduino library.
* **[Product Repository](https://github.com/sparkfun/Simultaneous_RFID_Tag_Reader)** - Main repository (including hardware files) for the RFID Shield.

License Information
-------------------

This product is _**open source**_!

The **code** is beerware; if you see me (or any other SparkFun employee) at the local, and you've found our code helpful, please buy us a round!

Please use, reuse, and modify these files as you see fit. Please maintain attribution to SparkFun Electronics and release anything derivative under the same license.

Distributed as-is; no warranty is given.

- Your friends at SparkFun.
