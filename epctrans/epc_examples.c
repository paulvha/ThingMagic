/**
 *  Copyright (c) 2017 Paul van Haastrecht.
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
 *
 * Examples have been taken (and some corrected) from GS1 : EPC tag standard 1.9 ,
 * Ractified november 2014.
 *
 *
 * Version 1.0.1 / februari 2026
 *   fixed compiler warnings/errors
 *
 * version 1.0 / november 2017
 *    initial version
 *
 * To compile : cc -o epcx epc_examples.c epctrans.c -lm
 *
 * To run : ./epcx
 *
 */

#define version "1.0 / november 2017"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#include "epctrans.h"

typedef struct sel {
  int index;
  char tag[10];
} sel;


struct sel selection[22] ={
  { 1, "sgcen-96"},
  { 2, "sgtin-96"},
  { 3, "sgtin-198"},
  { 4, "cpi-96"},
  { 5, "cpi-var"},
  { 6, "gid-96"},
  { 7, "adi-var"},
  { 8, "gdti-96"},
  { 9, "gdti-113"},
  { 10, "gdti-174"},
  { 11, "gsrnp-96"},
  { 12, "gsrn-96"},
  { 13, "sscc-96"},
  { 14, "sgln-96"},
  { 15, "sgln-195"},
  { 16, "gsrnp-96"},
  { 17, "grai-96"},
  { 18, "grai-170"},
  { 19, "giai-96"},
  { 20, "giai-202"},
  { 21, "usdod-96"},
  { 00, "end"}
};


extern int NoColor;
void usage(char *n)
{
  int i=0;

  p_printf(YELLOW,"%s [-t EPC-number] [-h] [-n ]\n"
  "\t-h\thelp\n"
  "\t-n\tno color in output\n"
  "\t-t #\tprovide EPC number from commandline (instead of interactive)\n"
  "\tEPC-number encoding/decoding examples: \n",n);

  while (selection[i].index != 0)
  {
    printf("\t%d\t   %s\n", selection[i].index, selection[i].tag);
    i++;
  }

  p_printf(YELLOW,"\n(c) copyright Paul van Haastrecht. version: %s \n\n", version);
  exit(0);
}

int main(int argc, char *argv[])
{
  char  *epc, c;
  char  tag_epc[100], pure_epc[100], tmp_epc[100];
  char  barcode[50], bin_epc[100], ons[30], select[10];
  uint8_t tmp_bin_epc[40];
  uint8_t bin_epc_in[40];

  int   bin_epclen,i,j, tmp, len, opt;
  int   GS1_len, filter, bits;

  i=0;
  j=0;

  while ((opt = getopt(argc, argv, "t:hn")) != -1)
  {
    switch (opt)
        {
        case 't':
          j = (int) strtod(optarg,NULL);
          if (j > 21 || j < 0)
          {
            p_printf(RED,"Selection incorrect %d\n", j);
            usage(argv[0]);
          }
          break;

        case 'n':
          NoColor=1;
          break;

        case 'h':
        default:
          usage(argv[0]);
          break;
    }
  }

  if (j == 0)
  {
    // get input
    p_printf(YELLOW,
    "This program is demonstrating examples for EPC encoding / decoding\n"
    "copyright (c) Paul van Haastrecht. version %s\n\n",version);

    p_printf(GREEN,"Make your selection from the examples: \n ");
    while (selection[i].index != 0)
    {
      printf("\t%d\t %s\n", selection[i].index, selection[i].tag);
      i++;
    }

    while (1)
    {
      printf("Enter a number between 1 and %d ", i);
      scanf("%d", &j);
      if (j <= i) break;
    }
  }

  strcpy(select, selection[j-1].tag);

  /*************** examples available *******************************
   * sgcen-96, sgtin-96, sgtin-198, cpi-96,  cpi-var, gid-96,  adi-var
   * gdti-96,  gdti-174, gsrnp-96,  gsrn-96, sscc-96, sgln-96, sgln-195
   * grai-96,  grai-170, giai-96,   giai-202,usdod-96
   ******************************************************************/

  if (strcmp(select,"sgcen-96") == 0)
  {
    GS1_len = 7;    // company GS1 length

    filter  = 3;    // filter value to add from pure to tag
    bits  = 96;   // tagsize to add from pure to tag

    strcpy(barcode, "(255) 401234567890104711");
    strcpy(pure_epc,"urn:epc:id:sgcn:4012345.67890.04711");
    strcpy(tag_epc, "urn:epc:tag:sgcn-96:3.4012345.67890.04711");
    strcpy(bin_epc, "3F74F4E4E612640000019907");
  }

  else if (strcmp(select,"giai-96") == 0)
  {
    GS1_len = 7;    // company GS1 length

    filter  = 3;    // filter value to add from pure to tag
    bits  = 96;   // tagsize to add from pure to tag

    strcpy(barcode, "(8004) 06141415678");
    strcpy(pure_epc,"urn:epc:id:giai:0614141.5678");
    strcpy(tag_epc, "urn:epc:tag:giai-96:3.0614141.5678");
    strcpy(bin_epc, "3474257BF40000000000162E");
  }

  else if (strcmp(select,"giai-202") == 0)
  {
    GS1_len = 7;    // company GS1 length

    filter  = 3;    // filter value to add from pure to tag
    bits  = 202;    // tagsize to add from pure to tag

    strcpy(barcode, "(8004) 061414132a/b");
    strcpy(pure_epc,"urn:epc:id:giai:0614141.32a%2Fb");
    strcpy(tag_epc, "urn:epc:tag:giai-202:3.0614141.32a%2Fb");
    strcpy(bin_epc, "3874257BF59B2C2BF1000000000000000000000000000000000");
  }

  else if (strcmp(select,"grai-96") == 0)
  {
    GS1_len = 7;    // company GS1 length

    filter  = 3;    // filter value to add from pure to tag
    bits  = 96;   // tagsize to add from pure to tag

    strcpy(barcode, "(8003) 006141411234525678");
    strcpy(pure_epc,"urn:epc:id:grai:0614141.12345.5678");
    strcpy(tag_epc, "urn:epc:tag:grai-96:3.0614141.12345.5678");
    strcpy(bin_epc, "3374257BF40C0E400000162E");
  }

  else if (strcmp(select,"grai-170") == 0)
  {
    GS1_len = 7;    // company GS1 length

    filter  = 3;    // filter value to add from pure to tag
    bits  = 170;    // tagsize to add from pure to tag

    strcpy(barcode, "(8003) 0061414112345232a/b");
    strcpy(pure_epc,"urn:epc:id:grai:0614141.12345.32a%2Fb");
    strcpy(tag_epc, "urn:epc:tag:grai-170:3.0614141.12345.32a%2Fb");
    strcpy(bin_epc, "3774257BF40C0E59B2C2BF100000000000000000000");
  }
  else if (strcmp(select,"sgtin-96") == 0)
  {
    GS1_len = 7;    // company GS1 length

    filter  = 3;    // filter value to add from pure to tag
    bits  = 96;   // tagsize to add from pure to tag

    strcpy(barcode,  "(01) 80614141123458 (21) 6789");
    strcpy(pure_epc, "urn:epc:id:sgtin:0614141.812345.6789");
    strcpy(tag_epc,  "urn:epc:tag:sgtin-96:3.0614141.812345.6789");
    strcpy(bin_epc,  "3074257BF7194E4000001A85");

  }

  else if (strcmp(select,"sgtin-198") == 0)
  {
    GS1_len = 7;    // company GS1 length

    filter  = 3;    // filter value to add from pure to tag
    bits  = 198;    // tagsize to add from pure to tag

    strcpy(barcode,  "(01) 70614141123451 (21) 32a/b");
    strcpy(pure_epc, "urn:epc:id:sgtin:0614141.712345.32a%2Fb");
    strcpy(tag_epc,  "urn:epc:tag:sgtin-198:3.0614141.712345.32a%2Fb");
    strcpy(bin_epc,  "3674257BF6B7A659B2C2BF1000000000000000000000000000");

  }

  else if (strcmp(select,"cpi-96") == 0)
  {
    GS1_len = 7;    // company GS1 length

    filter  = 3;    // filter value to add from pure to tag
    bits  = 96;   // tagsize to add from pure to tag

    strcpy(barcode,  "(8010) 061414198765 (8011) 12345");
    strcpy(pure_epc, "urn:epc:id:cpi:0614141.98765.12345");
    strcpy(tag_epc,  "urn:epc:tag:cpi-96:3.0614141.98765.12345");
    strcpy(bin_epc,  "3C74257BF400C0E680003039");
  }

  else if (strcmp(select,"cpi-var") == 0)
  {
    GS1_len = 7;    // company GS1 length

    filter  = 3;    // filter value to add from pure to tag
    bits  = 1;    // 1 = var

    strcpy(barcode,  "(8010) 06141415PQ7/Z43 (8011) 12345");
    strcpy(pure_epc, "urn:epc:id:cpi:0614141.5PQ7%2FZ43.12345");
    strcpy(tag_epc,  "urn:epc:tag:cpi-var:3.0614141.5PQ7%2FZ43.12345");
    strcpy(bin_epc,  "3D74257BF75411DEF6B4CC00000003039");
  }

  else if (strcmp(select,"adi-var") == 0)
  {
    GS1_len = 7;    // company GS1 length

    filter  = 3;    // filter value to add from pure to tag
    bits  = 96;   // tagsize to add from pure to tag

    strcpy(barcode,  "");
    strcpy(pure_epc, "urn:epc:id:adi:35962.PQ7VZ4.M37GXB92");
    strcpy(tag_epc,  "urn:epc:tag:adi-var:0.35962.PQ7VZ4.M37GXB92");
    strcpy(bin_epc,  "3B0E0CF5E76C9047759AD00373DC7602E7200");
  }

  else if (strcmp(select,"usdod-96") == 0)
  {
    GS1_len = 7;    // company GS1 length

    filter  = 3;    // filter value to add from pure to tag
    bits  = 96;   // tagsize to add from pure to tag

    strcpy(barcode,  "");
    strcpy(pure_epc, "urn:epc:id:usdod:CAGEY.5678");
    strcpy(tag_epc,  "urn:epc:tag:usdod-96:3.CAGEY.5678");
    strcpy(bin_epc,  "2F320434147455900000162E");
  }

  else if (strcmp(select,"gid-96") == 0)
  {
    GS1_len = 7;    // company GS1 length

    filter  = 3;    // filter value to add from pure to tag
    bits  = 96;   // tagsize to add from pure to tag

    strcpy(barcode,  "");
    strcpy(pure_epc, "urn:epc:id:gid:31415.271828.1414");
    strcpy(tag_epc,  "urn:epc:tag:gid-96:31415.271828.1414");
    strcpy(bin_epc,  "350007AB70425D4000000586");
  }

  else if (strcmp(select,"gdti-174") == 0)
  {
    GS1_len = 7;    // company GS1 length

    filter  = 3;    // filter value to add from pure to tag
    bits  = 174;    // tagsize to add from pure to tag

    strcpy(barcode,  "(253) 4012345987652ABCDefgh012345678");
    strcpy(pure_epc, "urn:epc:id:gdti:4012345.98765.ABCDefgh012345678");
    strcpy(tag_epc,  "urn:epc:tag:gdti-174:3.4012345.98765.ABCDefgh012345678");
    strcpy(bin_epc,  "3E74F4E4E7039B061438997367D0C18B266D1AB66EE0");
  }

  else if (strcmp(select,"gdti-113") == 0)
  {
    GS1_len = 7;    // company GS1 length

    filter  = 3;    // filter value to add from pure to tag
    bits  = 174;    // tagsize to add from pure to tag

    strcpy(barcode,  "(253) 401234598765212345678901234567");
    strcpy(pure_epc, "urn:epc:id:gdti:4012345.98765.12345678901234567");
    strcpy(tag_epc,  "urn:epc:tag:gdti-113:3.4012345.98765.12345678901234567");
    strcpy(bin_epc,  "3A74F4E4E7039AC790E65D7AA5C38000");
  }

  else if (strcmp(select,"gdti-96") == 0)
  {
    GS1_len = 7;    // company GS1 length

    filter  = 3;    // filter value to add from pure to tag
    bits  = 96;   // tagsize to add from pure to tag

    strcpy(barcode,  "(253) 06141411234525678");
    strcpy(pure_epc, "urn:epc:id:gdti:0614141.12345.5678");
    strcpy(tag_epc,  "urn:epc:tag:gdti-96:3.0614141.12345.5678");
    strcpy(bin_epc,  "2C74257BF46072000000162E");
  }
  else if (strcmp(select,"gsrnp-96") == 0)
  {
    GS1_len = 7;    // company GS1 length

    filter  = 3;    // filter value to add from pure to tag
    bits  = 96;   // tagsize to add from pure to tag

    strcpy(barcode,  "(8017) 061414112345678902");
    strcpy(pure_epc, "urn:epc:id:gsrnp:0614141.1234567890");
    strcpy(tag_epc,  "urn:epc:tag:gsrnp-96:3.0614141.1234567890");
    strcpy(bin_epc,  "2E74257BF4499602D2000000");
  }

  else if (strcmp(select,"gsrn-96") == 0)
  {
    GS1_len = 7;    // company GS1 length

    filter  = 3;    // filter value to add from pure to tag
    bits  = 96;   // tagsize to add from pure to tag

    strcpy(barcode,  "(8018) 061414112345678902");
    strcpy(pure_epc, "urn:epc:id:gsrn:0614141.1234567890");
    strcpy(tag_epc,  "urn:epc:tag:gsrn-96:3.0614141.1234567890");
    strcpy(bin_epc,  "2D14257BF4499602D2000000");
  }

  else if (strcmp(select,"sscc-96") == 0)
  {
    GS1_len = 7;    // company GS1 length

    filter  = 3;    // filter value to add from pure to tag
    bits  = 96;   // tagsize to add from pure to tag

    strcpy(barcode,  "(00) 106141412345678908");
    strcpy(pure_epc, "urn:epc:id:sscc:0614141.1234567890");
    strcpy(tag_epc,  "urn:epc:tag:sscc-96:3.0614141.1234567890");
    strcpy(bin_epc,  "3174257BF4499602D2000000");
  }
  else if (strcmp(select,"sgln-96") == 0)
  {
    GS1_len = 7;    // company GS1 length

    filter  = 3;    // filter value to add from pure to tag
    bits  = 96;   // tagsize to add from pure to tag

    strcpy(barcode,  "(414) 0614141123452 (254) 5678");
    strcpy(pure_epc, "urn:epc:id:sgln:0614141.12345.5678");
    strcpy(tag_epc,  "urn:epc:tag:sgln-96:3.0614141.12345.5678");
    strcpy(bin_epc,  "3274257BF46072000000162E");
  }

  else if (strcmp(select,"sgln-195") == 0)
  {
    GS1_len = 7;    // company GS1 length

    filter  = 3;    // filter value to add from pure to tag
    bits  = 195;    // tagsize to add from pure to tag

    strcpy(barcode,  "(414) 0614141123452 (254) 32a/b");
    strcpy(pure_epc, "urn:epc:id:sgln:0614141.12345.32a%2Fb");
    strcpy(tag_epc,  "urn:epc:tag:sgln-195:3.0614141.12345.32a%2Fb");
    strcpy(bin_epc,  "3974257BF46072CD9615F8800000000000000000000000000");
  }
  else
  {
    printf("*********************************************\n");
    printf("selection : %s. not known\n", select);
    printf("*********************************************\n");
    exit(0);
  }

  // initialize binary EPC and length
  len = strlen(bin_epc);

  for (i = 0, j = 0; i < len; i++)
  {
    c = bin_epc[i];

    if (c > '9') c += '9';

    if (i%2)
    {
      bin_epc_in[j++] |= (c & 0xf);
      tmp = 0;
    }
    else
    {
      bin_epc_in[j] = (c & 0xf) << 4;;
      tmp = 1;
    }

  }

  // set correct length
  bin_epclen = j + tmp;


p_printf(YELLOW,
"****************************************************************\n"
"***  %s         ENCODING ROUTINES \n"
"****************************************************************\n\n", select);

  // create a pure-identity EPC from barcode
  printf("1. from barcode to Pure-identity URI. Mandatory input Companylength %d and optional serial nummber 11111\n", GS1_len);

  if (enc_barcTOpure(barcode, tmp_epc, GS1_len, "11111") == EPC_SUCCESS)
  {
    printf("\tBarcode: \t\t%s resulted in \n\tpure-identity URI:\t%s\n\n", barcode, tmp_epc);
  }
  else
    printf("ERROR \n\n");

  // create a tag-encoding EPC from pure-identity EPC
  printf("2. from pure-identity URI to tag-encoding URI. Mandatory:  filter %d, tagsize %d", filter, bits);

  if (bits == 1) printf(" : -var\n");
  else printf("\n");

  if (enc_pureTOtag(pure_epc, tmp_epc, filter, bits) == EPC_SUCCESS)
  {
    printf("\tpure-identity URI:\t%s resulted in \n\ttag-encoding URI: \t%s\n\n", pure_epc, tmp_epc);
  }
  else
    printf("ERROR \n\n");

  // create a binary EPC from tag EPC
  printf("3. from tag-encoding URI to Binary EPC\n");

  if (enc_tagTObin(tag_epc, tmp_bin_epc, &len) == EPC_SUCCESS)
  {
    printf("\ttag-encoding URI:  \t%s resulted in\n\tbinary EPC: \t\t", tag_epc);
    for (i = 0; i < len; i ++) printf("%02X ", tmp_bin_epc[i]);
    printf("\n\n");
  }
  else
    printf("ERROR \n\n");

  // create a binary EPC from pure-identity EPC
  printf("4. from pure-identity to Binary EPC. Mandatory:  filter %d, tagsize %d", filter, bits);

  if (bits == 1) printf(" : -var\n");
  else printf("\n");

  if (enc_pureTObin(pure_epc, tmp_bin_epc, filter, bits, &len) == EPC_SUCCESS)
  {
    printf("\tpure-identity URI: \t%s resulted in\n\tbinary EPC:\t\t", pure_epc);
    for (i = 0; i < len; i ++) printf("%02X ", tmp_bin_epc[i]);
    printf("\n\n");
  }
  else
    printf("ERROR \n\n");

  // create a binary from an URI (autodetect)
  printf("5. autodetect URI and create binary. filter %d, tagsize %d", filter, bits);

  if (bits == 1) printf(" : -var\n");
  else printf("\n");
  // set EPC
  epc=pure_epc;
  if (enc_epcTObin(epc, tmp_bin_epc, filter, bits, &len) == EPC_SUCCESS)
  {
    printf("\tURI: \t\t\t%s resulted in\n\tbinary EPC: \t\t", epc);
    for (i = 0; i < len; i ++) printf("%02X ", tmp_bin_epc[i]);
    printf("\n\n");
  }
  else
    printf("ERROR \n\n");

  epc=tag_epc;
  if (enc_epcTObin(epc, tmp_bin_epc, filter, bits, &len) == EPC_SUCCESS)
  {
    printf("\tURI: \t\t\t%s resulted in\n\tbinary EPC: \t\t", epc);
    for (i = 0; i < len; i ++) printf("%02X ", tmp_bin_epc[i]);
    printf("\n\n");
  }
  else
    printf("ERROR \n\n");

p_printf(YELLOW,
"****************************************************************\n"
"***  %s         DECODING ROUTINES \n"
"****************************************************************\n\n", select);

  // create tag-encoding URI from binary
  printf("1. from Binary to tag-encoding URI\n");

  if (dec_binTOtag(bin_epc_in, tmp_epc, bin_epclen) == EPC_SUCCESS)
  {
    printf("\tbinary EPC: \t\t");
    for (i = 0; i < bin_epclen; i ++) printf("%02X ", bin_epc_in[i]);
    printf("\n\ttag-encoding URI: \t%s\n\n", tmp_epc);
  }
  else
    printf("\t ERROR \n\n");

  // create a pure-identity URI from tag-encoding URI
  printf("2. from tag-encoding URI to pure-identity EPC\n");

  if (dec_tagTOpure(tag_epc, tmp_epc) == EPC_SUCCESS)
  {
    printf("\ttag-encoding URI: \t%s resulted in \n\tpure-identity URI:\t%s\n\n",tag_epc, tmp_epc);
  }
  else
    printf("\t ERROR \n\n");

  // create pure-identity URI (as used in EPCIS) from binary
  printf("3. from Binary to pure-identity URI\n");

  if (dec_binTOpure(bin_epc_in, tmp_epc, bin_epclen) == EPC_SUCCESS)
  {
    printf("\tbinary EPC: \t\t");
    for (i = 0; i < bin_epclen; i ++) printf("%02X ", bin_epc_in[i]);
    printf("\n\tpure-identity URI: \t%s\n\n", tmp_epc);
  }
  else
    printf("\t ERROR \n\n");

  // auto detect EPC to GTIN
  printf("4. Autodetect from EPC URI to GTIN\n");
  if (dec_epcTObarc(tag_epc, barcode, 1) == EPC_SUCCESS)
  {
    printf("\ttag URI:\t\t%s\n", tag_epc);
    printf("\tBarcode:\t\t%s\n\n", barcode);
  }
  else
    printf("\t ERROR \n\n");

  // binary EPC to GTIN
  printf("5. from Binary to GTIN\n");

  if (dec_binTObarc(bin_epc_in, barcode, bin_epclen, 1 ) == EPC_SUCCESS)
  {
    printf("\tbinary EPC: \t\t");
    for (i = 0; i < bin_epclen; i ++) printf("%02X ", bin_epc_in[i]);
    printf("\n\tbarcode: \t\t%s\n\n", barcode);
  }
  else
    printf("\t ERROR \n\n");

  /* SPECIAL FOR SGTIN ONLY */
  if (stristr(select, "sgtin") != NULL)
  {
    p_printf(YELLOW, "\t********************\n"
    "\t** only for sgtin **\n"
    "\t********************\n\n");
    // create barcode from pure-identity URI
    printf("6. Autodetect from EPC URI to barcode\n");

    if (dec_epcTObarc(pure_epc, barcode, 0) == EPC_SUCCESS)
    {
      printf("\tpure-identity URI: \t%s\n", pure_epc);
      printf("\tBarcode:\t\t%s\n\n", barcode);
    }
    else
      printf("\t ERROR \n\n");

    printf("7. from Binary to barcode\n");

    if (dec_binTObarc(bin_epc_in, barcode, bin_epclen, 0 ) == EPC_SUCCESS)
    {
      printf("\tbinary EPC: \t\t");
      for (i = 0; i < bin_epclen; i ++) printf("%02X ", bin_epc_in[i]);
      printf("\n\tbarcode: \t\t%s\n\n", barcode);
    }
    else
      printf("\t ERROR \n\n");

    printf("8. autodetect EPC (tag or pure) to ONS\n");

    if (dec_epcTONS(pure_epc, ons ) == EPC_SUCCESS)
    {
      printf("\tEPC:\t\t\t%s\n\tONS:\t\t\t%s\n\n",pure_epc, ons);
    }
    else
      printf("\t ERROR \n\n");

    if (dec_epcTONS(tag_epc, ons ) == EPC_SUCCESS)
    {
      printf("\tEPC:\t\t\t%s\n\tONS:\t\t\t%s\n\n",tag_epc, ons);
    }
    else
      printf("\t ERROR \n\n");
  }

  exit(0);
}
