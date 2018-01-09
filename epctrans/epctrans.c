/**
 *  Copyright (c) 2017 Paul van Haastrecht.
 * 
 * version 1.0 / november 2017
 * 		initial version
 * 
 * 
 * EPC translation: This software will encode and decode between binary 
 * EPC, Tag encoding, pure Idendity, barcode / GTIN as defined by the EPC
 * Tag Data Standard  version 1.9. /  Ractified november 2014.
 * 
 * It can handle strings, integer, partinteger, partstrings, unpadded partition,
 * cage, numstrings, 6-bit strings, 6-bit partstrings. NOT all has been deeply
 * tested yet however.
 *
 * The following EPC Tags have been tested with samples:  (encoding / decoding etc) : 
 *  sgcen-96, sgtin-96, sgtin-198, cpi-96,   cpi-var,  gid-96,  adi-var,
 *  gdti-96,  gdti-113, gdti-174,  gsrnp-96, gsrn-96,  sscc-96, sgln-96,
 *  sgln-195, grai-96,  grai-175,  giai-96,  giai-202, usdod-96, 
 * 
 * adi-var, usdod-96 and gid-96 require special handling, which is implemented 
 * hardcoded on many places, the others follow the partition table
 * 
 * sgtin code can be translated to ONS (Object Name Service) for online lookup
 * This can be tested with dig -t NAPTR ONS-string. NO testing to a life service 
 * has been performed.
 *  
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "epctrans.h"

struct epcpart part_table[200] =
{
 { 0x0,"not programmed", 0, 0, 0, 0 , 0,"", 0, 0, "",'0', 0, 0, "",'0'},
 { 0x8,"Reserved until 64bit Sunset <SSCC-64>", 0, 0, 0, 0 , 0,"", 0, 0, "",'0', 0, 0, "",'0'},
 { 0x9,"Reserved until 64bit Sunset <SGLN-64>", 0, 0, 0, 0 , 0,"", 0, 0, "",'0', 0, 0, "",'0'},
 { 0xa,"Reserved until 64bit Sunset <GRAI-64>", 0, 0, 0, 0 , 0,"", 0, 0, "",'0', 0, 0, "",'0'},
 { 0xb,"Reserved until 64bit Sunset <GIAI-64>", 0, 0, 0, 0 , 0,"", 0, 0, "",'0', 0, 0, "",'0'},

 { 0x2c,"gdti-96", 96, 3, 0, 40 , 12,"GS1 company", 1,  0, "Document Type",'I', 41, 0x0, "serial",'i'},
 { 0x2c,"gdti-96", 96, 3, 1, 37 , 11,"GS1 company", 4,  1, "Document Type",'I', 41, 0x0, "serial",'i'},
 { 0x2c,"gdti-96", 96, 3, 2, 34 , 10,"GS1 company", 7,  2, "Document Type",'I', 41, 0x0, "serial",'i'},
 { 0x2c,"gdti-96", 96, 3, 3, 30 , 9, "GS1 company", 11, 3, "Document Type",'I', 41, 0x0, "serial",'i'},
 { 0x2c,"gdti-96", 96, 3, 4, 27 , 8, "GS1 company", 14, 4, "Document Type",'I', 41, 0x0, "serial",'i'},
 { 0x2c,"gdti-96", 96, 3, 5, 24 , 7, "GS1 company", 17, 5, "Document Type",'I', 41, 0x0, "serial",'i'},
 { 0x2c,"gdti-96", 96, 3, 6, 20 , 6, "GS1 company", 21, 6, "Document Type",'I', 41, 0x0, "serial",'i'},
 
 { 0x2d,"gsrn-96", 96, 3, 0, 40 , 12,"GS1 company", 18, 5, "Serial Reference",'I', 24, 0, "Reserved (zero)",'R'}, 
 { 0x2d,"gsrn-96", 96, 3, 1, 37 , 11,"GS1 company", 21, 6, "Serial Reference",'I', 24, 0, "Reserved (zero)",'R'},
 { 0x2d,"gsrn-96", 96, 3, 2, 34 , 10,"GS1 company", 24, 7, "Serial Reference",'I', 24, 0, "Reserved (zero)",'R'},
 { 0x2d,"gsrn-96", 96, 3, 3, 30 , 9, "GS1 company", 28, 8, "Serial Reference",'I', 24, 0, "Reserved (zero)",'R'},
 { 0x2d,"gsrn-96", 96, 3, 4, 27 , 8, "GS1 company", 31, 9, "Serial Reference",'I', 24, 0, "Reserved (zero)",'R'},
 { 0x2d,"gsrn-96", 96, 3, 5, 24 , 7, "GS1 company", 34, 10,"Serial Reference",'I', 24, 0, "Reserved (zero)",'R'},
 { 0x2d,"gsrn-96", 96, 3, 6, 20 , 6, "GS1 company", 38, 11,"Serial Reference",'I', 24, 0, "Reserved (zero)",'R'},

 { 0x2e,"gsrnp-96", 96, 3, 0, 40 , 12,"GS1 company", 18, 5, "Serial Reference",'I', 24, 0, "Reserved (zero)",'R'}, 
 { 0x2e,"gsrnp-96", 96, 3, 1, 37 , 11,"GS1 company", 21, 6, "Serial Reference",'I', 24, 0, "Reserved (zero)",'R'},
 { 0x2e,"gsrnp-96", 96, 3, 2, 34 , 10,"GS1 company", 24, 7, "Serial Reference",'I', 24, 0, "Reserved (zero)",'R'},
 { 0x2e,"gsrnp-96", 96, 3, 3, 30 , 9, "GS1 company", 28, 8, "Serial Reference",'I', 24, 0, "Reserved (zero)",'R'},
 { 0x2e,"gsrnp-96", 96, 3, 4, 27 , 8, "GS1 company", 31, 9, "Serial Reference",'I', 24, 0, "Reserved (zero)",'R'},
 { 0x2e,"gsrnp-96", 96, 3, 5, 24 , 7, "GS1 company", 34, 10,"Serial Reference",'I', 24, 0, "Reserved (zero)",'R'},
 { 0x2e,"gsrnp-96", 96, 3, 6, 20 , 6, "GS1 company", 38, 11,"Serial Reference",'I', 24, 0, "Reserved (zero)",'R'},

 // special one (has been implemented hardcoded
 { 0x2f,"usdod-96", 0, 0, 0, 0 , 0,"", 0, 0, "",'0', 0, 0, "",'0'},
  
 { 0x30,"sgtin-96", 96, 3, 0, 40 , 12,"GS1 company", 4,  1, "Item Reference",'I', 38, 0x0, "serial",'i'},
 { 0x30,"sgtin-96", 96, 3, 1, 37 , 11,"GS1 company", 7,  2, "Item Reference",'I', 38, 0x0, "serial",'i'},
 { 0x30,"sgtin-96", 96, 3, 2, 34 , 10,"GS1 company", 10, 3, "Item Reference",'I', 38, 0x0, "serial",'i'},
 { 0x30,"sgtin-96", 96, 3, 3, 30 , 9, "GS1 company", 14, 4, "Item Reference",'I', 38, 0x0, "serial",'i'},
 { 0x30,"sgtin-96", 96, 3, 4, 27 , 8, "GS1 company", 17, 5, "Item Reference",'I', 38, 0x0, "serial",'i'},
 { 0x30,"sgtin-96", 96, 3, 5, 24 , 7, "GS1 company", 20, 6, "Item Reference",'I', 38, 0x0, "serial",'i'},
 { 0x30,"sgtin-96", 96, 3, 6, 20 , 6, "GS1 company", 24, 7, "Item Reference",'I', 38, 0x0, "serial",'i'},
 
 { 0x31,"sscc-96", 96, 3, 0, 40 , 12,"GS1 company", 18, 5, "Serial Reference",'I', 24, 0, "Reserved (zero)",'R'}, 
 { 0x31,"sscc-96", 96, 3, 1, 37 , 11,"GS1 company", 21, 6, "Serial Reference",'I', 24, 0, "Reserved (zero)",'R'},
 { 0x31,"sscc-96", 96, 3, 2, 34 , 10,"GS1 company", 24, 7, "Serial Reference",'I', 24, 0, "Reserved (zero)",'R'},
 { 0x31,"sscc-96", 96, 3, 3, 30 , 9, "GS1 company", 28, 8, "Serial Reference",'I', 24, 0, "Reserved (zero)",'R'},
 { 0x31,"sscc-96", 96, 3, 4, 27 , 8, "GS1 company", 31, 9, "Serial Reference",'I', 24, 0, "Reserved (zero)",'R'},
 { 0x31,"sscc-96", 96, 3, 5, 24 , 7, "GS1 company", 34, 10,"Serial Reference",'I', 24, 0, "Reserved (zero)",'R'},
 { 0x31,"sscc-96", 96, 3, 6, 20 , 6, "GS1 company", 38, 11,"Serial Reference",'I', 24, 0, "Reserved (zero)",'R'},

 { 0x32,"sgln-96", 96, 3, 0, 40 , 12,"GS1 company", 1, 0, "Location Reference",'I', 41, 0x0, "Extension",'i'},
 { 0x32,"sgln-96", 96, 3, 1, 37 , 11,"GS1 company", 3, 1, "Location Reference",'I', 41, 0x0, "Extension",'i'},
 { 0x32,"sgln-96", 96, 3, 2, 34 , 10,"GS1 company", 7, 2, "Location Reference",'I', 41, 0x0, "Extension",'i'},
 { 0x32,"sgln-96", 96, 3, 3, 30 , 9, "GS1 company", 11, 3,"Location Reference",'I', 41, 0x0, "Extension",'i'},
 { 0x32,"sgln-96", 96, 3, 4, 27 , 8, "GS1 company", 14, 4,"Location Reference",'I', 41, 0x0, "Extension",'i'},
 { 0x32,"sgln-96", 96, 3, 5, 24 , 7, "GS1 company", 17, 5,"Location Reference",'I', 41, 0x0, "Extension",'i'},
 { 0x32,"sgln-96", 96, 3, 6, 20 , 6, "GS1 company", 21, 6,"Location Reference",'I', 41, 0x0, "Extension",'i'},
 
 { 0x33,"grai-96", 96, 3, 0, 40 , 12,"GS1 company", 4,  0, "Asset Type",'I', 38, 0x0, "serial",'i'},
 { 0x33,"grai-96", 96, 3, 1, 37 , 11,"GS1 company", 7,  1, "Asset Type",'I', 38, 0x0, "serial",'i'},
 { 0x33,"grai-96", 96, 3, 2, 34 , 10,"GS1 company", 10, 2, "Asset Type",'I', 38, 0x0, "serial",'i'},
 { 0x33,"grai-96", 96, 3, 3, 30 , 9, "GS1 company", 14, 3, "Asset Type",'I', 38, 0x0, "serial",'i'},
 { 0x33,"grai-96", 96, 3, 4, 27 , 8, "GS1 company", 17, 4, "Asset Type",'I', 38, 0x0, "serial",'i'},
 { 0x33,"grai-96", 96, 3, 5, 24 , 7, "GS1 company", 20, 5, "Asset Type",'I', 38, 0x0, "serial",'i'},
 { 0x33,"grai-96", 96, 3, 6, 20 , 6, "GS1 company", 24, 6, "Asset Type",'I', 38, 0x0, "serial",'i'},

 { 0x34,"giai-96", 96, 3, 0, 40 , 12,"GS1 company", 42, 13, "Individual Asset Reference",'U', 0, 0x0, "",'0'},
 { 0x34,"giai-96", 96, 3, 1, 37 , 11,"GS1 company", 45, 14, "Individual Asset Reference",'U', 0, 0x0, "",'0'},
 { 0x34,"giai-96", 96, 3, 2, 34 , 10,"GS1 company", 48, 15, "Individual Asset Reference",'U', 0, 0x0, "",'0'},
 { 0x34,"giai-96", 96, 3, 3, 30 , 9, "GS1 company", 52, 16, "Individual Asset Reference",'U', 0, 0x0, "",'0'},
 { 0x34,"giai-96", 96, 3, 4, 27 , 8, "GS1 company", 55, 17, "Individual Asset Reference",'U', 0, 0x0, "",'0'},
 { 0x34,"giai-96", 96, 3, 5, 24 , 7, "GS1 company", 58, 18, "Individual Asset Reference",'U', 0, 0x0, "",'0'},
 { 0x34,"giai-96", 96, 3, 6, 20 , 6, "GS1 company", 62, 19, "Individual Asset Reference",'U', 0, 0x0, "",'0'},
 
 // special one (has been implemented hardcoded)
 { 0x35,"gid-96", 0, 0, 0, 0, 0,"", 0, 0, "",'i', 36, 0x0, "Serial Number",'i'},
  
 { 0x36,"sgtin-198", 198, 3, 0, 40 , 12,"GS1 company", 4,  1, "Item Reference",'I', 140, 0x0, "serial",'s'},
 { 0x36,"sgtin-198", 198, 3, 1, 37 , 11,"GS1 company", 7,  2, "Item Reference",'I', 140, 0x0, "serial",'s'},
 { 0x36,"sgtin-198", 198, 3, 2, 34 , 10,"GS1 company", 10, 3, "Item Reference",'I', 140, 0x0, "serial",'s'},
 { 0x36,"sgtin-198", 198, 3, 3, 30 , 9, "GS1 company", 14, 4, "Item Reference",'I', 140, 0x0, "serial",'s'},
 { 0x36,"sgtin-198", 198, 3, 4, 27 , 8, "GS1 company", 17, 5, "Item Reference",'I', 140, 0x0, "serial",'s'},
 { 0x36,"sgtin-198", 198, 3, 5, 24 , 7, "GS1 company", 20, 6, "Item Reference",'I', 140, 0x0, "serial",'s'},
 { 0x36,"sgtin-198", 198, 3, 6, 20 , 6, "GS1 company", 24, 7, "Item Reference",'I', 140, 0x0, "serial",'s'},
 
 { 0x37,"grai-170", 170, 3, 0, 40 , 12,"GS1 company", 4,  0, "Asset Type",'I', 112, 0x0, "serial",'s'},
 { 0x37,"grai-170", 170, 3, 1, 37 , 11,"GS1 company", 7,  1, "Asset Type",'I', 112, 0x0, "serial",'s'},
 { 0x37,"grai-170", 170, 3, 2, 34 , 10,"GS1 company", 10, 2, "Asset Type",'I', 112, 0x0, "serial",'s'},
 { 0x37,"grai-170", 170, 3, 3, 30 , 9, "GS1 company", 14, 3, "Asset Type",'I', 112, 0x0, "serial",'s'},
 { 0x37,"grai-170", 170, 3, 4, 27 , 8, "GS1 company", 17, 4, "Asset Type",'I', 112, 0x0, "serial",'s'},
 { 0x37,"grai-170", 170, 3, 5, 24 , 7, "GS1 company", 20, 5, "Asset Type",'I', 112, 0x0, "serial",'s'},
 { 0x37,"grai-170", 170, 3, 6, 20 , 6, "GS1 company", 24, 6, "Asset Type",'I', 112, 0x0, "serial",'s'},

 { 0x38,"giai-202", 202, 3, 0, 40 , 12,"GS1 company", 148, 18, "Individual Asset Reference",'S', 0, 0x0, "",'0'},
 { 0x38,"giai-202", 202, 3, 1, 37 , 11,"GS1 company", 151, 19, "Individual Asset Reference",'S', 0, 0x0, "",'0'},
 { 0x38,"giai-202", 202, 3, 2, 34 , 10,"GS1 company", 154, 20, "Individual Asset Reference",'S', 0, 0x0, "",'0'},
 { 0x38,"giai-202", 202, 3, 3, 30 , 9, "GS1 company", 158, 21, "Individual Asset Reference",'S', 0, 0x0, "",'0'},
 { 0x38,"giai-202", 202, 3, 4, 27 , 8, "GS1 company", 161, 22, "Individual Asset Reference",'S', 0, 0x0, "",'0'},
 { 0x38,"giai-202", 202, 3, 5, 24 , 7, "GS1 company", 164, 23, "Individual Asset Reference",'S', 0, 0x0, "",'0'},
 { 0x38,"giai-202", 202, 3, 6, 20 , 6, "GS1 company", 168, 24, "Individual Asset Reference",'S', 0, 0x0, "",'0'},
 
 { 0x39,"sgln-195", 195, 3, 0, 40 , 12,"GS1 company", 1,  0, "Location Reference",'I', 140, 0x0, "Extension",'s'},
 { 0x39,"sgln-195", 195, 3, 1, 37 , 11,"GS1 company", 3,  1, "Location Reference",'I', 140, 0x0, "Extension",'s'},
 { 0x39,"sgln-195", 195, 3, 2, 34 , 10,"GS1 company", 7,  2, "Location Reference",'I', 140, 0x0, "Extension",'s'},
 { 0x39,"sgln-195", 195, 3, 3, 30 , 9, "GS1 company", 11, 3, "Location Reference",'I', 140, 0x0, "Extension",'s'},
 { 0x39,"sgln-195", 195, 3, 4, 27 , 8, "GS1 company", 14, 4, "Location Reference",'I', 140, 0x0, "Extension",'s'},
 { 0x39,"sgln-195", 195, 3, 5, 24 , 7, "GS1 company", 17, 5, "Location Reference",'I', 140, 0x0, "Extension",'s'},
 { 0x39,"sgln-195", 195, 3, 6, 20 , 6, "GS1 company", 21, 6, "Location Reference",'I', 140, 0x0, "Extension",'s'},
 
 { 0x3a,"gdti-113", 113, 3, 0, 40 , 12,"GS1 company", 1,  0, "Document Type",'I', 58, 0x0, "serial",'n'},
 { 0x3a,"gdti-113", 113, 3, 1, 37 , 11,"GS1 company", 4,  1, "Document Type",'I', 58, 0x0, "serial",'n'},
 { 0x3a,"gdti-113", 113, 3, 2, 34 , 10,"GS1 company", 7,  2, "Document Type",'I', 58, 0x0, "serial",'n'},
 { 0x3a,"gdti-113", 113, 3, 3, 30 , 9, "GS1 company", 11, 3, "Document Type",'I', 58, 0x0, "serial",'n'},
 { 0x3a,"gdti-113", 113, 3, 4, 27 , 8, "GS1 company", 14, 4, "Document Type",'I', 58, 0x0, "serial",'n'},
 { 0x3a,"gdti-113", 113, 3, 5, 24 , 7, "GS1 company", 17, 5, "Document Type",'I', 58, 0x0, "serial",'n'},
 { 0x3a,"gdti-113", 113, 3, 6, 20 , 6, "GS1 company", 21, 6, "Document Type",'I', 58, 0x0, "serial",'n'},
 
 // special one (has been implemented hardcoded)
 { 0x3b,"adi-var", 0, 6, 0, 0 , 0, "CAGE/ DoDAAC", 0, 0, "Part Number",'d', 0, 0, "serial",'d'},
 
 { 0x3c,"cpi-96", 96, 3, 0, 40 , 12,"GS1 company", 11, 3, "Component/Part Reference",'U', 31, 0x0, "serial",'i'},
 { 0x3c,"cpi-96", 96, 3, 1, 37 , 11,"GS1 company", 14, 4, "Component/Part Reference",'U', 31, 0x0, "serial",'i'},
 { 0x3c,"cpi-96", 96, 3, 2, 34 , 10,"GS1 company", 17, 5, "Component/Part Reference",'U', 31, 0x0, "serial",'i'},
 { 0x3c,"cpi-96", 96, 3, 3, 30 , 9, "GS1 company", 21, 6, "Component/Part Reference",'U', 31, 0x0, "serial",'i'},
 { 0x3c,"cpi-96", 96, 3, 4, 27 , 8, "GS1 company", 24, 7, "Component/Part Reference",'U', 31, 0x0, "serial",'i'},
 { 0x3c,"cpi-96", 96, 3, 5, 24 , 7, "GS1 company", 27, 8, "Component/Part Reference",'U', 31, 0x0, "serial",'i'},
 { 0x3c,"cpi-96", 96, 3, 6, 20 , 6, "GS1 company", 31, 9, "Component/Part Reference",'U', 31, 0x0, "serial",'i'},

 { 0x3d,"cpi-var", 1, 3, 0, 40 , 12,"GS1 company", 114, 18, "Component/Part Reference",'D', 40, 0x0, "serial",'i'},
 { 0x3d,"cpi-var", 1, 3, 1, 37 , 11,"GS1 company", 120, 19, "Component/Part Reference",'D', 40, 0x0, "serial",'i'},
 { 0x3d,"cpi-var", 1, 3, 2, 34 , 10,"GS1 company", 126, 20, "Component/Part Reference",'D', 40, 0x0, "serial",'i'},
 { 0x3d,"cpi-var", 1, 3, 3, 30 , 9, "GS1 company", 132, 21, "Component/Part Reference",'D', 40, 0x0, "serial",'i'},
 { 0x3d,"cpi-var", 1, 3, 4, 27 , 8, "GS1 company", 138, 22, "Component/Part Reference",'D', 40, 0x0, "serial",'i'},
 { 0x3d,"cpi-var", 1, 3, 5, 24 , 7, "GS1 company", 144, 23, "Component/Part Reference",'D', 40, 0x0, "serial",'i'},
 { 0x3d,"cpi-var", 1, 3, 6, 20 , 6, "GS1 company", 150, 24, "Component/Part Reference",'D', 40, 0x0, "serial",'i'},
 
 { 0x3e,"gdti-174", 174, 3, 0, 40 , 12,"GS1 company", 1,  0, "Document Type",'I', 119, 0x0, "serial",'s'},
 { 0x3e,"gdti-174", 174, 3, 1, 37 , 11,"GS1 company", 4,  1, "Document Type",'I', 119, 0x0, "serial",'s'},
 { 0x3e,"gdti-174", 174, 3, 2, 34 , 10,"GS1 company", 7,  2, "Document Type",'I', 119, 0x0, "serial",'s'},
 { 0x3e,"gdti-174", 174, 3, 3, 30 , 9, "GS1 company", 11, 3, "Document Type",'I', 119, 0x0, "serial",'s'},
 { 0x3e,"gdti-174", 174, 3, 4, 27 , 8, "GS1 company", 14, 4, "Document Type",'I', 119, 0x0, "serial",'s'},
 { 0x3e,"gdti-174", 174, 3, 5, 24 , 7, "GS1 company", 17, 5, "Document Type",'I', 119, 0x0, "serial",'s'},
 { 0x3e,"gdti-174", 174, 3, 6, 20 , 6, "GS1 company", 21, 6, "Document Type",'I', 119, 0x0, "serial",'s'},

 { 0x3f,"sgcn-96", 96, 3, 0, 40 , 12,"GS1 company", 1,  0, "Coupon Reference",'I', 41, 0x0, "serial",'n'},
 { 0x3f,"sgcn-96", 96, 3, 1, 37 , 11,"GS1 company", 3,  1, "Coupon Reference",'I', 41, 0x0, "serial",'n'},
 { 0x3f,"sgcn-96", 96, 3, 2, 34 , 10,"GS1 company", 7,  2, "Coupon Reference",'I', 41, 0x0, "serial",'n'},
 { 0x3f,"sgcn-96", 96, 3, 3, 30 , 9, "GS1 company", 11, 3, "Coupon Reference",'I', 41, 0x0, "serial",'n'},
 { 0x3f,"sgcn-96", 96, 3, 4, 27 , 8, "GS1 company", 14, 4, "Coupon Reference",'I', 41, 0x0, "serial",'n'},
 { 0x3f,"sgcn-96", 96, 3, 5, 24 , 7, "GS1 company", 17, 5, "Coupon Reference",'I', 41, 0x0, "serial",'n'},
 { 0x3f,"sgcn-96", 96, 3, 6, 20 , 6, "GS1 company", 21, 6, "Coupon Reference",'I', 41, 0x0, "serial",'n'},  
  
 { 0xff,"END" , 0, 0, 0, 0 , 0,"", 0, 0, "",'0', 0, 0, "",'0'},
};


// store bits before adding as byte during encoding 
uint8_t epc_byte = 0;

// offset in binary EPC during encoding & decoding
int epc_pos = 0;

// bit buffer during decoding
uint16_t bitval;

// # valid bits during encoding & decoding
int	 bitcnt = 0;

/**
 * Find  case insensative
 * 
 * str1 = haystack
 * str2 = needle
 * 
 * return :
 * 0 = no match
 * OK pointer to start of match
 */

char* stristr( const char* haystack, const char* needle )
{
    const char* p1 = haystack ;
    const char* p2 = needle ;
    const char* r = *p2 == 0 ? haystack : 0 ;

    // as long as not end of string(s)
    while( *p1 != 0 && *p2 != 0 )
    {
        // if same character
        if( tolower( (unsigned char)*p1 ) == tolower( (unsigned char)*p2 ) )
        {
            // remember match position in haystack
            if( r == 0 )
            {
                r = p1 ;
            }
            // next position in needle
            p2++ ;
        }
        else
        {
            // reset needle
            p2 = needle ;
            
            // reset starting point haystack
            if( r != 0 )
            {
                p1 = r + 1 ;
            }
            // compare the needle in the haystack
            if( tolower( (unsigned char)*p1 ) == tolower( (unsigned char)*p2 ) )
            {
                // remember start
                r = p1 ;
                // next position needle
                p2++ ;
            }
            else
            {
                // reset remember
                r = 0 ;
            }
        }
        // next haystack
        p1++ ;
    }
    
    // if end of needle (and thus was found) return the start position
    // else return zero
    return *p2 == 0 ? (char*)r : 0 ;
}

/* print error message */
// display without color
int NoColor = 0;

/* Display in color
 * @param format : Message to display and optional arguments
 *                same as printf
 * @param level :  1 = RED, 2 = GREEN, 3 = YELLOW 4 = BLUE 5 = WHITE
 * 
 *  if NoColor was set, output is always WHITE.
 */
void p_printf(int level, char *format, ...)
{
    char	*col;
    int		coll=level;
    va_list arg;
    
    //allocate memory
    col = malloc(strlen(format) + 20);
    
    if (NoColor) coll = WHITE;
				
    switch(coll)
    {
		case RED:
			sprintf(col,REDSTR, format);
			break;
		case GREEN:
			sprintf(col,GRNSTR, format);
			break;		
		case YELLOW:
			sprintf(col,YLWSTR, format);
			break;		
		case BLUE:
			sprintf(col,BLUSTR, format);
			break;
		default:
			sprintf(col,"%s",format);
	}

   	va_start (arg, format);
	vfprintf (stdout, col, arg);
	va_end (arg);

	fflush(stdout);

    // release memory
    free(col);
}




/** 
 * parse an EPC uri entry and store content
 * p :   input start point in URI to copy from
 * del : input delimiter character (not added in out put)
 * b:    output string with characters between p and delimiter + 0x0 
 * 
 * To read between current point and end of input, set delimiter to 0x0.
 * 
 * return: 
 * error/could not find in uri : NULL
 * else  pointer in uri AFTER delimiter
 */
char *pars_uri_entry(char *p, char del, char *b)
{	
	char	*s = p;
	
	// initialize return
	*b = 0x0;
	
	while (*p != del)
	{
		 if (*p != 0x0) *b++ = *p++;
		 else
		 {
			 p_printf(RED,"Incorrect URI: can not parse and find %c in %s\n", del, s);
			 return(NULL);
		}
	}
	
	*b = 0x0;	// terminiate 
	
	// if delimiter was NOT 0x0, skip delimiter
	if (*p != 0x0) p++;
	
	return(p);
}

/** lookup entry in partition table
 * scheme : scheme from EPC URI
 * L = used to find the right partition row based on length
 *     if sent to 0xff, it is ignored as search argument
 * 
 * return : 
 * 0xff  error
 * else offset in partition table
 */
uint16_t epc_scheme_lkup(char *scheme, int L)
{
	uint16_t  i=0;
	
	if (strcmp(scheme, "gid-96") == 0 || strcmp(scheme, "adi-var") == 0 || strcmp(scheme, "usdod-96") == 0 ) 
	{
		p_printf(RED,"%s: requires special handling. Not implemented\n", scheme);
		return(0xff);
	}
	
	while (1)
	{
		if (strcmp(part_table[i].scheme, "END") == 0) return(0xff);
		else 
		{	
			if (strcmp(part_table[i].scheme, scheme) == 0) 
			{
				// if length is a search argument
				if (L != 0xff)
				{
					if (part_table[i].comp1_dig == L) return(i);
				}
				else
					return(i);
			}
		}

		i++;
	}
}

/** 
 * lookup entry in partition table
 * hdr : header from binary EPC
 * part: it partition entry. (if more than 6 it will be ignored)
 * 
 * There are 2 special one to handle : GID-96 and ADI-VAR.
 * 
 * return : 
 * 0xff  error
 * else offset in partition table
 */
uint16_t epc_hdr_lkup(uint64_t hdr, int part)
{
	uint16_t  i=0;
	
	if (hdr == 0x35 )
	{
		p_printf(RED,"header 0x35 is GID-96, which is hardcoded\n");
		return(0xff);
	}
	else if (hdr == 0x3b)
	{
		p_printf(RED,"header 0x35 is adi-var, which is hardcoded\n");
		return(0xff);
	}
	else if (hdr == 0x2f)
	{
		p_printf(RED,"header 0x2f is usdod-96,which is hardcoded\n");
		return(0xff);
	}
		
	while (1)
	{
		if (strcmp(part_table[i].scheme, "END") == 0) 
		{
			if ((hdr >= 0x40 && hdr < 0xff) || (hdr >= 0xc && hdr < 0x10))
				p_printf(RED,"hdr: 0x%02lx: Reserved until 64 bit sunset\n", hdr);

			else if ((hdr >= 0x3f && hdr < 0x40) ||(hdr >= 0x10 && hdr < 0x2c))
				p_printf(RED,"hdr: 0x%02lx: Reserved for Future Use\n", hdr);

			else if (hdr >= 0x1 && hdr < 0x8)
				p_printf(RED,"hdr: 0x%02lx: EPC header indicates RFU EPC\n", hdr);			
			
			else if (hdr == 0xff)
				p_printf(RED,"hdr: 0x%02lx: Reserved for Future headers longer than 8 bits\n", hdr);					
			
			else
				p_printf(RED,"Can not find information for header %02lx\n", hdr);
			
			return(0xff);
		}
		else 
		{	// partition row has to be 0 - 6
			if (part < 7)
			{
				if (part_table[i].header == hdr && part_table[i].partrow == part)
					return(i);
			}
			else
				if (part_table[i].header == hdr) 	return(i);
		}
		
		i++;
	}
}

/**
 * cpi-96 & cpi-var 
 *  
 * input : urn:epc:id:cpi:0614141.98765.12345
 * ouput : (8010) 061414198765 (8011) 12345
 */

int dec_epcTObarc_cpi(char *p,char *out)
{
	char buf1[35], buf2[35], buf3[35];
	
	if ((p = pars_uri_entry(p,'.',buf1)) == NULL) return(EPC_ERROR);
	if ((p = pars_uri_entry(p,'.',buf2)) == NULL) return(EPC_ERROR);
	if ((p = pars_uri_entry(p,0x0,buf3)) == NULL) return(EPC_ERROR);	

	strcpy(buf1+strlen(buf1), buf2);
	sprintf(out,"(8010) %s (8011) %s",buf1,buf3);
	
	return(EPC_SUCCESS);
}


/**
 * Gid-96 
 * 
 */

int dec_epcTObarc_gid(char *p,char *out)
{
	char buf1[35], buf2[35], buf3[35];
	
	if ((p = pars_uri_entry(p,'.',buf1)) == NULL) return(EPC_ERROR);
	if ((p = pars_uri_entry(p,'.',buf2)) == NULL) return(EPC_ERROR);
	if ((p = pars_uri_entry(p,0x0,buf3)) == NULL) return(EPC_ERROR);	

	sprintf(out,"general manager : %s, class : %s, serial : %s",buf1,buf2,buf3);
	
	return(EPC_SUCCESS);
}

/**
 * adi-var 
 * 
 */

int dec_epcTObarc_adi(char *p,char *out)
{
	char buf1[35], buf2[35], buf3[35];
	
	if ((p = pars_uri_entry(p,'.',buf1)) == NULL) return(EPC_ERROR);
	if ((p = pars_uri_entry(p,'.',buf2)) == NULL) return(EPC_ERROR);
	if ((p = pars_uri_entry(p,0x0,buf3)) == NULL) return(EPC_ERROR);	
	
	sprintf(out,"CAGE/DoDAAC : %s, Partnumber : %s, serial : %s",buf1,buf2,buf3);
	return(EPC_SUCCESS); 
}

/**
 * usdod-96 
 * 
 */

int dec_epcTObarc_usdod(char *p,char *out)
{
	char buf1[35], buf3[35];
	
	if ((p = pars_uri_entry(p,'.',buf1)) == NULL) return(EPC_ERROR);
	if ((p = pars_uri_entry(p,0x0,buf3)) == NULL) return(EPC_ERROR);	
	
	sprintf(out,"CAGE/DoDAAC : %s, serial : %s",buf1,buf3);
	return(EPC_SUCCESS); 
}

/** 
 * sgln 
 * 
 * input:
 * urn:epc:id:sgln:0614141.12345.5678
 * or
 * urn:epc:tag:sgln-96:3.0614141.12345.5678
 *
 * (414) 0614141123452 (254) 5678
 *
 */

int dec_epcTObarc_sgln(char *p,char *out)
{
	char buf1[50], buf2[50], buf3[35];
	int digit, mul, len,i;
		
	if ((p = pars_uri_entry(p,'.',buf1)) == NULL) return(EPC_ERROR);
	if ((p = pars_uri_entry(p,'.',buf2)) == NULL) return(EPC_ERROR);
	if ((p = pars_uri_entry(p,0x0,buf3)) == NULL) return(EPC_ERROR);	

	// initialize values
	len = strlen(buf1);
	strcpy(buf1+len,buf2);
	
	len = strlen(buf1);

	/* calculate check digit */
	mul = 1;
	digit = 0;
		
	for(i =0; i< len; i++)
	{
		digit += (buf1[i] -'0') * mul;
	
		if (mul == 3) mul=1;
		else mul = 3;
	} 

	digit = digit%  10;
	if (digit) digit = 10 - digit;

	// translate (potential) escaped characters in serial
	p = buf2;
	
	for (i = 0; i < strlen(buf3); i++)
	{
		if (buf3[i] == '%')
		{
			i++;

			// set next 2 bytes as single character
			if (buf3[i] > '9') 	*p = ((buf3[i++] + '9') & 0x0f) << 4;
			else *p = (buf3[i++] & 0xf) << 4;
		
			if (buf3[i] > '9')	*p++ |= (buf3[i] + '9') & 0xf;
			else *p++ |= buf3[i] & 0xf;
		}
		else
			*p++ =  buf3[i];
	}
	
	*p = 0x0;

	/* output */
	sprintf(out,"(414) %s%d (254) %s",buf1,digit,buf2);
	
	return(EPC_SUCCESS);
}

/** 
 * giai
 * 
 * input:
 * urn:epc:id:giai:0614141.5678
 * or
 * urn:epc:id:giai:0614141.32a%2Fb
 *
 * (8004) 06141415678 or
 * (8004) 061414132a/b
 *
 */

int dec_epcTObarc_giai(char *p,char *out)
{
	char buf1[50], buf2[50], buf3[35], *m;
	int  i;
		
	if ((p = pars_uri_entry(p,'.',buf1)) == NULL) return(EPC_ERROR);
	if ((p = pars_uri_entry(p,0x0,buf3)) == NULL) return(EPC_ERROR);

	// translate (potential) escaped characters in asset reference
	m = buf2;
	
	for (i = 0; i < strlen(buf3); i++)
	{
		if (buf3[i] == '%')
		{
			i++;

			// set next 2 bytes as single character
			if (buf3[i] > '9') 	*m = ((buf3[i++] + '9') & 0x0f) << 4;
			else *m = (buf3[i++] & 0xf) << 4;
		
			if (buf3[i] > '9')	*m++ |= (buf3[i] + '9') & 0xf;
			else *m++ |= buf3[i] & 0xf;
		}
		else
			*m++ =  buf3[i];
	}
	
	*m = 0x0;

	/* output */
	sprintf(out,"(8004) %s%s",buf1,buf2);
	
	return(EPC_SUCCESS);
}


/** 
 * grai
 * 
 * input:
 * urn:epc:tag:grai-96:3.0614141.12345.5678
 * or
 * urn:epc:id:grai:0614141.12345.32a%2Fb
 *
 * (8003) 006141411234525678 or
 * (8003) 0061414112345232a/b
 *
 */

int dec_epcTObarc_grai(char *p,char *out)
{
	char buf1[50], buf2[50], buf3[35];
	int digit, mul, len,i;
		
	if ((p = pars_uri_entry(p,'.',buf1)) == NULL) return(EPC_ERROR);
	if ((p = pars_uri_entry(p,'.',buf2)) == NULL) return(EPC_ERROR);
	if ((p = pars_uri_entry(p,0x0,buf3)) == NULL) return(EPC_ERROR);	

	// initialize values
	len = strlen(buf1);
	strcpy(buf1+len,buf2);
	
	len = strlen(buf1);

	/* calculate check digit */
	mul = 1;
	digit = 0;
		
	for(i =0; i< len; i++)
	{
		digit += (buf1[i] -'0') * mul;
	
		if (mul == 3) mul=1;
		else mul = 3;
	} 

	digit = digit%  10;
	if (digit) digit = 10 - digit;

	// translate (potential) escaped characters in serial
	p = buf2;
	
	for (i = 0; i < strlen(buf3); i++)
	{
		if (buf3[i] == '%')
		{
			i++;

			// set next 2 bytes as single character
			if (buf3[i] > '9') 	*p = ((buf3[i++] + '9') & 0x0f) << 4;
			else *p = (buf3[i++] & 0xf) << 4;
		
			if (buf3[i] > '9')	*p++ |= (buf3[i] + '9') & 0xf;
			else *p++ |= buf3[i] & 0xf;
		}
		else
			*p++ =  buf3[i];
	}
	
	*p = 0x0;

	/* output */
	sprintf(out,"(8003) 0%s%d%s",buf1,digit,buf2);
	
	return(EPC_SUCCESS);
}



/** 
 * gdti 
 * 
 * input:
 * urn:epc:id:gdti:4012345.98765.ABCDefgh012345678
 * or
 * urn:epc:tag:gdti-174:3.4012345.98765.ABCDefgh012345678
 *
 * (253) 4012345987652ABCDefgh012345678
 *
 */

int dec_epcTObarc_gdti(char *p,char *out)
{
	char buf1[50], buf2[20], buf3[35];
	int digit, mul, len,i;
		
	if ((p = pars_uri_entry(p,'.',buf1)) == NULL) return(EPC_ERROR);
	if ((p = pars_uri_entry(p,'.',buf2)) == NULL) return(EPC_ERROR);
	if ((p = pars_uri_entry(p,0x0,buf3)) == NULL) return(EPC_ERROR);	

	// initialize values
	len = strlen(buf1);
	strcpy(buf1+len,buf2);
	
	len = strlen(buf1);

	/* calculate check digit */
	mul = 1;
	digit = 0;
		
	for(i =0; i< len; i++)
	{
		digit += (buf1[i] -'0') * mul;
	
		if (mul == 3) mul=1;
		else mul = 3;
	} 

	digit = digit%  10;
	if (digit) digit = 10 - digit;

	/* output */
	sprintf(out,"(253) %s%d%s",buf1,digit,buf3);
	
	return(EPC_SUCCESS);
}

/** 
 * sgrn-96 
 * 
 * input:
 * urn:epc:id:gsrn:0614141.1234567890
 * urn:epc:id:gsrnp:0614141.1234567890
 * or
 * urn:epc:tag:gsrn-96:3.0614141.1234567890
 * urn:epc:tag:gsrnp-96:3.0614141.1234567890
 * 
 * output
 * (8018) 061414112345678902
 * (8017) 061414112345678902

 */

int dec_epcTObarc_gsrn(char *p,char *out, char *scheme)
{
	char buf1[50], buf2[20];
	int digit, mul, len,i;
	
	if ((p = pars_uri_entry(p,'.',buf1)) == NULL) return(EPC_ERROR);
	if ((p = pars_uri_entry(p,0x0,buf2)) == NULL) return(EPC_ERROR);

	// initialize values
	len = strlen(buf1);
	strcpy(buf1+len,buf2);
	
	len = strlen(buf1);

	/* calculate check digit */
	mul = 3;
	digit = 0;
		
	for(i =0; i< len; i++)
	{
		digit += (buf1[i] -'0') * mul;
	
		if (mul == 3) mul=1;
		else mul = 3;
	} 

	digit = digit%  10;
	if (digit) digit = 10 - digit;

	/* output */
	if(strcmp(scheme, "sgrnp") == 0)
		sprintf(out,"(8017) %s%d",buf1,digit);
	else if(strcmp(scheme, "sgrn") == 0)
		sprintf(out,"(8018) %s%d",buf1,digit);

	return(EPC_SUCCESS);
}

/** 
 * sscc-96 
 * 
 * input:
 * urn:epc:id:sscc:0614141.1234567890
 * or
 * urn:epc:tag:sscc-96:3.0614141.1234567890

 * (00) 106141412345678908

 */

int dec_epcTObarc_sscc(char *p,char *out)
{
	char buf1[50], buf2[20], buf3[35];
	int digit, mul, len,i;
		
	if ((p = pars_uri_entry(p,'.',buf2)) == NULL) return(EPC_ERROR);
	if ((p = pars_uri_entry(p,0x0,buf3)) == NULL) return(EPC_ERROR);

	// initialize values
	sprintf(buf1,"%c%s%s",buf3[0],buf2,&buf3[1]);
	
	len = strlen(buf1);

	/* calculate check digit */
	mul = 3;
	digit = 0;
		
	for(i =0; i< len; i++)
	{
		digit += (buf1[i] -'0') * mul;
	
		if (mul == 3) mul=1;
		else mul = 3;
	} 

	digit = digit%  10;
	if (digit) digit = 10 - digit;

	/* output */
	sprintf(out,"(00) %s%d",buf1,digit);
	
	return(EPC_SUCCESS);
}



/** 
 * sgcn-96 
 * 
 * input:
 * urn:epc:id:sgcn:4012345.67890.04711
 * or
 * urn:epc:tag:sgcn-96:3.4012345.67890.04711

 * (255) 401234567890104711 

 */

int dec_epcTObarc_sgcn(char *p,char *out)
{
	char buf1[50], buf2[20], buf3[35];
	int digit, mul, len,i;
		
	if ((p = pars_uri_entry(p,'.',buf1)) == NULL) return(EPC_ERROR);
	if ((p = pars_uri_entry(p,'.',buf2)) == NULL) return(EPC_ERROR);
	if ((p = pars_uri_entry(p,0x0,buf3)) == NULL) return(EPC_ERROR);	

	// initialize values
	len = strlen(buf1);
	strcpy(buf1+len,buf2);
	
	len = strlen(buf1);

	/* calculate check digit */
	mul = 1;
	digit = 0;
		
	for(i =0; i< len; i++)
	{
		digit += (buf1[i] -'0') * mul;
	
		if (mul == 3) mul=1;
		else mul = 3;
	} 

	digit = digit%  10;
	if (digit) digit = 10 - digit;

	/* output */
	sprintf(out,"(255) %s%d%s",buf1,digit,buf3);
	
	return(EPC_SUCCESS);
}

/**
 * Decode an EPC  (tag or Pure-identity uri) to barcode
 * epc     : input pure or tag URI
 * out     : output of the barcode 
 * format  : 0 = barcode, 1 = GTIN
 * 
 * input : 
 * 		urn:epc:id:sgtin:763003032.0120.10562771
 * 		urn:epc:tag:sgtin-96:4.763003032.0120.10562771
 * 
 * output: 
 *  format = 0 : 76300303201207
 * 	format = 1 : (01)76300303201207 (21) 10562771
 * 
 */

int dec_epcTObarc(char *epc, char* out, int format)
{
	char	purestart[]= "urn:epc:id:";
	char	tagstart[]= "urn:epc:tag:";
	char	scheme[25], tmp[25], barcode[30];
	char	buf1[20], buf2[30], buf3[20];
	char	*p = epc;
	int	    odd = 0 , even = 0, digit, i;
	
	buf3[0]=0x0;
				
	/* detect pure-identity URI at start */
	if (stristr(epc, purestart) != NULL)
	{
		// initialize values
		p += strlen(purestart); 

		// skip scheme
		if ((p = pars_uri_entry(p,':',scheme)) == NULL) return(EPC_ERROR);
	}
	
	/* detect tag-encodng URI at start */
	else if (stristr(epc, tagstart) != NULL)
	{
		// initialize values
		p += strlen(tagstart); 

		// skip scheme
		if ((p = pars_uri_entry(p,':',scheme)) == NULL) return(EPC_ERROR);
		
		// gid-96 has NO filter !
		if (stristr(scheme,"gid") == NULL)
		{
			/* skip filter */

			if ((p = pars_uri_entry(p,'.',tmp)) == NULL) return(EPC_ERROR);
		}
	}
	else
	{	
		p_printf(RED,"invalid EPC: %s\n", epc);
		return(EPC_ERROR);
	}

	// the following need special work
	if (stristr(scheme,"sgcn") != NULL) return(dec_epcTObarc_sgcn(p,out));
	if (stristr(scheme,"adi") != NULL) return(dec_epcTObarc_adi(p,out));
	if (stristr(scheme,"usdod") != NULL) return(dec_epcTObarc_usdod(p,out));
	if (stristr(scheme,"gid") != NULL) return(dec_epcTObarc_gid(p,out));
	if (stristr(scheme,"cpi") != NULL) return(dec_epcTObarc_cpi(p,out));
	if (stristr(scheme,"gdti") != NULL) return(dec_epcTObarc_gdti(p,out));
	if (stristr(scheme,"gsrn") != NULL) return(dec_epcTObarc_gsrn(p,out,scheme));
	if (stristr(scheme,"sscc") != NULL) return(dec_epcTObarc_sscc(p,out));
	if (stristr(scheme,"sgln") != NULL) return(dec_epcTObarc_sgln(p,out));
	if (stristr(scheme,"grai") != NULL) return(dec_epcTObarc_grai(p,out));
	if (stristr(scheme,"giai") != NULL) return(dec_epcTObarc_giai(p,out));
	
	
	/* sgtin assumed from here */
	// get company info, number
	if ((p = pars_uri_entry(p,'.',buf1)) == NULL) return(EPC_ERROR);	
	if ((p = pars_uri_entry(p,'.',buf2)) == NULL) return(EPC_ERROR);
	
	// if more, get serial number
	if (p < epc + strlen(epc)) 
		if ((p = pars_uri_entry(p,0x0,buf3)) == NULL) return(EPC_ERROR);
	
	// create barcode
	sprintf(barcode,"%c%s%s",buf2[0],buf1,&buf2[1]);
	
	// get odd  & even numbers
	for (i =0; i < strlen(barcode);i++)
	{
		if (barcode[i] < '0' || barcode[i]> '9')
		{
			p_printf(RED,"invalid character in EPC : %c or 0x%02x\n", barcode[i], barcode[i]);
			return(EPC_ERROR);
		}
		
		// calculate odd and even places
		if (i == 0 || (i+1)%2)	odd += barcode[i] - '0';
		else even += barcode[i] - '0';
	}
	
	// GTN-8     GTN-12     GTN-13/GLN   GTN-14    SSCC 
	if(i != 7 && i != 11 && i != 12 && i != 13 && i != 17)
	{
		p_printf(RED,"invalid number of digits input : %d\n", i);
		return(EPC_ERROR);
	}
	
	// calculate check digit
	digit = ((odd*3) + even) % 10;
	if (digit) digit = 10 - digit;

	// add to barcode
	sprintf(barcode,"%s%c", barcode, (digit + '0'));
	
	// if GTIN format is requested
	if (format == 1)
	{
		// add serial number (if available)
		if (strlen(buf3) > 0)
		{
			// translate escaped characters
			p = buf2;
			
			for (i = 0; i < strlen(buf3); i++)
			{
				if (buf3[i] == '%')
				{
					i++;

					// set next 2 bytes as single character
					if (buf3[i] > '9') 	*p = ((buf3[i++] + '9') & 0x0f) << 4;
					else *p = (buf3[i++] & 0xf) << 4;
				
					if (buf3[i] > '9')	*p++ |= (buf3[i] + '9') & 0xf;
					else *p++ |= buf3[i] & 0xf;
				}
				else
					*p++ =  buf3[i];
			}
			
			*p = 0x0;
			
			sprintf(out,"(01) %s (21) %s", barcode, buf2);
		}
		else
			sprintf(out,"(01) %s", barcode);		
	}
	else
		sprintf(out,"%s", barcode);
		
	return(EPC_SUCCESS);
}

/**
 * get 16 bits from the binary EPC into temporary buffer
 */

void dec_fill_bit_buffer(uint8_t *epc)
{
	bitval = epc[epc_pos++] << 8;
	bitval |= epc[epc_pos++];
	bitcnt = 16; 
}



/** 
 * get (max) 64 bit integer value from binary EPC
 * 
 * EPC : input binary EPC to parse
 * bits: input number of bits to get
 * val : output value
 */
int dec_get_integer(uint8_t *epc, int bits, uint64_t *val)
{
	*val = 0;

	if (bits > 64 )
	{
		p_printf(RED,"Too many bits requested. Max 64, request %d\n", bits);
		return(EPC_ERROR);
	}
	
	while (bits--)
	{
		// if temp bit buffer is empty, get next 16 bits
		if (bitcnt == 0) dec_fill_bit_buffer(epc);

		// move earlier left (if first time nothing happens)
		*val <<= 1;
		
		// if MSB in input is 1 : set 1 else leave 0
		if (bitval & 0x8000)  *val = *val + 1 ;

		// remove evaluated bit
		bitval <<= 1;
		bitcnt--;
	}
	
	return(EPC_SUCCESS);
}

/** 
 * create integer string from binary EPC
 * 
 * EPC : input binary EPC to parse
 * bits: input number of bits to get  : (max) 64 bit
 * out : output string 
 */
int dec_comp_integer(uint8_t *epc, int bits, char *out)
{
	uint64_t val64;

	// get integer
	if (dec_get_integer(epc, bits, &val64) == EPC_ERROR) return(EPC_ERROR);
			
	/*The output shall not begin with a zero character if it is two or
	 * more digits in length.*/
	sprintf(out,"%ld",val64);

	return(EPC_SUCCESS);
}





/** 
 * get 7 bit string values from binary EPC
 * 
 * EPC    : input binary EPC to parse
 * bits   : input number of bits to get
 * string : output string value (0x0 terminated)
 * 
 */
int dec_comp_string(uint8_t *epc, int bits, char *string)
{
	int     vald = 0;
	int		count = 1, done = 0;
	char	*p = string;
	
	while (bits--)
	{
		// if temp bit buffer is empty, fill it
		if (bitcnt == 0) dec_fill_bit_buffer(epc);

		// move earlier entry left (if first time nothing happens)
		vald <<= 1;

		// if MSB in input is 1 : set 1 else leave 0
		if (bitval & 0x8000)  vald = vald + 1 ;
	
		// remove evaluated bit
		bitval <<= 1;
		bitcnt--;

		// each character is 7 bits
		if (count++ == 7)
		{
			// end of string input detected
			if (vald == 0x0)
			{
				// must have at least ONE character
				if (p == string)
				{
					p_printf(RED,"Error empty string received\n");
					return(EPC_ERROR);
				}
				
				// indicate done and now only count bits out
				done= 1;
			}
		
			if (! done)
			{

				// check for GS3A3Component
				if (vald < 0x21 || vald > 0x7A)
				{
					p_printf(RED,"illegal character %c, 0x%02X\n", vald,vald);
					return(EPC_ERROR);
				}
				
				// translate special characters
				if (vald == 0x22 || vald == 0x25 || vald == 0x26 || vald == 0x2f ||
				vald == 0x3c || vald == 0x3e || vald == 0x3f )
				{
					*p++ = '%';
					sprintf(p,"%02X", vald);
					p += 2;
				}
				
				// add character to output
				else
					*p++ = vald;
			}
			
			vald = 0;
			count = 1;
		}
	}
	
	*p = 0x0;
	
	return(EPC_SUCCESS);
}


/**
 * 
 * only for SGTIN !
 * 
 * input : 
 * urn:epc:id:sgtin:0434687.075801.274877906943
 * urn:epc:tag:sgtin-96:3.0434687.075801.274877906943
 * 
 * output: 075801.0434687.sgtin.id.onsepc.com
 * 
 * check with dig  -t  NAPTR  075861.0434687.sgtin.id.onsepc.com
 */
int dec_epcTONS(char *epc, char *ons)
{
	char	purestart[]= "urn:epc:id:";
	char	tagstart[]= "urn:epc:tag:";
	char	scheme[15], comp[15], part[35], tmp[10];
	char 	*p = epc;
	
	/* detect pure-identity URI at start */
	if (stristr(epc, purestart) != NULL)
	{
		// initialize values
		p += strlen(purestart); 
		
		// get scheme
		if ((p = pars_uri_entry(p,':',scheme)) == NULL) return(EPC_ERROR);
				
		if(stristr(scheme,"sgtin") == NULL)
		{
			p_printf(RED,"can not create ONS with scheme %s\n", scheme);
			return(EPC_ERROR);
		}
	}

	/* detect tag-encodng URI at start */
	else if (stristr(epc, tagstart) != NULL)
	{
		// initialize values
		p += strlen(tagstart); 

		// get scheme
		if ((p = pars_uri_entry(p,':',scheme)) == NULL) return(EPC_ERROR);

		if(stristr(scheme,"sgtin") == NULL)
		{
			p_printf(RED,"can not create ONS with scheme %s\n", scheme);
			return(EPC_ERROR);
		}
		
		/* skip filter */
		if ((p = pars_uri_entry(p,'.', tmp)) == NULL) return(EPC_ERROR);
	}
	else
	{	
		p_printf(RED,"invalid EPC: %s\n", epc);
		return(EPC_ERROR);
	}

	// get company info, number
	if ((p = pars_uri_entry(p,'.',comp)) == NULL) return(EPC_ERROR);	
	if ((p = pars_uri_entry(p,'.',part)) == NULL) return(EPC_ERROR);

	// create ONS
	sprintf(ons,"%s.%s.sgtin.id.onsepc.com",part, comp);

	return(EPC_SUCCESS);
}
/**
 * Decode a binary EPC to barcode
 * 
 * epc     : input binary epc
 * barcode : output the barcode 
 * len     : number of binary bytes in epc
 * format  : 0= barcode , 1= GTIN
 * 
 * example :
 * input : 308ED7A809801E0000A12CD3
 * 
 * output: 
 *  format = 0 76300303201207
 * 	format = 1 (01) 76300303201207	(21) 10562771
 */

int dec_binTObarc(uint8_t *bin_epc, char* barcode, int len, int format)
{
	char	tmp[50];
	
	// binary to pure
	if (dec_binTOpure(bin_epc,  tmp, len) == EPC_ERROR) return(EPC_ERROR);

	// pure Uri to barcode
	return(dec_epcTObarc(tmp, barcode, format));
}

/**
 * create from tag_EPC a pure-identity_EPC 
 * 
 * tag_EPC  : input tag EPC
 * pure_epc : output pure EPC
 * 
 * example
 * input : urn:epc:tag:sgtin-96:4.763003032.0120.10562771
 * output : urn:epc:id:sgtin:763003032.0120.10562771
 */
 
int dec_tagTOpure(char *tag_epc, char* pure_epc)
{
	char	*p;
	char	uristart[]= "urn:epc:tag:";
	char	scheme[15], buf[20];
	
	// detect correct format URL from tag and start
	if ((p = stristr(tag_epc, uristart)) == NULL)
	{
		p_printf(RED,"incorrect URI : %s. stopping\n", tag_epc);
		return(EPC_ERROR);
	}
		
	// initialize values
	p += strlen(uristart); 

	// get scheme without tagsize
	if ((p = pars_uri_entry(p,'-',scheme)) == NULL) return(EPC_ERROR);

	// gid does NOT have filter values
	if (strcmp(scheme, "gid") == 0)
	{
		if ((p = pars_uri_entry(p,':',buf)) == NULL) return(EPC_ERROR);
	}
	else // skip tagsize & filter
	{
		if ((p = pars_uri_entry(p,'.',buf)) == NULL) return(EPC_ERROR);
	}
	// Building url
	sprintf(pure_epc, "urn:epc:id:%s:%s",scheme, p);

	return(EPC_SUCCESS);
}


/** 
 * get part integer from binary EPC
 * 
 * EPC : input binary EPC to parse
 * bits: input number of bits to get  : (max) 64 bit
 * dig : input number of digits in out
 * out : output string
 *  
 */
int dec_comp_part_unpadded(uint8_t *epc, int bits, int dig, char *out)
{
	uint64_t val64;

	// get integer
	if (dec_get_integer(epc, bits, &val64) == EPC_ERROR) return(EPC_ERROR);

	/* The value must be less than 10^L , where L is the value of digits
	 * specified in the the matching partition table row.*/
	if (val64 > pow(10,(double) dig))
	{
		p_printf(RED,"Number to large. Max %f\n", pow(10,(double) dig));
		return(EPC_ERROR);
	}			
			
	/* no leading zeros (except that if D = 0 it is converted to a single zero digit).*/	
	sprintf(out,"%ld",val64);
	
	return(EPC_SUCCESS);
}

/** 
 * get part integer from binary EPC
 * 
 * EPC : input binary EPC to parse
 * bits: input number of bits to get  : (max) 64 bit
 * dig : input number of digits in out
 * out : output string
 *  
 */
int dec_comp_part_integer(uint8_t *epc, int bits, int dig, char *out)
{
	uint64_t val64;
	char	addtobuf[10];

	// get integer
	if (dec_get_integer(epc, bits, &val64) == EPC_ERROR) return(EPC_ERROR);

	/* The value must be less than 10^L , where L is the value of digits
	 * specified in the the matching partition table row.*/
	if (val64 > pow(10,(double) dig))
	{
		p_printf(RED,"Number to large. Max %f\n", pow(10,(double) dig));
		return(EPC_ERROR);
	}			
			
	/* padding on the left with zero (“0”) characters to make digits in total*/	
	sprintf(addtobuf,"%c0%dld",'%',dig);
	sprintf(out,addtobuf,val64);
	
	return(EPC_SUCCESS);
}

/** 
 * create integer NumericString from binary EPC
 * 
 * EPC : input binary EPC to parse
 * bits: input number of bits to get  : (max) 64 bit
 * out : output string 
 */
int dec_comp_NumString_integer(uint8_t *epc, int bits, char *out)
{
	// get integer
	if (dec_comp_integer(epc,  bits, out) == EPC_ERROR) return(EPC_ERROR);
	
	/* first character MUST 1 and length has to be more than 1) */		
	if (out[0] != '1' || strlen(out) < 2)
	{
		p_printf(RED,"Incorrect NumericString. Does not start with 1 or empty: %s\n", out);
		return(EPC_ERROR);
	}
	
	/* Delete the leading “1” character from the numeral */
	sprintf(out, "%s", &out[1]);

	return(EPC_SUCCESS);
}



/** 
 * get 7 bit string values from binary EPC
 * EPC : input binary EPC to parse
 * bits: input number of bits to get
 * string : output string value (0x0 terminated)
 * 
 */

int dec_get_numstring(uint8_t *epc, int bits, char *string)
{
	// get string or INTEGER ?????
	if (dec_comp_string(epc, bits, string) == EPC_ERROR)
		return(EPC_ERROR);
	
	// top character MUST be 1	
	if (strlen(string) != '1')
	{
		p_printf(RED,"invalid Numstring. Got %s\n", string);
		return(EPC_ERROR);
	}
	
	// remove top character ( should that be string[0] ?)
	string[strlen(string)] = 0x0;
		
	return(EPC_SUCCESS);
}

/**
 * create 6 bits character string from binary
 * 
 * The length of this bit string is always 36 bits
 * 
 * CAGECodeOrDODAAC 6 bit code-value is nearly the same as ASCII, but removed the 2 top bits. As a result it only supports
 * a digits and capitals. Also capital I and capital O are not allowed as they could be confused with digit 1 (one) and 0 (zero).
 * The difference between 'cage' and '6-bit string' is in the later 3-character escape sequence is allowed to handle #.
 * 
 */
int dec_comp_cage(uint8_t *epc, char *string)
{
	int     vald = 0;
	int		bits = 36;	
	int		count = 1;
	char	*p = string;
	
	while (bits--)
	{
		// if temp bit buffer is empty, fill it
		if (bitcnt == 0) dec_fill_bit_buffer(epc);

		// move earlier entry left (if first time nothing happens)
		vald <<= 1;
		
		// if MSB in input is 1 : set 1 else leave 0
		if (bitval & 0x8000)  vald = vald + 1 ;
	
		// remove evaluated bit
		bitval <<= 1;
		bitcnt--;

		// each character is 6 bits
		if (count++ == 6)
		{
			count = 1;
				
			/* The first 6-bit segment must be the value 100000 (0x20), 
			 * or correspond to a digit character, or an uppercase 
			 * alphabetic character excluding the letters I and O */
		
			if (p == string)
			{
				// skip 100000 in case first
				if (vald == 0x20)
				{
					vald=0;
					continue;
				}
			}
			
			// check for CAGECodeOrDODAAC
			if (vald >= '0' && vald <= 'Z')
			{
				/* capital I and capital O are not allowed as they could 
				 * could be confused with digit 1 (one) and 0 (zero) */
				if (vald != 'I' && vald != 'O')
				{		
					// add 6 bits character
					*p++ = vald;
		
					vald = 0;

					continue;
				}
			}

			p_printf(RED,"incorrect CAGECodeOrDODAAC character: %c, %x\n",vald, vald);
			return(EPC_ERROR);
		}
	}
	
	*p = 0x0;
	return(EPC_SUCCESS);
}

/** 
 * get 6 bit string values from binary EPC
 * 
 * EPC    : input binary EPC to parse
 * bits   : input number of bits to get
 * string : output string value (0x0 terminated)
 * 
 */
int dec_comp_cage6bit_string(uint8_t *epc, int min_dig, int max_dig, char *string)
{
	int     vald = 0;
	int		count = 1, dig_cnt=0;
	char	*p = string;
	
	while (1)
	{
		// if temp bit buffer is empty, fill it
		if (bitcnt == 0) dec_fill_bit_buffer(epc);

		// move earlier entry left (if first time nothing happens)
		vald <<= 1;
		
		// if MSB in input is 1: set 1 else leave 0
		if (bitval & 0x8000)  vald = vald + 1 ;
	
		// remove evaluated bit
		bitval <<= 1;
		bitcnt--;

		// each character is 6 bits
		if (count++ == 6)
		{
			count = 1;
			
			// 0x0 is end of string
			if (vald == 0)	break;
			
			// the top 2 bits need to be added (if more than 'A')
			if (vald < 0x1b) vald |= 0x40;
			
			/* The  6-bit segment must correspond to a digit character, 
			 * or an uppercase alphabetic character excluding the 
			 * letters I and O */
			
			// check for CAGECodeOrDODAAC
			if ((vald >= '0' && vald <= 'Z') || vald == '#' || vald == '-' || vald == '/')
			{
				/* capital I and capital O are not allowed as they could 
				 * could be confused with digit 1 (one) and 0 (zero) */
				if (vald != 'I' && vald != 'O')
				{		
					// add 6 bits character & translate special character(s)
					if (vald == '#' || vald == '-' || vald == '/')
					{ 
						*p++ = '%';
						sprintf(p,"%02X", vald);
						p += 2;	
					}
					else
						*p++ = vald;
				
					vald = 0;
					
					/* count digit (can not use strlen later due to 
					 * potential 3 character sequence(s) */
					dig_cnt++;
					
					continue;
				}
			}
			
			p_printf(RED,"incorrect CAGECodeOrDODAAC character: %c\n",vald);
			return(EPC_ERROR);
		}
	}
	
	*p = 0x0;
	
	/* The number of 6-bit segments preceding the terminating segment 
	 * must be greater than or equal to the minimum number of characters 
	 * and less than or equal to the maximum number of characters 
	 * specified in the footnote to the coding table for this coding 
	 * table column. If not, stop: the input is invalid */
	 
	if (dig_cnt < min_dig  || dig_cnt > max_dig)
	{
		p_printf(RED,"incorrect length: max_digits: %d, min_digits: %d, got: %d\n",min_dig, max_dig, dig_cnt);
		return(EPC_ERROR);
	}
	
	return(EPC_SUCCESS);
}





/** 
 * gid-96 needs special handling as it does NOT use partition table
 * 
 * The format is:
 * 8  bit EPC header (0x35)
 * 28 bit General manager number	(integer)
 * 24 bit object class				(integer)
 * 36 bit serial number				(integer)
 * 
 * No checks etc
 * 
 * 35 00 07 ab 70 42 5d 40 00 00 05 86 
 * 
 */
int dec_binTOtag_gid(uint8_t *epc, char *buf, int len)
{
	uint64_t	GM, OC, SN;

	// check on gid-96 header
	if (epc[0]  != 0x35) return(EPC_ERROR);

	if (dec_get_integer(epc, 28, &GM) == EPC_ERROR) return(EPC_ERROR);
	if (dec_get_integer(epc, 24, &OC) == EPC_ERROR) return(EPC_ERROR);
	if (dec_get_integer(epc, 36, &SN) == EPC_ERROR) return(EPC_ERROR);
	
	sprintf(buf, "urn:epc:tag:gid-96:%ld.%ld.%ld",GM,OC,SN);
	
	return(EPC_SUCCESS);	
}


/**
 *  usdod-96 needs special handling as it does NOT use partition table
 * 
 * 8   bit EPC header (0x3b)
 * 4   bit filter
 * 48  bit  6 x 8 bits CAGE/ DoDAAC
 * 36  bit Serial Number integer
 * 
 */
int dec_binTOtag_usdod(uint8_t *epc, char * buf, int len)
{
	char		  CA[7], SN[35];;
	uint64_t	  filter, tmp;
	int			  i, j;
	
	// get filter
	if (dec_get_integer(epc, 4, &filter) == EPC_ERROR) return(EPC_ERROR);

	// get max 6 cage  & remove leading spaces
    for(i = 0, j = 0; i < 6 ; i++)
    {
		if (dec_get_integer(epc, 8, &tmp) == EPC_ERROR) return(EPC_ERROR);
		
		// remove leading spaces
		if (tmp == 0x20)
		{
			if (j != 0)
			{
				p_printf(RED,"usdod incorrect space character\n");
				return(EPC_ERROR);
			}
		}
		else
			CA[j++] = (int) tmp;
	}
	
    CA[j] = 0x0;
   
    // get min 1, max 30 characters
    if(dec_comp_integer(epc, 36, SN) == EPC_ERROR)
		return(EPC_ERROR);

	sprintf(buf, "urn:epc:tag:usdod-96:%ld.%s.%s",filter,CA,SN);
				
	return(EPC_SUCCESS);	
}


/**
 *  ad-var needs special handling as it does NOT use partition table
 * 
 * 8   bit EPC header (0x3b)
 * 6   bit filter
 * 36  bit CAGE/ DoDAAC            6-bit CAGE/DoDAAC
 * 6 - 198 bit (MAX) Part Number   6-bit Variable String
 * 12 -186 bit (MAX) Serial Number 6-bit Variable String
 * 
 * part number digit   0 >= #  <=32
 * serial number digit 1 >= #  <=30
 * 
 * The “#” character (represented in the URI by the escape sequence %23)
 * may appear as the first character of the Serial Number segment, but 
 * otherwise may not appear in the Part Number segment or elsewhere in 
 * the Serial Number segment.
 */
int dec_binTOtag_adi(uint8_t *epc, char * buf, int len)
{
	char		  CA[7];
	uint64_t	  filter;
	static  char  PN[35];
	static	char  SN[35];
	char	*p;
	
	// get filter
	if (dec_get_integer(epc, 6, &filter) == EPC_ERROR) return(EPC_ERROR);
	
	// get max 6 cage
	if (dec_comp_cage(epc, CA) == EPC_ERROR) return(EPC_ERROR);
   
    // get min 0, max 32 characters
    if(dec_comp_cage6bit_string(epc, 0, 32, PN) == EPC_ERROR)
		return(EPC_ERROR);	
    
    // get min 1, max 30 characters
    if(dec_comp_cage6bit_string(epc, 1, 30, SN) == EPC_ERROR)
		return(EPC_ERROR);
	
	// %23 may only appear as first character(s) in partnumber
	if ((p = stristr(&PN[1], "%23")) != NULL) 
	{
		p_printf(RED,"invalid place in adi-var for %%23 (#)\n");
		return(EPC_ERROR);
	}

	// %23 may NOT appear in serial number
	if ((p = stristr(SN, "%23")) != NULL) 
	{
			p_printf(RED,"invalid Serial Number in adi-var with %%23 (#) : %s\n", SN);
			return(EPC_ERROR);
	}
	
	sprintf(buf, "urn:epc:tag:adi-var:%ld.%s.%s.%s",filter,CA,PN,SN);
				
	return(EPC_SUCCESS);	
}

/**
 * Decode a binary EPC to tag-encoding Uri
 * 
 * epc : input binary epc
 * buf : output tag-encoding uri 
 * len : input length of binary epc in bytes
 * 
 * example:
 * input  : 
 * 		308ED7A809801E0000A12CD3
 * 
 * output : 
 *      urn:epc:tag:sgtin-96:4.763003032.0120.10562771
 */

int dec_binTOtag(uint8_t *epc, char * buf, int len)
{
	uint16_t row;
	uint8_t hdr;
	uint64_t partition, filter, first_entry;
	char	 second_entry_string[100], third_entry_string[100];
	char dispbuf[30];
	
	// find entry based on EPC header
	hdr = epc[0];
	bitcnt=0;		// initialize
	epc_pos = 1;
		
	// gid-96 needs special handling (hardcoded)
	if (hdr == 0x35) return(dec_binTOtag_gid(epc, buf, len));
	
	// adi_var needs special handling (hardcoded)
	else if (hdr == 0x3b) return(dec_binTOtag_adi(epc, buf, len));

	// usdod-96 needs special handling (hardcoded)
	else if (hdr == 0x2f) return(dec_binTOtag_usdod(epc, buf, len));

	// 0xf: neglect partition as part of lookup	
	if ((row = epc_hdr_lkup(hdr, 0xf)) == 0xff) return(EPC_ERROR);

	// if for future use
	if (part_table[row].bits == 0)
	{
		p_printf(RED,"header %02x : %s for future use/not implemented. stopping\n",hdr, part_table[row].scheme);
		return(EPC_ERROR);
	}

	// length check of provided EPC
	if (part_table[row].bits > len * 8)
	{
		p_printf(RED,"Not enough EPC data. Need %d bits, received %d bits. stopping\n",part_table[row].bits, len * 8);
		return(EPC_ERROR);
	}
	
	/*TODO: check on 64 bits as that is coded differently  !! */
	
	
	// get first 16 bits fom the binary
	dec_fill_bit_buffer(epc);
	
	/* format in current 16 bits buffer looks like
	 * assume filter = 3 bits
	 * P = partition bits
	 * M = first value bits
	 * 
	 * MSB...............................LSB
	 * filter partition  10 bits component-1
	 * f f f   p p p     M M M M M M M M M M
	 */	

	/* get filter value */
	if (dec_get_integer(epc, part_table[row].filterlen, &filter) == EPC_ERROR)
		return(EPC_ERROR);

	/* get partition value */
	if (dec_get_integer(epc, 3, &partition) == EPC_ERROR)	return(EPC_ERROR);	
		
	/* lookup match partition entry including partition row	*/
	if ((row = epc_hdr_lkup(hdr, partition)) == 0xff) return(EPC_ERROR);

	/* get compontent_1 entry as part_integer (always)*/
	if (dec_get_integer(epc, part_table[row].comp1_len, &first_entry) == EPC_ERROR)
		return(EPC_ERROR);

	/* get component 2 entry if bits requested*/
	if (part_table[row].comp2_len > 0)
	{
		/* obtain and validate depending type */
		if (part_table[row].comp2_type == PartInteger)
		{ 
			if (dec_comp_part_integer (epc, part_table[row].comp2_len, part_table[row].comp2_dig, second_entry_string) == EPC_ERROR)
				return(EPC_ERROR);
		}
		
		else if (part_table[row].comp2_type == Integer)
		{
			if (dec_comp_integer(epc, part_table[row].comp2_len, second_entry_string) == EPC_ERROR)
				return(EPC_ERROR);
		}
		
		else if (part_table[row].comp2_type == PartString)
		{
			if (dec_comp_string(epc, part_table[row].comp2_len, second_entry_string) == EPC_ERROR)
				return(EPC_ERROR);
		}
		
		else if (part_table[row].comp2_type == String)
		{
			if (dec_comp_string(epc, part_table[row].comp2_len, second_entry_string) == EPC_ERROR)
				return(EPC_ERROR);
		}
		
		else if (part_table[row].comp2_type == PartUnPadInteger)
		{ 
			if (dec_comp_part_unpadded (epc, part_table[row].comp2_len, part_table[row].comp2_dig, second_entry_string) == EPC_ERROR)
				return(EPC_ERROR);
		}
		
		else if (part_table[row].comp2_type == NumString)
		{
			if (dec_comp_NumString_integer(epc, part_table[row].comp2_len, second_entry_string) == EPC_ERROR)
				return(EPC_ERROR);
		}		
		
		else if (part_table[row].comp2_type == Cage)
		{
			if (dec_comp_cage(epc, second_entry_string) == EPC_ERROR)
				return(EPC_ERROR);
		}
			
		else if (part_table[row].comp2_type == String6bit || part_table[row].comp2_type == PartString6bit)
		{
			if (dec_comp_cage6bit_string(epc, 0, part_table[row].comp2_dig, second_entry_string) == EPC_ERROR)
				return(EPC_ERROR);
		}	// more input options can be added later here !!!
		else
		{
			p_printf(RED,"unknown translation request for component 2: %c\n", part_table[row].comp2_type);
			return(EPC_ERROR);
		}
		
	}

	/** get component_3 entry if bits requested*/
	if (part_table[row].comp3_len > 0)
	{
		/* obtain and validate depending type */
		if (part_table[row].comp3_type == PartInteger)
		{ 
			if (dec_comp_part_integer (epc, part_table[row].comp3_len, part_table[row].comp3_dig, third_entry_string) == EPC_ERROR)
				return(EPC_ERROR);
		}
		
		else if (part_table[row].comp3_type == Integer)
		{
			if (dec_comp_integer(epc, part_table[row].comp3_len, third_entry_string) == EPC_ERROR)
				return(EPC_ERROR);
		}
		
		else if (part_table[row].comp3_type == PartString)
		{
			if (dec_comp_string(epc, part_table[row].comp3_len, third_entry_string) == EPC_ERROR)
				return(EPC_ERROR);
		}
		
		else if (part_table[row].comp3_type == String)
		{
			if (dec_comp_string(epc, part_table[row].comp3_len, third_entry_string) == EPC_ERROR)
				return(EPC_ERROR);
		}
		
		else if (part_table[row].comp3_type == PartUnPadInteger)
		{ 
			if (dec_comp_part_unpadded (epc, part_table[row].comp3_len, part_table[row].comp3_dig, third_entry_string) == EPC_ERROR)
				return(EPC_ERROR);
		}
		
		else if (part_table[row].comp3_type == NumString)
		{
			if (dec_comp_NumString_integer(epc, part_table[row].comp3_len, third_entry_string) == EPC_ERROR)
				return(EPC_ERROR);
		}	
		else if (part_table[row].comp3_type == Reserved)
		{
				// nothing to do
		}
		
		else if (part_table[row].comp3_type == Cage)
		{
			if (dec_comp_cage(epc, third_entry_string) == EPC_ERROR)
				return(EPC_ERROR);
		}
		
		else if (part_table[row].comp3_type == String6bit || part_table[row].comp3_type == PartString6bit)
		{
			if (dec_comp_cage6bit_string(epc, 0, part_table[row].comp3_dig, third_entry_string) == EPC_ERROR)
				return(EPC_ERROR);
		} // more input options can be added later here !!!
		else
		{
			p_printf(RED,"unknown translation request for component 3: %c\n", part_table[row].comp3_type);
			return(EPC_ERROR);
		}
	
	}
	
	// create URI
	sprintf(dispbuf,"urn:epc:tag:%%s:%%ld.%%0%dld.", part_table[row].comp1_dig);
	sprintf(buf, dispbuf,part_table[row].scheme,filter,first_entry);

	if (part_table[row].comp2_len > 0)
	{
		if (part_table[row].comp3_type == Reserved ||part_table[row].comp3_type == NotAvailable  )
			sprintf(buf,"%s%s",buf,second_entry_string);
		else
			sprintf(buf,"%s%s.",buf,second_entry_string);
	}
	/* Note that if the “Other Field Digits” column specifies zero, then D must be the empty string, 
	 * implying the overall input segment ends with a “dot” character.*/
	else
		sprintf(buf,"%s.",buf);
	
	// add optional entry
	if (part_table[row].comp3_len > 0 && part_table[row].comp3_type != Reserved )
	{
		sprintf(buf,"%s%s",buf,third_entry_string);
	}
	
	return(EPC_SUCCESS);	
}

/** add a bit for the binary EPC
 *  add a byte to the binary EPC if 8 bits had been added
 * 
 * bit : input either 1 or 0 to set
 * EPC : input overall EPC to add complete bytes
 * 
 */
void enc_add_bit(int bit, uint8_t *epc)
{
	if (bitcnt++ == 8)		// save EPC-BYTE 
	{
		epc[epc_pos++] = epc_byte;
		epc_byte = 0;
		bitcnt = 1;
	}
	
	if (bit)	epc_byte |= 0x1 << (8 - bitcnt);
}

/** 
 * Add int value as integer to binary epc
 *  
 * epc : binary epc to add the bits to
 * val : integer with value to add
 * len : length in bits to add from value
 */
void enc_add_int(uint8_t *epc, int val, int len)
{
	int tmp;

	// keep # bits that needs to remain
	tmp = val << (8 - len);
	
	// add bit by bit
	while (len--)
	{
		enc_add_bit(tmp & 0x80 ? 1 : 0,	 epc);
		tmp <<= 1;
	}
}


/** 
 * Add string value as integer to binary epc
 * 
 * it is assumed that bit length of a single integer is NOT more than 64 bits
 * in the current setup that should NOT be the case as 12 digits  = 42 bits are maximum
 * The 64 bit would support up to 19 (dec) digits
 * 
 * This is different than enc_add_val_integer() as it relates to the validation and padding
 * 
 * epc : binary epc to add to
 * val : value to add
 * len : field length in bits to add from value
 * dig : field length in digits
 * 
 * if dig = 0 , no check on number of digits will be performed, instead 
 * a check on number of bits will be done
 * 
 * No padding is required as digit <=> length bits
 */
int enc_add_val_part_integer(uint8_t *epc, char *val, int len, int dig)
{
	int tmp, bits = len;
	uint64_t val64;

	if (len > 64)
	{
		p_printf(RED,"Error: can only handle up to 64 bits integers. request : %d\n", len);
		return(EPC_ERROR);
	}
	
	/* check on PaddedNumericComponent */
	tmp = strlen(val);
	
	while(tmp--)
	{
		if (val[tmp] < '0' || val[tmp] > '9')
		{
			p_printf(RED,"incorrect integer value : %s\n", val);
			return(EPC_ERROR);
		}
	}
	
	sscanf(val, "%ld", &val64);
	
	/* if dig = 0 , no check on number of digits will be performed, instead 
	 * a check on number of bits will be done */

	if (dig > 0)
	{
		/* The number of digits in D must match the corresponding value 
		 * specified in the “Other Field Digits” column of the matching 
		 * partition table row.*/
		
		tmp = strlen(val);
		
		if( tmp != dig)
		{
			p_printf(RED,"Error: The number of digits not correct. expected %d, got %d (%s)\n", dig, tmp, val);
			return(EPC_ERROR);
		}
	}
	else
	{
		/* The value of the string when considered as a decimal integer must be less than 2^b , 
	     * where b is the value specified in the “Coding Segmen Bit Count” row of the encoding table or
	     * in the “Other Field Digits” column of the matching partition table row */
	 
		if(val64 >= pow(2,(double)len))
		{
			p_printf(RED,"Value is to big. Got %ld, maximum %0f\n", val64,  pow(2,(double)len));
			return(EPC_ERROR);
		}
	}

	// keep # of bits that needs to remain
	val64 = val64 << (64 - len);
	
	// add bit after bit
	while (bits--)
	{
		enc_add_bit(val64 & 0x8000000000000000 ? 1:0, epc);
		val64 <<= 1;
	}
	
	return(EPC_SUCCESS);
}


/** 
 * Add a CAGEC or DODAAC string as 6 x 6-bit characters to binary epc
 * 
 * Consider the input to be a string of five or six characters d 1 d 2 ...d N , where each character d i is
 * a single character. Translate each character to a 6-bit string using Table G-2 (Appendix G).
 * Concatenate those 6-bit strings in the order corresponding to the input. If the input was five characters,
 * prepend the 6-bit value 100000 to the left of the result. The resulting 36-bit string is the output. 
 * 
 * Mainly used by DOD.
 * 
 * CAGECodeOrDODAAC 6 bit code-value is nearly the same as ASCII, but removed the 2 top bits. As a result it only supports
 * a digits and capitals. Also capital I and capital O are not allowed as they could be confused with digit 1 (one) and 0 (zero).
 * The difference between 'cage' and '6-bit string' is in the later 3-character escape sequence is allowed
 * to handle #.
 * 
 * epc : output binary epc to add to
 * val : input value to add
 */

int enc_add_val_cage(uint8_t *epc, char *val)
{
	int i, len = strlen(val);
	
	/* If the input was five characters, prepend the 6-bit value 
	 * 100000 (0x20) to the left */
	if (len == 5)	enc_add_int(epc,0x20, 6);
	
	// check on CAGECodeOrDODAAC
	for (i = 0; i< len ;i++)
	{

		if (val[i] >= '0' || val[i] <= 'Z')
		{
			/* capital I and capital O are not allowed as they could 
			 * could be confused with digit 1 (one) and 0 (zero) */
			if (val[i] != 'I' && val[i] != 'O')
			{		
				// add 6 bits character
				enc_add_int(epc,val[i],6);
				continue;
			}
		}

		p_printf(RED,"incorrect CAGECodeOrDODAAC character: %c in %s\n",val[i], val);
		return(EPC_ERROR);

	}
		
	return(EPC_SUCCESS);
}

/** 
 * Add a string as 6-bit characters to binary
 *  
 * epc  : output binary epc to add the bits to
 * str  : input character string to add
 * min_dig  : input minimum number of digits in string
 * max_dig  : input maximum number of digits in string
 * len		: output number of bits added
 * 
 * Mainly used by DOD.
 * 
 * CAGECodeOrDODAAC 6 bit code-value is nearly the same as ASCII, but removed the 2 top bits. As a result it only supports
 * a digits and capitals. Also capital I and capital O are not allowed as they could be confused with digit 1 (one) and 0 (zero).
 * The difference between 'cage' and '6-bit string' is in the later 3-character escape sequence is allowed
 * to handle #.
 * 
 * This routine can handle the D-component of "6-Bit Variable String Partition Table"
 * min-dig has to be set 0, max_dig to maximum number of digits
 */
 
int enc_add_val_cage6bit_string(uint8_t *epc, char *str, int min_dig, int max_dig)
{
	int len = strlen(str);
	int i, tmp;
		
	/* check it will fit
	 * The number of characters in the input must be greater than or 
	 * equal to the minimum number of characters and less than or equal 
	 * to the maximum number of characters specified in the footnote
	 * to the coding table for this coding table column. For the 
	 * purposes of this rule, an escape triplet (%nn) is counted 
	 * as one character.*/ 
	i=0;
	tmp=0;

	
	// escape sequence (like: %22) counts as 1 (not 3) characters
	while (str[i] != 0x0)  if (str[i++] == '%') tmp++;

	if ((len - (tmp*2) > max_dig) || (len - (tmp*2) < min_dig))
	{
		p_printf(RED,"Error: length Cage / DODAAC string incorrect: min_digits %d, max_digits %d, got number digits %d (%s)\n",
		min_dig, max_dig, len - tmp*2, str);
		return(EPC_ERROR);
	}		 	
	
	// parse input string 
	for (i = 0; i < len ; i++)
	{
		tmp = str[i];
		
		if (tmp == '%')
		{
			i++;
			
			// set next 2 bytes as single character
			if (str[i] > '9')
				tmp = ((str[i++] + '9') & 0x0f) << 4;
			else
				tmp = (str[i++] & 0xf) << 4;
			
			if (str[i] > '9')
				tmp |= (str[i] + '9') & 0xf;
			else
				tmp |= str[i] & 0xf;
		}
	
		// check on CAGECodeOrDODAAC
		if ((tmp >= '0' || tmp <= 'Z') || tmp == '#')
		{
			/* capital I and capital O are not allowed as they could 
			 * be confused with digit 1 (one) and 0 (zero) */
			if (tmp != 'I' && tmp != 'O')
			{		
				// add 6 bits character
				enc_add_int(epc, tmp,6);

				continue;
			}
		}

		p_printf(RED,"incorrect CAGECodeOrDODAAC character: %c in string %s\n",tmp,str);
		return(EPC_ERROR);
	}
	
	// append six zero bits(000000).
	enc_add_int(epc,0,6);

	return(EPC_SUCCESS);
}

/**
 * if the field is unpadded: The number of characters in the URI fields is always less than or equal 
 * to a known limit, and the number of bits in the binary encoding is always a constant number of bits.
 * 
 * epc : binary epc to add to
 * val : value to add
 * len : field length in bits to add from value
 * dig : field length in digits
 */

int enc_add_val_part_unpad_integer(uint8_t *epc, char *val, int len, int dig)
{
	int tmp, bits = len;
	uint64_t val64;

	if (bits > 64)
	{
		p_printf(RED,"Error: can only handle up to 64 bits integers. request : %d\n", bits);
		return(EPC_ERROR);
	}

	tmp = strlen(val);
	
	/* The value of D, considered as a decimal integer, must be less than 2 N , where N is the number
	 * of bits specified in the “other field bits” column of the matching partition table row. */
	
	if (tmp > dig)
	{
		p_printf(RED,"incorrect length unpadded component: expected %d, got %d (%s)\n", dig, tmp , val);
		return(EPC_ERROR);
	} 
	
	// check on NumericComponent
	while(tmp--)
	{
		if (val[tmp] < '0' || val[tmp] > '9')
		{
			p_printf(RED,"incorrect integer value : %s\n", val);
			return(EPC_ERROR);
		}
	}
	
	sscanf(val, "%ld", &val64);

	// keep # of bits that needs to remain
	val64 = val64 << (64 - len);
	
	// add bit after bit
	while (bits--)
	{
		enc_add_bit(val64 & 0x8000000000000000 ? 1:0, epc);
		val64 <<= 1;
	}
	
	return(EPC_SUCCESS);
}


/** 
 * Add character string value to binary
 *  
 * epc  : input binary epc to add the bits to
 * str  : input character string to add
 * bitc : input total number of bits to add.
 * dig  : input max number of digits 
 * 
 * will pad 0x0 to the right in case less characters than
 * bits
 * 
 * This routine can handle the D-component of "String Partition Table"
 * 
 * if dig = 0, a validation of fit on bits, otherwise a check is done
 * that the length of str is less of equal to dig.
 * 
 */
int enc_add_val_string(uint8_t *epc, char *str, int bitc, int dig)
{
	int len = strlen(str);
	int i, tmp, cnt;
	
	// check it will fit 
	i=0;
	cnt=0;
	
	// escape sequence (%22) counts as 1 (not 3) characters
	while (str[i] != 0x0)  if (str[i++] == '%') cnt++;
	
	if (dig > 0)
	{
		/* The number of characters in D must be less than or equal to 
		 * the corresponding value specified in the “Other Field Maximum 
		 * Characters” column of the matching partition table row. For 
		 * the purposes of this rule, an escape triplet (%nn) is counted
		 * as one character.*/
		 			
		if (len - (cnt*2) > dig)
		{
			p_printf(RED,"incorrect length string component: Max %d, got %d (%s)\n",
			dig, len - cnt*2, str);
			return(EPC_ERROR);
		} 
	}
	
	else
	{		
		/* The number of characters must be less than or equal to b/7, where b is the value 
		 * specified in the “Coding Segment Bit Count” row of the coding table */
		if ( len -(cnt*2) > bitc/7)
		{
			p_printf(RED,"Error: length string will not fit bits: expected %d, got %d (%s)\n",
			bitc/7, len - cnt*2, str);
			return(EPC_ERROR);
		}		 	
	}
	
	// parse input string 
	for (i = 0; i < len ; i++)
	{
		tmp = str[i];
		
		if (tmp == '%')
		{
			i++;
						
			/* According to the definition encoding procedure for strings 14.3.5
			 * For each portion of D that matches the Escape production of 
			 * the grammar specified in Section 5 (that is, a 3-character 
			 * sequence consisting of a % character followed by two hexadecimal digits), 
			 * the two hexadecimal characters following the % character must map to one 
			 * of the 82 allowed characters specified in Table 46 ( Appendix A).
			 * 
			 * SO IT SEEMS THAT ALL CHARACTERS CAN BE ESCAPED, BUT THE SPECIAL TRANSLATED
			 * CHARACTERS MUST BE DURING DECODING AND AS INPUT. 
			 * 
			 * Hence NO check on the MUST-do translation only
			 */
			
			// set next 2 bytes as single character
			if (str[i] > '9') 	tmp = ((str[i++] + '9') & 0x0f) << 4;
			else tmp = (str[i++] & 0xf) << 4;
			
			if (str[i] > '9')	tmp |= (str[i] + '9') & 0xf;
			else tmp |= str[i] & 0xf;
		}
		
		// check on GS3A3Component grammer
		if (tmp < 0x21 || tmp > 0x7A)
		{
			p_printf(RED,"String illegal character 0x%02x\n", str[i]);
			return(EPC_ERROR);
		}
		
		// add 7 bits character
		enc_add_int(epc,tmp,7);
	}
	
	// padd to the right with zero bits (if necessary)
	tmp = bitc - ((len - 2*cnt) * 7);

	if (tmp > 0)
	{
		while (tmp--)	enc_add_bit(0, epc);
	}
		
	return(EPC_SUCCESS);
}

/**
 * add zeros for now as the space is reserved
 * 
 * epc : output binary epc to add the bits to
 * len : input length in bits to add to the EPC
 * */
int enc_add_reserved(uint8_t *epc, int len)
{
	int bits = len;
	
	while (bits--) enc_add_bit(0,epc);
	
	return(EPC_SUCCESS);
}
/** 
 * Add string value as integer to epc
 * 
 * The bit length of a single integer can NOT be more than 64 bits
 * in the current setup that should NOT be the case as 12 digits  = 42 bits are maximum
 * The 64 bit would support up to 19 (dec) digits
 * 
 * epc : output binary epc to add the bits to
 * val : input string with value to add
 * len : input length in bits to add to the EPC
 * 
 * This is different than enc_add_val_part_integer() as it relates to 
 * the validation and padding to the left with zero bits as necessary 
 * to make total bits.
 * 
 */
int enc_add_val_integer(uint8_t *epc, char *val, int len)
{
	int tmp, bits;
	uint64_t val64;

	if (len > 64)
	{
		p_printf(RED,"Error: can only handle up to 64 bits integers. request : %d\n", len);
		return(EPC_ERROR);
	}	
	
	tmp = strlen(val);

	// check on valid NumericComponents
	while(tmp--)
	{
		if (val[tmp] < '0' || val[tmp] > '9')
		{
			p_printf(RED,"incorrect integer value : %s\n", val);
			return(EPC_ERROR);
		}
	}
	
	sscanf(val, "%ld", &val64);

	/* The value of the string when considered as a decimal integer must be less than 2^b , 
	 * where b is the value specified in the “Coding Segmen Bit Count” row of the encoding table */
	 
	if(val64 >= pow(2,(double)len))
	{
		p_printf(RED,"Value is to big. Got %ld, maximum %0f\n", val64,  pow(2,(double)len));
		return(EPC_ERROR);
	}
	
	/* padding to the left is required. As bits are loaded in enc_add_bit from
	 * left to right, we need to determine the leading (left) 0's. 
	 * 
	 * First determine number of bits needed for current value 
	 */
	 
	for (bits = 0 ; bits < len ; bits++)
	{ 
		if (val64 < pow(2, (double) bits)) break;
	}

	/* calculate how many bits need to be padded */
	tmp = len - bits;
		
	/* insert the bits first (if needed) == padding to the left */
	while (tmp--)	enc_add_bit(0,epc);

	/* keep the number bits that are needed */
	val64 = val64 << (64 - bits);

	/* shift & add bit after bit of the value */
	while (bits--)
	{
		enc_add_bit(val64 & 0x8000000000000000 ? 1:0, epc);
		val64 <<= 1;
	}

	return(EPC_SUCCESS);
}



/**
 * create a pure-identity URI from binary EPC  
 * epc : input binary epc 
 * buf : output Pure Idendity 
 * len : length of binary epc 
 * 
 * input : 308ED7A809801E0000A12CD3
 * output: urn:epc:id:sgtin:763003032.0120.10562771
 * 
 */

int dec_binTOpure(uint8_t *epc,  char * tag_pure, int len)
{
	char	tag_epc[100];
	
	// bin to tag
	if (dec_binTOtag(epc, tag_epc, len) == EPC_ERROR) return(EPC_ERROR);

	// tag to pure
	return(dec_tagTOpure(tag_epc, tag_pure));
}

/** 
 * add a numeric string to binary
 * epc  : output binary EPC
 * str  : input value
 * bitc : num of bits add
 * dig  : num of digits
 * 
 * leading zeros are preserved by adding a '1' before them
 * 
 * THIS DEFINITION IS VERY BADLY WRITTEN IN THE SPECIFICATION AS
 * GDTI-113 and SGCN-96 are the only schemes that uses this encoding method.
 *
 */

int enc_add_NumString_integer(uint8_t *epc, char *str, int bitc, int dig)
{

	char	*buf;
	int		ret;
	
	int 	len = strlen(str);
	
	/* The number of digits in the string, D, must be such that 2 × 10^D < 2^b , 
	 * where b is the value specified in the “Coding Segment Bit Count” row of 
	 * the encoding table. */
 
	if (2 * (uint64_t) pow(10,len) >= (uint64_t) pow(2,bitc))
	{
		p_printf(RED,"Numeric string has to many digits\n");
		return(EPC_ERROR);
	}
	
	// get enough memory to hold the string
	buf = malloc(len + 2);
	
	// start with a 1 character
	buf[0] = '1';
	
	// add the numeric string
	strcpy(&buf[1], str);

	ret= enc_add_val_integer(epc, buf, bitc);
		
	free(buf);
	return(ret);
}

/** 
 * gid-96 needs special handling as it does NOT use partition table
 * 
 * The format is:
 * 8  bit EPC header (0x35)
 * 28 bit General manager number	(integer)
 * 24 bit object class				(integer)
 * 36 bit serial number				(integer)
 * 
 * No checks etc
 * input : urn:epc:tag:gid-96:31415.271828.1414
 *          
 * output: 350007AB70425D4000000586
 */
int enc_tagTObin_gid(char *p, uint8_t *epc, int *len)
{
	char	buf[20];

	/* add EPC header (8 bits) */
	enc_add_int(epc, 0x35, 8);
	
	//  add general manager
	if ((p = pars_uri_entry(p,'.',buf)) == NULL) return(EPC_ERROR);
	if (enc_add_val_integer(epc, buf, 28) == EPC_ERROR)	return(EPC_ERROR);

	//  add object class
	if ((p = pars_uri_entry(p,'.',buf)) == NULL) return(EPC_ERROR);
	if (enc_add_val_integer(epc, buf, 24) == EPC_ERROR)	return(EPC_ERROR);

	//  add serial
	if ((p = pars_uri_entry(p,0x0,buf)) == NULL) return(EPC_ERROR);
	if (enc_add_val_integer(epc, buf, 36) == EPC_ERROR)	return(EPC_ERROR);
	
	// flush any last byte pending
	if (bitcnt)	epc[epc_pos++] = epc_byte;
	
	*len = epc_pos;
	
	return(EPC_SUCCESS);
}

/** 
 * usdod-96 needs special handling as it does NOT use partition table
 * 
 * 8   bit EPC header (0x2f)
 * 4   bit filter
 * 
 * 48  bit  6 X 8 ASCII CHARACTER, LIKE CAGE/ DoDAAC but THEN 8 BIT
 * 
 * 36  bit. after the serial number is converted into binary format, 
 * 	   it must be left-padded with zeros to 36 bits total. 
 * 
 * input  : urn:epc:tag:usdod-96:3.CAGEY.5678
 * output : 2F320434147455900000162E
 *         
 */
int enc_tagTObin_usdod(char *p, uint8_t *epc, int *len)
{
	char	buf[20];
	int		i,j;
	
	/* add EPC header (8 bits) */
	enc_add_int(epc, 0x2F, 8);
	
	//  add filter
	if ((p = pars_uri_entry(p,'.',buf)) == NULL) return(EPC_ERROR);
	if (enc_add_val_integer(epc, buf, 4) == EPC_ERROR)	return(EPC_ERROR);

	//  add Government Managed Identifier
	if ((p = pars_uri_entry(p,'.',buf)) == NULL) return(EPC_ERROR);
	
	j = strlen(buf);
	
	if (j > 6)
	{
		p_printf(RED,"incorrect USDOD length. Maximum 6 character : %s\n", buf);
		return(EPC_ERROR);
	}

	// prepend with spaces if needed
	i = 6 -j;
	
	while (i-- > 0 )  enc_add_int(epc, 0x20, 8);
	
	// add identifier
	for (i = 0; i < j ; i++)
	{
		// validate character
		if (buf[i] < '0' || buf[i] > 'Z') 
		{
			p_printf(RED,"incorrect USDOD character %c in %s\n", buf[i], buf);
			return(EPC_ERROR);
		}

		// add valid character
		enc_add_int(epc,buf[i],8);
	}

	//  add serial number
	if ((p = pars_uri_entry(p,0x0,buf)) == NULL) return(EPC_ERROR);
	if (enc_add_val_integer(epc, buf, 36) == EPC_ERROR)	return(EPC_ERROR);
	
	// flush any last byte pending
	if (bitcnt)	epc[epc_pos++] = epc_byte;
	
	*len = epc_pos;
	
	return(EPC_SUCCESS);
}

/** 
 * adi-var needs special handling as it does NOT use partition table
 * 
 * 8   bit EPC header (0x3b)
 * 6   bit filter
 * 36  bit CAGE/ DoDAAC            6-bit CAGE/DoDAAC
 * 6 - 198 bit (MAX) Part Number   6-bit Variable String
 * 12 -186 bit (MAX) Serial Number 6-bit Variable String
 * 
 * part number digit   0 >= #  <=32
 * serial number digit 1 >= #  <=30
 * 
 * The “#” character (represented in the URI by the escape sequence %23)
 * may appear as the first character of the Serial Number segment, but 
 * otherwise may not appear in the Part Number segment or elsewhere in 
 * the Serial Number segment.
 * 
 * input  : urn:epc:tag:adi-var:3.35962.PQ7VZ4.M37GXB92
 * output : 3B0E0CF5E76C9047759AD00373DC7602E72000
 *         
 */

int enc_tagTObin_adi(char *p, uint8_t *epc, int *len)
{
	char	buf[20];
	
	/* add EPC header (8 bits) */
	enc_add_int(epc, 0x3B, 8);
	
	//  add filter
	if ((p = pars_uri_entry(p,'.',buf)) == NULL) return(EPC_ERROR);
	if (enc_add_val_integer(epc, buf, 6) == EPC_ERROR)	return(EPC_ERROR);

	//  add cage
	if ((p = pars_uri_entry(p,'.',buf)) == NULL) return(EPC_ERROR);
	if (enc_add_val_cage(epc, buf) == EPC_ERROR)	return(EPC_ERROR);

	//  add part number
	if ((p = pars_uri_entry(p,'.',buf)) == NULL) return(EPC_ERROR);
	if (enc_add_val_cage6bit_string(epc, buf,0, 32) == EPC_ERROR)	return(EPC_ERROR);

	//  add serial number
	if ((p = pars_uri_entry(p,0x0,buf)) == NULL) return(EPC_ERROR);
	if (enc_add_val_cage6bit_string(epc, buf,1, 30) == EPC_ERROR)	return(EPC_ERROR);
	
	// flush any last byte pending
	if (bitcnt)	epc[epc_pos++] = epc_byte;
	
	*len = epc_pos;
	
	return(EPC_SUCCESS);
}

/**
 * encode a tag URI to binary EPC  
 * uri : input tag URI 
 * epc : output binary epc to return
 * len : output number of bytes in binary EPC.
 * 
 * example
 * input  : urn:epc:tag:sgtin-96:0.763003032.0120.10562771
 * output : 308ED7A809801E0000A12CD3
 */

int enc_tagTObin(char *uri, uint8_t *epc, int *len)
{
	char 	*p;
	char 	buf[30];
	char 	scheme[10];		// holds the scheme
	char	filter[10];		// holds filter value
	char	C_entry [15];	// holds comp1_ entry
	char	uristart[]= "urn:epc:tag:";
	uint16_t row;
	
	// detect correct format URL and start
	if ((p = stristr(uri, uristart)) == NULL)
	{
		p_printf(RED,"incorrect URI : %s. stopping\n", uri);
		return(EPC_ERROR);
	}
	
//TODO: check on 64 bits as that is coded differently  !!		
	
	// initialize values
	p += strlen(uristart); 
	epc_byte = epc_pos = bitcnt= 0x0;

	/* get scheme & filter & C_entry */
	if ((p = pars_uri_entry(p,':',scheme)) == NULL) return(EPC_ERROR);
				
	// gid-96 requires a different encoding
	if(strcmp(scheme,"gid-96") == 0) 
		return(enc_tagTObin_gid(p, epc, len));
		
	// adi-var requires a different encoding
	else if(strcmp(scheme,"adi-var") == 0)
		return(enc_tagTObin_adi(p, epc, len));
	
	// usdod requires a different encoding
	else if(strcmp(scheme,"usdod-96") == 0)
		return(enc_tagTObin_usdod(p, epc, len));
	
	/* get filter & C_entry */
	if ((p = pars_uri_entry(p,'.',filter)) == NULL) return(EPC_ERROR);
	if ((p = pars_uri_entry(p,'.',C_entry)) == NULL) return(EPC_ERROR);

	/* find a partition row for the scheme, based on C length) */
	if ((row = epc_scheme_lkup(scheme, strlen(C_entry))) == 0xff)
	{
		p_printf(RED,"Scheme(%s) in combination with length C (%d)unknown. stopping\n", scheme, (int) strlen(C_entry));
		return(EPC_ERROR);
	}		
	
	/* if valid, but not implemented (yet) */
	if (part_table[row].bits == 0)
	{
		p_printf(RED,"Scheme not implemented yet. %s. stopping\n", scheme);
		return(EPC_ERROR);
	}

	/* add EPC header (8 bits) */
	enc_add_int(epc, part_table[row].header, 8);

	/* add filter bits, if requested */
	if (part_table[row].filterlen > 0)
	{
		if (enc_add_val_integer(epc, filter, part_table[row].filterlen))
			return(EPC_ERROR);
	}

	/* add partition table row, 3 bits*/
	enc_add_int(epc, part_table[row].partrow, 3);

	/* add component 1 entry / GS1 company . Always Partition integer */
	if (enc_add_val_part_integer(epc, C_entry, part_table[row].comp1_len, part_table[row].comp1_dig))
		return(EPC_ERROR);

	// add component 2 entry
	if (part_table[row].comp2_len > 0)
	{
		if (part_table[row].comp3_type != Reserved && part_table[row].comp3_type != NotAvailable)
		{
			// obtain component2
			if ((p = pars_uri_entry(p,'.',buf)) == NULL) return(EPC_ERROR);
		}
		else
		{
			// obtain component2
			if ((p = pars_uri_entry(p,0x0,buf)) == NULL) return(EPC_ERROR);
		}

		// add component-2 depending on required translation
		if (part_table[row].comp2_type == PartUnPadInteger)
		{
			if (enc_add_val_part_unpad_integer(epc, buf, part_table[row].comp2_len, part_table[row].comp2_dig) == EPC_ERROR)
				return(EPC_ERROR);
		}
		
		else if (part_table[row].comp2_type == PartInteger)
		{
			if (enc_add_val_part_integer(epc, buf, part_table[row].comp2_len, part_table[row].comp2_dig) == EPC_ERROR)
				return(EPC_ERROR);
		}
		
		else if (part_table[row].comp2_type == PartString)
		{
			if (enc_add_val_string(epc, buf, part_table[row].comp2_len, part_table[row].comp2_dig) == EPC_ERROR)	
				return(EPC_ERROR);
		}
		
		else if (part_table[row].comp2_type == Integer)
		{
			if (enc_add_val_integer(epc, buf, part_table[row].comp2_len) == EPC_ERROR)
				return(EPC_ERROR);
		}

		else if (part_table[row].comp2_type == String)
		{
			if (enc_add_val_string(epc, buf, part_table[row].comp2_len, 0) == EPC_ERROR)
				return(EPC_ERROR);
		}
		
		else if (part_table[row].comp2_type == Cage)
		{	
			if (enc_add_val_cage(epc, buf) == EPC_ERROR) return(EPC_ERROR);
		}
		
		else if (part_table[row].comp2_type == String6bit || part_table[row].comp2_type == PartString6bit)
		{	
			if (enc_add_val_cage6bit_string(epc, buf, 0, part_table[row].comp2_dig) == EPC_ERROR)
				return(EPC_ERROR);
		}
		else if (part_table[row].comp2_type == NumString)
		{	
			if (enc_add_NumString_integer(epc, buf, part_table[row].comp2_len, part_table[row].comp2_dig) == EPC_ERROR)
				return(EPC_ERROR);
		}

		else
		{
			p_printf(RED,"Value translation %d not implemented for component 2. stopping\n", part_table[row].comp2_type);
			return(EPC_ERROR);
		}

	}

	// add component 3 entry
	if (part_table[row].comp3_len > 0)
	{
		if (part_table[row].comp3_type != Reserved && part_table[row].comp3_type != NotAvailable)
		{
			// obtain component3
			if ((p = pars_uri_entry(p,0x0,buf)) == NULL) return(EPC_ERROR);
		}
		
		// add component-3 depending on required translation
		if (part_table[row].comp3_type == PartUnPadInteger)
		{
			if (enc_add_val_part_unpad_integer(epc, buf, part_table[row].comp3_len, part_table[row].comp3_dig) == EPC_ERROR)
				return(EPC_ERROR);
		}
		
		else if (part_table[row].comp3_type == PartInteger)
		{
			if (enc_add_val_part_integer(epc, buf, part_table[row].comp3_len, part_table[row].comp3_dig) == EPC_ERROR)
				return(EPC_ERROR);
		}
		
		else if (part_table[row].comp3_type == PartString)
		{
			if (enc_add_val_string(epc, buf, part_table[row].comp3_len, part_table[row].comp3_dig) == EPC_ERROR)	
				return(EPC_ERROR);
		}
		
		else if (part_table[row].comp3_type == Integer)
		{
			if (enc_add_val_integer(epc, buf, part_table[row].comp3_len) == EPC_ERROR)
				return(EPC_ERROR);
		}

		else if (part_table[row].comp3_type == String)
		{
			if (enc_add_val_string(epc, buf, part_table[row].comp3_len, 0) == EPC_ERROR)
				return(EPC_ERROR);
		}
	
		else if (part_table[row].comp3_type == Cage)
		{	
			if (enc_add_val_cage(epc, buf) == EPC_ERROR) return(EPC_ERROR);
		}
		
		else if (part_table[row].comp3_type == String6bit || part_table[row].comp3_type == PartString6bit)
		{	
			if (enc_add_val_cage6bit_string(epc, buf, 0, part_table[row].comp3_dig) == EPC_ERROR)
				return(EPC_ERROR);
		}
		else if (part_table[row].comp3_type == NumString)
		{	
			if (enc_add_NumString_integer(epc, buf, part_table[row].comp3_len, part_table[row].comp3_dig) == EPC_ERROR)
				return(EPC_ERROR);
		}
		
		else if (part_table[row].comp3_type == Reserved)
		{	
			if (enc_add_reserved(epc, part_table[row].comp3_len) == EPC_ERROR)
				return(EPC_ERROR);
		}
		
		else if (part_table[row].comp3_type == NotAvailable)
		{	
			// nothing to do
		}
		else
		{
			p_printf(RED,"Value translation '%c' not implemented for component 3. stopping\n", part_table[row].comp3_type);
			return(EPC_ERROR);
		}

	}

	/* end close out */
	
	// flush any last byte pending
	if (bitcnt)	 epc[epc_pos++] = epc_byte;

	*len = epc_pos;
	
	return(EPC_SUCCESS);
}

/**
 * 
 * gdti special
 * 
 * barcode  : input
 * pure_epc : output
 * GS1len	: input
 * 
 * example:
 * input : (253) 4012345987652ABCDefgh012345678
 * output: urn:epc:id:gdti:4012345.98765.ABCDefgh012345678
 */
int enc_barcTOpure_gdti(char *barcode, char *pure_epc, int GS1_len)
{
	int	i,j, mul, digit, skip;
	char comp[13], serial[13];
	int len =strlen(barcode);
	
	digit=skip=i=j=0;
	mul = 1;		
	
	while (j < 13)
	{
		// first skip (253),
		if (barcode[i] == '(')	    skip++;
		
		else if (barcode[i] == ')')	skip++;
	
		// save barcode if not in skip (2) and not space
		else if (skip == 2  && barcode[i] != 0x20)
		{
			if (j == GS1_len) comp[j++] = '.';
			
			comp[j++] = barcode[i];
	
			digit += (barcode[i] - '0') * mul;
			
			if (mul == 3) mul = 1;
			else mul = 3;
		}
		
		i++;
		
		if (i > len)
		{
			p_printf(RED,"can not parse scgn barcode\n");
			return(EPC_ERROR);
		}
	}
	
	comp[j] = 0x0;
	
	// calculate check digit
	digit = digit%  10;
	if (digit) digit = 10 - digit;

	// validate
	if (digit != barcode[i++] - '0')
	{
		p_printf(RED,"invalid gcn barcode\n");
		return(EPC_ERROR);
	}

	j=0;
	
	while(barcode[i])
	{
		serial[j++] = barcode[i++];
	}

	serial[j] = 0x0;
	
		// create pure-identity URI
	sprintf(pure_epc, "urn:epc:id:gdti:%s.%s", comp, serial);
	
	return(EPC_SUCCESS);
}

/** 
 *  sgln special
 * 
 *  barc     : input
 *  pure-epc : output
 *  GS_len   : input company code length
 * 
 * example        
 *  input :  (414) 0614141123452 (254) 5678
 *  output : urn:epc:id:sgln:0614141.12345.5678
 * 
 */

int enc_barcTOpure_sgln(char *barcode, char *pure_epc, int GS1_len)
{
	int	i,j, mul, digit, skip;
	char comp[13], serial[13], check, *p, *s;
	int len = strlen(barcode);
	
	digit=skip=i=j=0;
	mul = 1;		
	
	p = comp;
	s = serial;
	
		
	// create barcode
	while (i < len)
	{
		// first skip (414) and later (254)
		if (barcode[i] == '(')	    skip++;
		else if (barcode[i] == ')')	skip++;

		// save barcode if not in skip (2) and not space
		else if (skip == 2  && barcode[i] != 0x20)
		{
			 if (j++ < 12)
			 {
			 
				*p++ =  barcode[i];
			 
				 digit += (barcode[i] - '0') * mul;
			
				 if (mul == 3) mul = 1;
				 else mul = 3;
			}
			else
				check = barcode[i];
		}
		
		// save serial if not in skip (4) and not space
		else if (skip == 4 &&  barcode[i] != 0x20) 
		{
			if ( barcode[i] == '"' || barcode[i] == '&' || barcode[i] == '%' || barcode[i] == '/')
			{
				*s++ = '%';
				sprintf(s,"%02X", barcode[i]);
				s +=2;
			}
			else
				*s++ =  barcode[i];
		}
		
		i++;
	}

	// terminate strings
	*p = 0x0;
	*s = 0x0;
	
	// calculate check digit
	digit = digit%  10;
	if (digit) digit = 10 - digit;

	// validate
	if (digit != check - '0')
	{
		p_printf(RED,"invalid sgln barcode\n");
		return(EPC_ERROR);
	}

	// create pure-identity URI
	sprintf(pure_epc, "urn:epc:id:sgln:%s.%s", comp, serial);
	
	return(EPC_SUCCESS);
}


/** 
 * sscc special 
 * 
 *  barc     : input
 *  pure-epc : output
 *  GS_len   : input company code length
 * 
 * example        
 *  input :  (00) 106141412345678908
 *  output : urn:epc:id:sscc:0614141.1234567890
 * 
 */

int enc_barcTOpure_sscc(char *barcode, char *pure_epc, int GS1_len)
{
	int	i,j, mul, digit, skip;
	char comp[13];
	int len = strlen(barcode);
	
	digit=skip=i=j=0;
	mul = 3;		
	
	char save_char=0x0; 
		
	// create barcode
	while (j < 18 && i < len)
	{
		// first skip (00),
		if (barcode[i] == '(')	    skip++;
		else if (barcode[i] == ')')	skip++;
	
		// save barcode if not in skip (2) and not space
		else if (skip == 2 && barcode[i] != 0x20)
		{
			if (save_char == 0) save_char = barcode[i];
			else
			{
				if (j == GS1_len)
				{
					 comp[j++] = '.';
					 comp[j++] = save_char;
				}
				
				comp[j++] = barcode[i];
			}
			
			digit += (barcode[i] - '0') * mul;
		
			if (mul == 3) mul = 1;
			else mul = 3;	
		}
	
		i++;
	}
	
	comp[j] = 0x0;
	
	// calculate check digit
	digit = digit%  10;
	if (digit) digit = 10 - digit;

	// validate
	if (digit != barcode[i] - '0')
	{
		p_printf(RED,"invalid sscc barcode\n");
		return(EPC_ERROR);
	}

		// create pure-identity URI
	sprintf(pure_epc, "urn:epc:id:sscc:%s", comp);
	
	return(EPC_SUCCESS);
}

/**
 * gsrn special
 * 
 * barcode  : input
 * pure_epc : output
 * GS1len	: input
 * 
 * example:
 * input (8017) 061414112345678902 or  (8018) 061414112345678902
 * 
 * output 
 * 	8017 urn:epc:id:gsrnp:0614141.1234567890
 *  8018 urn:epc:id:gsrn:0614141.1234567890
 */
int enc_barcTOpure_gsrn(char *barcode, char *pure_epc, int GS1_len, char *ai)
{
	int	i,j, mul, digit, skip;
	char comp[13];
	int len =strlen(barcode);
	
	digit=skip=i=j=0;
	mul = 3;
	
	while (j < 18)
	{
		// first skip (8017 / 8018),
		if (barcode[i] == '(')	    skip++;
		else if (barcode[i] == ')')	skip++;
	
		// save barcode if not in skip and not space
		else if (skip == 2 && barcode[i] != 0x20)
		{
			if (j == GS1_len) comp[j++] = '.';
		
			comp[j++] = barcode[i];
	
			digit += (barcode[i] - '0') * mul;
			
			if (mul == 3) mul = 1;
			else mul = 3;
		}
		
		i++;
		
		if (i > len)
		{
			p_printf(RED,"can not parse gsrn barcode\n");
			return(EPC_ERROR);
		}
	}
	
	comp[j] = 0x0;
	
	// calculate check digit
	digit = digit%  10;
	if (digit) digit = 10 - digit;

	// validate
	if (digit != barcode[i++] - '0')
	{
		p_printf(RED,"invalid gsrn barcode\n");
		return(EPC_ERROR);
	}

	j=0;
	
	if (strcmp(ai,"8017") == 0)
		// create pure-identity URI (GSRNP)
		sprintf(pure_epc, "urn:epc:id:gsrnp:%s", comp);
	
	else if (strcmp(ai,"8018") == 0)
	 // create pure-identity URI (GSRN)
		sprintf(pure_epc, "urn:epc:id:gsrn:%s", comp);

		
	return(EPC_SUCCESS);
}


/** 
 *  cpi special
 *  
 *  8010 : company code (GS1-LEN) + CPID reference number =< 30
 *  8011 : CPID serial =< 12 character 
 * 
 *  barcode  : input
 *  pure-epc : output
 *  GS_len   : input company code length
 * 
 * example       
 *  input : (8010) 061414198765 (8011) 12345
 *  output : urn:epc:id:cpi:0614141.98765.12345
 * 
 */

int enc_barcTOpure_cpi(char *barcode, char *pure_epc, int GS1_len)
{
	int	 i,j,len, skip;
	char comp[50], serial[13], *s;

	// initialize values
	skip=i=j=0;
	s = serial;
	len =strlen(barcode);	
	
	for (i = 0 ; i < len; i++)
	{
		// skip (8010) and (8011)
		if (barcode[i] == '(')	    skip++;
		else if (barcode[i] == ')')	skip++;
	
		// save barcode after 8010 and not space
		else if (skip == 2  && barcode[i] != 0x20)
		{
			if (j == GS1_len) comp[j++] = '.';
			
			// check on valid digit
			if (barcode[i] < '0' || barcode[i] > 'Z')
			{
				if (barcode[i] != '#' && barcode[i] != '-' && barcode[i] != '/')
				{
					p_printf(RED,"illegal character '%c' cpi-barcode: %s\n",barcode[i], barcode);
					return(EPC_ERROR);
				}
				else
				{
					comp[j++] = '%';
					sprintf(&comp[j],"%02X", barcode[i]);
					j += 2;
				}
			}
			
			else
				comp[j++] = barcode[i];
		}

		// save barcode serial after (8011) and not space
		else if (skip == 4  && barcode[i] != 0x20)
		{
			*s++ = barcode[i];
		}

	}
	
	*s = comp[j] = 0x0;
	
	// create pure-identity URI
	sprintf(pure_epc, "urn:epc:id:cpi:%s.%s", comp, serial);
	
	return(EPC_SUCCESS);
}

/** 
 *  grai special
 *  
 *  barc     : input
 *  pure-epc : output
 *  GS_len   : input company code length
 * 
 * example        
 *  input : (8003) 006141411234525678 or (8003) 0061414112345232a/b
 *  output : 
 *  urn:epc:id:grai:0614141.12345.5678 or
 *  urn:epc:id:grai:0614141.12345.32a%2Fb
 */

int enc_barcTOpure_grai(char *barcode, char *pure_epc, int GS1_len)
{
	int	i,j, mul, digit, skip, skipped_zero=0;
	char comp[13], serial[13], *s;
	int len =strlen(barcode);
	
	digit=skip=i=j=0;
	mul = 1;		
	
	while (j < 13)
	{
		// first skip (8003),
		if (barcode[i] == '(')	    skip++;
		else if (barcode[i] == ')')	skip++;
	
		// save barcode if not in skip (2) and not space
		else if (skip == 2  && barcode[i] != 0x20)
		{
			if (skipped_zero++)
			{
				if (j == GS1_len) comp[j++] = '.';
				
				comp[j++] = barcode[i];
		
				digit += (barcode[i] - '0') * mul;
				
				if (mul == 3) mul = 1;
				else mul = 3;
			}
		}
		
		i++;
		
		if (i > len)
		{
			p_printf(RED,"can not parse grai barcode\n");
			return(EPC_ERROR);
		}
	}
	
	comp[j] = 0x0;
	
	// calculate check digit
	digit = digit%  10;
	if (digit) digit = 10 - digit;

	// validate
	if (digit != barcode[i++] - '0')
	{
		p_printf(RED,"invalid grai barcode\n");
		return(EPC_ERROR);
	}

	j=0;
	
	s=serial;
	
	while(barcode[i])
	{
			if ( barcode[i] == '"' || barcode[i] == '&' || barcode[i] == '%' || barcode[i] == '/')
			{
				*s++ = '%';
				sprintf(s,"%02X", barcode[i]);
				s +=2;
			}
			else
				*s++ =  barcode[i];
			
			i++;
	}

	*s = 0x0;
	
		// create pure-identity URI
	sprintf(pure_epc, "urn:epc:id:grai:%s.%s", comp, serial);
	
	return(EPC_SUCCESS);
}

/** 
 *  giai special
 *  
 *  barc 	 : input
 *  pure-epc : output
 *  GS_len   : input company code length
 * 
 * example        
 *  input : 
 * 		(8004) 06141415678 or 
 * 		(8004) 0061414132a/b
 * 
 *  ouput :
 *  	urn:epc:id:giai:0614141.5678 or
 *  	urn:epc:id:giai:0614141.32a%2Fb
 */

int enc_barcTOpure_giai(char *barcode, char *pure_epc, int GS1_len)
{
	int	i,j, skip;
	char comp[13];
	int len = strlen(barcode);
	
	skip=i=j=0;
	
	while (i < len)
	{
		// first skip (8004),
		if (barcode[i] == '(')	    skip++;
		else if (barcode[i] == ')')	skip++;
	
		// save barcode if not in skip (2) and not space
		else if (skip == 2  && barcode[i] != 0x20)
		{
			if (j == GS1_len) comp[j++] = '.';

			if ( j >  GS1_len)
			{
				if ( barcode[i] == '"' || barcode[i] == '&' || barcode[i] == '%' || barcode[i] == '/')
				{
					comp[j++] = '%';
					sprintf(&comp[j],"%02X", barcode[i]);
					j +=2;

					i++;
					continue;
				}
			}

			comp[j++] = barcode[i];
		}
		
		i++;
	}
	
	comp[j] = 0x0;

	// create pure-identity URI
	sprintf(pure_epc, "urn:epc:id:giai:%s", comp);
	
	return(EPC_SUCCESS);
}

/** 
 *  gcn special 
 * 
 * 	1- 12 company code
 *  13 check digit
 *  14 - 25 serial
 * 
 *  barc 	 : input
 *  pure-epc : output
 *  GS_len   : input company code length
 * 
 * example        
 *  input : (255) 401234567890104711
 *  output : urn:epc:id:sgcn:4012345.67890.04711
 * 
 */

int enc_barcTOpure_gcn(char *barcode, char *pure_epc, int GS1_len)
{
	int	i,j, mul, digit, skip;
	char comp[13], serial[13];
	int len =strlen(barcode);
	
	digit=skip=i=j=0;
	mul = 1;		//gtin-13
	
	while (j < 13)
	{
		// first time skip potential (255),
		if (barcode[i] == '(')	    skip++;
		
		else if (barcode[i] == ')')	skip++;
	
		// save barcode if not in skip (0 or 2) and not space
		else if ((skip == 2 || skip == 0) && barcode[i] != 0x20)
		{
			if (j == GS1_len) comp[j++] = '.';
			
			comp[j++] = barcode[i];
	
			digit += (barcode[i] - '0') * mul;
			
			if (mul == 3) mul = 1;
			else mul = 3;
		}
		
		i++;
		
		if (i > len)
		{
			p_printf(RED,"can not parse scgn barcode\n");
			return(EPC_ERROR);
		}
	}
	
	comp[j] = 0x0;
	
	// calculate check digit
	digit = digit%  10;
	if (digit) digit = 10 - digit;

	// validate
	if (digit != barcode[i++] - '0')
	{
		p_printf(RED,"invalid sgcn barcode\n");
		return(EPC_ERROR);
	}

	j=0;
	
	while(barcode[i])
	{
		serial[j++] = barcode[i++];
	}

	serial[j] = 0x0;
	
		// create pure-identity URI
	sprintf(pure_epc, "urn:epc:id:sgcn:%s.%s", comp, serial);
	
	return(EPC_SUCCESS);
}
/**
 * Encode a barcode to pure-identity uri
 * 
 * barc    : input the barcode
 * epc     : output pure URI
 * GS1_len : length of company code
 * serial  : serial number to add (NULL if not needed)
 * 
 * example :
 * 
 * input: 
 * 	barcode 07630030321207 
 *  company code length (assume 9)
 *  serial num to add   (assume serial number 00000)
 * 
 * output: urn:epc:id:sgtin:763003032.0120.00000
 *  
 * OR
 * 
 * input: (01) 07630039685614 (21) 6568798
 * output: urn:epc:id:sgtin:763003968.0561.6568798
 * 
 * The serial number in input string will overrule any potential 
 * provided serial number to the call.
 */

int enc_barcTOpure(char *barc, char *pure_epc, int GS1_len, char * serial)
{
	int 	i, j, odd = 0, even = 0, digit, skip=0;
	char	comp[25], barcode[50], *p, *s;
	char	serie[15];
	
	if (strlen(barc) == 0)
	{
		strcpy(pure_epc, "empty barcode. can not translate");
		return(EPC_SUCCESS);
	}
	
	// copy provided serial number (if any)
	strncpy(serie, serial,15);
	
	p = barcode;
	s = serie;
	
	for (i = 0 , j = 0; i < strlen(barc); i ++)
	{
		if (barc[i] == '(')	        skip++;
		else if (barc[i] == ')')
		{
			comp[j] = 0x0;
			
			//SGCEN
			if (strcmp(comp, "255") == 0)
				return(enc_barcTOpure_gcn(barc, pure_epc, GS1_len));

			//GRAI
			if (strcmp(comp, "8003") == 0)
				return(enc_barcTOpure_grai(barc, pure_epc, GS1_len));
			
			//GIAI
			if (strcmp(comp, "8004") == 0)
				return(enc_barcTOpure_giai(barc, pure_epc, GS1_len));
			
			// CPID
			if (strcmp(comp, "8010") == 0)
				return(enc_barcTOpure_cpi(barc, pure_epc, GS1_len));
		
			// GSRNP /GSRN
			if (strcmp(comp, "8017") == 0 || strcmp(comp, "8018") == 0)
				return(enc_barcTOpure_gsrn(barc, pure_epc, GS1_len, comp));
		
			// GDTI
			if (strcmp(comp, "253") == 0)
				return(enc_barcTOpure_gdti(barc, pure_epc, GS1_len));
			
			// SSCC
			if (strcmp(comp, "00") == 0 )
				return(enc_barcTOpure_sscc(barc, pure_epc, GS1_len));
			
			// SGLN
			if (strcmp(comp, "414") == 0 )
				return(enc_barcTOpure_sgln(barc, pure_epc, GS1_len));		
		}
		
		else if (skip == 1)
		{
			comp[j++] = barc[i];
			comp[j] = 0x0;
		}
	
	}
	/** 
	 * 
	 *  GTIN / SGTIN	
	 */
	skip =0;
	// parse potential  (1) 046232343546 (21) 2345234
	// to:046232343546 and seriestring:2345234
	for (i = 0 ; i < strlen(barc); i ++)
	{
		// first time skip potential (1), later skip potential (21)
		if (barc[i] == '(')	        skip++;
		else if (barc[i] == ')')	skip++;
	
		// save barcode if not in skip (0 or 2) and not space
		else if ((skip == 2 || skip == 0) && barc[i] != 0x20) *p++ =  barc[i];
		
		// save potential serial if not in skip (4) and not space
		else if (skip == 4 &&  barc[i] != 0x20) 
		{
			if ( barc[i] == '"' || barc[i] == '&' || barc[i] == '%' || barc[i] == '/')
			{
				*s++ = '%';
				sprintf(s,"%02X", barc[i]);
				s +=2;
			}
			else
				*s++ =  barc[i];
		}
	}

	// terminate strings
	*p = 0x0;
	*s = 0x0;
	
	i = strlen(barcode);	
	// GTN-8     GTN-12    GTN-13/GLN   GTN-14    SSCC 
	if(i != 8 && i != 12 && i != 13 && i != 14 && i != 18)
	{
		p_printf(RED,"invalid number of digits input : %d\n", i);
		return(EPC_ERROR);
	}

	if(GS1_len > i - 1)
	{
		p_printf(RED,"%d not enough digits for company-id length (%d)\n", i, GS1_len);
		return(EPC_ERROR);
	}
	
	char	save_char=0x0; 	
	p = comp;
	
	/* copy and validate check digit */
	for(j=0; j< i-1; j++)
	{
		// if company code complete
		if (j == GS1_len + 1)
		{
			 *p++ = '.';
			 *p++ = save_char;
		}

		// save first character
		if (save_char == 0x0) save_char = barcode[j];
		else	*p++ = barcode[j];
		
		// get odd and even number
		if (j+1 == 1 || (j+1)%2) odd +=barcode[j] -'0';
		else even +=barcode[j] -'0';
	}
	
	// calculate check digit
	digit = ((odd*3) + even) % 10;
	if (digit) digit = 10 - digit;

	// validate
	if (digit != barcode[i-1] - '0')
	{
		p_printf(RED,"Invalid check digit. Expected %d, received %c\n",digit,  barcode[i-1]);
		return(EPC_ERROR);
	}
	
	*p = 0x0;
	
	// create pure-identity URI
	sprintf(pure_epc, "urn:epc:id:sgtin:%s.%s", comp, serie);

	return(EPC_SUCCESS);
}

/**
 * create binary EPC from an EPC (autodetect)
 * 
 * epc     : input EPC
 * bin_EPC : output binary EPC
 * filter  : input filter value	(needed in case of pure-identity)
 * bits    : input tagsize in bits (needed in case of pure-identity)
 * len     : output length of binary EPC
 * 
 * 
 * input : 
 * 		pure_EPC  urn:epc:id:sgtin:763003032.0120.10562771
 * 		filter   0		(example: no filter)
 *      bits     96		(example: taglength is 96 bits)
 *  
 * 		or
 * 
 * 		urn:epc:tag:sgtin-96:0.763003032.0120.10562771
 * 
 * 		filter  ignored
 * 		bits	ignored
 * 
 * output : 300ED7A809801E0000A12CD3
 * 
 */
int enc_epcTObin(char *epc, uint8_t *bin_epc, int filter, int bits, int *len)
{
	char	purestart[]= "urn:epc:id:";
	char	tagstart[]= "urn:epc:tag:";

	// detect pure-identity URL at start
	if (stristr(epc, purestart) != NULL)
	{
		return(enc_pureTObin(epc, bin_epc, filter, bits, len));
	}
	
	// detect tag-encoding URL at start
	else if (stristr(epc, tagstart) != NULL)
	{
		return(enc_tagTObin(epc, bin_epc, len));
	}

	p_printf(RED,"EPC not recognized %s\n", epc);
	return(EPC_ERROR);
}

/**
 * create binary EPC from Pure EPC
 * 
 * pure_epc : input pure EPC
 * bin_EPC  : output bianry EPC
 * filter   : input filter value
 * bits     : input tagsize in bits
 * len      : output length of binary EPC
 * 
 * input : 
 * 		pure_EPC  urn:epc:id:sgtin:763003032.0120.10562771
 * 		filter   4
 *      bits     96
 *  
 * output : 308ED7A809801E0000A12CD3
 */

int enc_pureTObin(char *pure_epc, uint8_t *bin_epc, int filter, int bits, int *len)
{
	char	tag_epc[100];
	
	if (enc_pureTOtag(pure_epc, tag_epc, filter, bits) == EPC_ERROR) return(EPC_ERROR);

	return(enc_tagTObin(tag_epc, bin_epc, len));
}

/**
 * create tag-encoding EPC from pure-identity EPC
 * pure_epc : input pure EPC
 * tag_EPC  : output tag EPC
 * filter   : input filter value
 * bits     : input tagsize in bits (if 1 then -var is appended)
 * 
 * input : 
 * 		pure_epc  urn:epc:id:sgtin:763003032.0120.10562771
 * 		filter   4
 *      bits     96
 *  
 * output : 
 * 		tag_epc   urn:epc:tag:sgtin-96:4.763003032.0120.10562771
 * 
 */
int enc_pureTOtag(char *pure_epc, char *tag_epc, int filter, int bits)
{
	char *p = pure_epc;
	char  scheme[15];
	
	char	uristart[]= "urn:epc:id:";
	
	// detect correct format URL at start
	if ((p = stristr(pure_epc, uristart)) == NULL)
	{
		p_printf(RED,"incorrect pure URI : %s. stopping\n", pure_epc);
		return(EPC_ERROR);
	}
		
	if (filter > 7)
	{
		p_printf(RED,"invalid filter value %d\n", filter);
		return(EPC_ERROR);
	}
			
	// initialize values
	p += strlen(uristart); 
	
	// get scheme
	if ((p = pars_uri_entry(p,':',scheme)) == NULL) return(EPC_ERROR);
	
	// Gid does not have filter
	if (strcmp(scheme,"gid") == 0)
	{
		// add uri to tag_epc
		sprintf(tag_epc, "urn:epc:tag:%s-96:%s",scheme, p);
	}

	// adi-var
	else if (strcmp(scheme,"adi") == 0)
	{
		// add uri to tag_epc
		sprintf(tag_epc, "urn:epc:tag:%s-var:%d.%s",scheme,filter, p);
	}
	// usdod
	else if (strcmp(scheme,"usdod") == 0)
	{
		// add uri to tag_epc
		sprintf(tag_epc, "urn:epc:tag:%s-96:%d.%s",scheme,filter, p);
	}
	else
	{
		if (bits == 1)
			sprintf(scheme, "%s-var", scheme);
		else
			sprintf(scheme, "%s-%d", scheme, bits);
	
		// check for correct bits combination
		if (epc_scheme_lkup(scheme, 0xff) == 0xff)
		{
			p_printf(RED,"scheme %s : not found in partition table\n", scheme);
			return(EPC_ERROR);
		}
	
		// add uri to tag_epc
		sprintf(tag_epc, "urn:epc:tag:%s:%d.%s",scheme,filter, p);
	}	
	return(EPC_SUCCESS);
}
