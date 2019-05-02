# Connecting RFID Simultaneous reader with Arduino or ESP32

## ===========================================================

An investigation and solution to connect the RFID reader to an
Arduino or ESP32 Thing and control with a remote program that has
been written with the MercuryAPI from Jadak. Succesfully tested against
the URA and DEMO application running on Windows.
<br> A detailed description of the options and findings are in Arduin-Nano.odt

## Getting Started
To connect a SparkFun Simultaneous RFID Reader - M6E Nano (SEN-14066) to an application
running on Linux or Windows (like the Universal Reader Assistant  or URA)
you need a separate USB to Serial converter.
<br>I wondered why an Arduino or ESP32 Thing could not be used as many people
tried that (including me) and failed.

<br>First the good news after analyzing and testing: it CAN be used. !  BUT you need to follow some steps in order to achieve that.

## Prerequisites
You must have installed the Sparkfun RFID Simultaneous reader library:
https://github.com/sparkfun/SparkFun_Simultaneous_RFID_Tag_Reader_Library/archive/master.zip

## Software installation
Obtain the pass_through.ino and start in Arduino IDE

## Program usage
### Program options
Please see the description in the top of the sketch and read the documentation (odt)

## Versioning
###  November 2018
 * initial version

###  April 2019
 * added detailed documentation
 * added support for baudrate change
 * added blocking reset signal
 * added support for ESP32 Thing Sparkfun
 * tested with URA and demo running from Windows PC

## Author
 * Paul van Haastrecht (paulvha@hotmail.com)

## License
This project is licensed under the GNU GENERAL PUBLIC LICENSE 3.0

## Acknowledgements
Make sure to read the Sparkfun Hookup guide
https://learn.sparkfun.com/tutorials/simultaneous-rfid-tag-reader-hookup-guide/introduction


