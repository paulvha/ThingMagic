# GPIO support Simultaneous RFID tag reader

## ===========================================================

A document and supporting code provides an enhancement to the Sparkfun
library to set and read the GPIO on the Simultaneous RFID tag reader.

<br> A detailed description of the options and findings are in gpio_RFID.odt

## Getting Started
Sparkfun has created an Simultaneous RFID tag reader, based on the M6e Nano
from Jadak (formally ThingMagic). (https://www.sparkfun.com/products/14066)

Sparkfun has also created a library for the Arduino-like micro-boards which
is a scaled down version based on the official Mercury API from Jadak.
(https://github.com/sparkfun/SparkFun_Simultaneous_RFID_Tag_Reader_Library)

While this Sparkfun library works stable and good, it is lacking a number
of options, including the ability to control the GPIO’s, whic is added with
this extension.

These GPIOs are broken out on the board and GPIO01, LV2, LV3 and LV4.

## CAUTION WARNING CAUTION WARNING
Based onthe hardware design manual of the M6e Nano:

Please be aware and remind yourself that on the Nano GPIO1 and the LV2,
LV3, and LV4 pins are 3.3V only.

Each line configurable as input or output interface by default it is an
input with internal pull-down, with a resistance value of between 20 and 60 kOhms (40 kOhms nominal).

The GPIO pins should connect through 1 kOhm resistors to the module to
ensure the input Voltage limits are maintained even if the module is shut off.

Lines configured as inputs must be low whenever the module is turned
off and low at the time the module is turned on.

## Prerequisites
Sparkfun library https://github.com/sparkfun/SparkFun_Simultaneous_RFID_Tag_Reader_Library

## Software installation
A detailed description installation description is in gpio_RFID.

## Program usage
### Program options
Please see the description in the documentation (odt) and example sketch

## Versioning

### version 1.0 / August 2019
 * Initial version

## Author
 * Paul van Haastrecht (paulvha@hotmail.com)

## License
This project is licensed under the GNU GENERAL PUBLIC LICENSE 3.0

## Acknowledgements
Mercury API & programming guide


