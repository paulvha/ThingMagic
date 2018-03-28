# version 1.0   initial version  March 2018

Copyright (c) 2017 Paul van Haastrecht <paulvha@hotmail.com>


## Background

Based on a number of questions how to connect a ThingMagic reader with nodes.js, I decided to dust of the old knowledge around Java script,
and work to combine that with the current knowledge around the API.

A working demo has been created on Ubuntu / Raspberry PI Debian. It has been tested on nodejs V 4.2.6 and V 9.9.0 in combination with the
ThingMagic Mercury API 29.4.34. It will read a selected memory bank or all banks plus the ability to provide optional parameters.

Please be aware the node.js is called nodejs in Ubuntu because of potential conflict with other programs.

This has been tested with a M6E Nano connected as a USB device, but there should be no reason why it would not work with an other device
with minor adjustments.

## Usage

Format : IP-address:port?search-string

The search-string options are supported in this release:
    CMD :   READALL, READUSER, READTID, READEPC or READRESERVERD
    TIMEOUT :   value
    RETRY :     value

examples :
192.68.1.160:8080? CMD=READALL
Will read all the memory banks with the default timeout (1000ms) and default retry-count (5)

192.68.1.160:8080? CMD=READEPC,TIMEOUT=500
Will read the EPC memory bank with requested timeout (500ms) and default retry-count (5)

192.68.1.160:8080? CMD=READTID, RETRY=10
Will read the TID memory bank with the default timeout (1000ms) and requested retry-count (10)

192.68.1.160:8080? CMD=READUSER,TIMEOUT=500, RETRY=10
Will read  the USER memory bank with requested timeout (500ms) and requested retry-count (10)

192.68.1.160:8080? CMD=READRESERVERD,TIMEOUT=500, RETRY=0
Will read  the RESERVERD memory bank with requested timeout (500ms) and requested retry-count (0). A retry-count of zero means keep trying until successful.

Special commands:
192.68.1.160:8080? CMD=CANCEL will terminate the current server which will try to restart.


## Software installation

For detailed information about the software, customization and installation, please read the included nodejs_tmr.odt


## License info

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation
files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy,
modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software
is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

