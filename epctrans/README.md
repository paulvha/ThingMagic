# version 1.0	initial version

Copyright (c) 2017 Paul van Haastrecht <paulvha@hotmail.com>

## Highlevel description

EPC translation : This software will encode and decode between binary 
EPC, Tag encoding, Pure idendity, barcode / GTIN as defined by the EPC
Tag Data Standard  version 1.9.

Following EPC Tags have been tested with (included)examples: 
sgcen-96, sgtin-96, sgtin-198, cpi-96,   cpi-var,  gid-96,  adi-var,
gdti-96,  gdti-113, gdti-174,  gsrnp-96, gsrn-96,  sscc-96, sgln-96,
sgln-195, grai-96,  grai-175,  giai-96,  giai-202, usdod-96, 

This software has the following routines
Decoding:
  From binary EPC to tag-encoding URI 
  From tag-encoding URI to pure-identity URI
  From binary EPC to pure-identity URI 
  From tag-encoding URI or Pure-Identity URI to barcode (autodetect)
  From binary EPC to barcode
  From Tag-encoding URI or Pure-Identity URI to ONS URL (autodetect) 

Encoding:
  From barcode to Pure-Identity 
  From pure-identity to tag-encoding URI 
  From tag-encoding to binary 
  From pure-identity to binary 
  From Tag_encoding or Pure-Identity URI to binary (autodetect) 
  
## Software usage

epctrans.h and epctrans.c contain the translation routines.
epc_examples.c show how to use the translation routines.

Copy all into the same directory. 

To compile : cc -o epcx epc_examples.c epctrans.c -lm

To run ./epcx [ -t epc_number] [-h] [-n]
-h   : help
-n   : no colour in output
-t # : select EPC format on command line instead of interactive.
