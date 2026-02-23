/** Copyright (c) 2017 Paul van Haastrecht.
 *
 * Version 1.0.1 / februari 2026
 *   fixed compiler warnings/errors
 *
 *  version 1.0 / november 2017
 *    initial version
 *
 * EPC translation : This software will encode and decode between binary
 * EPC, Tag encoding, pure Idendity, barcode / GTIN as defined by the EPC
 * Tag Data Standard  version 1.9.
 *
 * The following EPC Tags have been tested with samples:
 *  sgcen-96, sgtin-96, sgtin-198, cpi-96,   cpi-var,  gid-96,  adi-var,
 *  gdti-96,  gdti-113, gdti-174,  gsrnp-96, gsrn-96,  sscc-96, sgln-96,
 *  sgln-195, grai-96,  grai-175,  giai-96,  giai-202, usdod-96,
 *
 * It can handle strings, integer, partinteger, partstrings, unpadded partition,
 * cage, numstrings, 6-bit strings, 6-bit partstrings. NOT all has been tested
 * yet however.
 *
 * adi-var, usdod-96 and gid-96 require special handling, which is implemented
 * hardcoded on many places, the others follow the partition table
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

#ifndef epctrans
#define epctrans

// return codes
#define EPC_SUCCESS   0
#define EPC_ERROR     1

#include <inttypes.h>
/*******************************************************************/
/**                   external functions                           */
/*******************************************************************/
/******************************************/
/** decode functions ( binary -> barcode)*/
/******************************************/

/* from binary to tag-encoding URI
 *
 * epc : input binary epc
 * buf : output tag-encoding uri
 * len : input length of binary epc in bytes
 */
int dec_binTOtag(uint8_t *epc, char *buf, int len);

/* from tag-encoding URI to pure-identity URI
 *
 * tag_EPC  : input tag EPC
 * pure_epc : output pure identity EPC
 */
int dec_tagTOpure(char *tag_epc, char *pure_epc);

/* from binary to pure-identity URI
 *
 * epc : input binary epc
 * buf : output Pure Idendity
 * len : length of binary epc
 */
int dec_binTOpure(uint8_t *epc,  char *tag_pure, int len);

/* from Tag-encoding URI or Pure-Identity URI to barcode (autodetect)
 *
 * epc     : input pure or tag URI
 * out     : output of the barcode
 * format  : 0 = barcode, 1 = GTIN (only for SGTIN)
 */
int dec_epcTObarc(char *epc, char *out, int format);

/* from binary to barcode
 *
 * epc     : input binary epc
 * barcode : output the barcode
 * len     : number of binary bytes in epc
 * format  : 0= barcode , 1 = GTIN (only for SGTIN)
 */
int dec_binTObarc(uint8_t *bin_epc, char *barcode, int len, int format);


/*
 *  only for SGTIN !
 *  from Tag-encoding URI or Pure-Identity URI to ONS (autodetect)
 *  epc : input tag or pure identity
 *  ons : ONS lookup format
 *
 *  check with dig  -t  NAPTR  075861.0434687.sgtin.id.onsepc.com
 */
int dec_epcTONS(char *epc, char *ons);

/********************************************/
/** encoding functions ( barcode -> binary) */
/********************************************/

/* from barcode to Pure-Identity
 *
 * barc    : input the barcode
 * epc     : output pure URI
 * GS1_len : length of company code
 * serial  : serial number to add (NULL if not needed)
 *
 * The serial number in input string will overrule any potential
 * provided serial number to the call.
 */
int enc_barcTOpure(char *barcode, char *pure_epc, int GS1_len, char * serial);

/* from pure-identity to tag-encoding URI
 * pure_epc : input pure EPC
 * tag_EPC  : output tag EPC
 * filter   : input filter value
 * bits     : input tagsize in bits
 */
int enc_pureTOtag(char *pure_epc, char *tag_epc, int filter, int bits);

/* from tag-encoding to binary
 * uri : input tag URI
 * epc : output binary epc to return
 * len : output number of bytes in binary EPC.
 */
int enc_tagTObin(char *uri, uint8_t *epc, int *len);

/* from pure-identity to binary
 * pure_epc: input pure EPC
 * bin_EPC : output bianry EPC
 * filter  : input filter value
 * bits    : input tagsize in bits
 * len     : output length of binary EPC
 */
int enc_pureTObin(char *pure_epc, uint8_t *bin_epc, int filter, int bits, int *len);

/* from Tag_encoding or Pure-Identity URI to binary (autodetect)
 * epc     : input EPC
 * bin_EPC : output binary EPC
 * filter  : input filter value (needed in case of pure-identity)
 * bits    : input tagsize in bits (needed in case of pure-identity)
 * len     : output length of binary EPC
 */
int enc_epcTObin(char *epc, uint8_t *bin_epc, int filter, int bits, int *len);


/*******************************************************************/
/**                   Internal functions                         */
/*******************************************************************/

// decoding supporting
int dec_binTOtag_adi(uint8_t *epc, char * buf, int len);
int dec_binTOtag_gid(uint8_t *epc, char *buf, int len);
int dec_binTOtag_usdod(uint8_t *epc, char * buf, int len);
int dec_comp_NumString_integer(uint8_t *epc, int bits, char *out);
int dec_comp_cage(uint8_t *epc, char *string);
int dec_comp_cage6bit_string(uint8_t *epc, int min_dig, int max_dig, char *string);
int dec_comp_integer(uint8_t *epc, int bits, char *out);
int dec_comp_part_integer(uint8_t *epc, int bits, int dig, char *out);
int dec_comp_part_unpadded(uint8_t *epc, int bits, int dig, char *out);
int dec_comp_string(uint8_t *epc, int bits, char *string);
int dec_epcTObarc_adi(char *p,char *out);
int dec_epcTObarc_cpi(char *p,char *out);
int dec_epcTObarc_gdti(char *p,char *out);
int dec_epcTObarc_giai(char *p,char *out);
int dec_epcTObarc_gid(char *p,char *out);
int dec_epcTObarc_grai(char *p,char *out);
int dec_epcTObarc_gsrn(char *p,char *out, char *scheme);
int dec_epcTObarc_sgcn(char *p,char *out);
int dec_epcTObarc_sscc(char *p,char *out);
int dec_epcTObarc_usdod(char *p,char *out);
void dec_fill_bit_buffer(uint8_t *epc);
int dec_get_integer(uint8_t *epc, int bits, uint64_t *val);
int dec_get_numstring(uint8_t *epc, int bits, char *string);

// encoding supporting
int enc_add_NumString_integer(uint8_t *epc, char *str, int bitc, int dig);
void enc_add_bit(int bit, uint8_t *epc);
void enc_add_int(uint8_t *epc, int val, int len);
int enc_add_reserved(uint8_t *epc, int len);
int enc_add_val_cage(uint8_t *epc, char *val);
int enc_add_val_cage6bit_string(uint8_t *epc, char *str, int min_dig, int max_dig);
int enc_add_val_integer(uint8_t *epc, char *val, int len);
int enc_add_val_part_integer(uint8_t *epc, char *val, int len, int dig);
int enc_add_val_part_unpad_integer(uint8_t *epc, char *val, int len, int dig);
int enc_add_val_string(uint8_t *epc, char *str, int bitc, int dig);
int enc_barcTOpure_cpi(char *barcode, char *pure_epc, int GS1_len);
int enc_barcTOpure_gcn(char *barcode, char *pure_epc, int GS1_len);
int enc_barcTOpure_gdti(char *barcode, char *pure_epc, int GS1_len);
int enc_barcTOpure_giai(char *barcode, char *pure_epc, int GS1_len);
int enc_barcTOpure_grai(char *barcode, char *pure_epc, int GS1_len);
int enc_barcTOpure_gsrn(char *barcode, char *pure_epc, int GS1_len, char *ai);
int enc_barcTOpure_sgln(char *barcode, char *pure_epc, int GS1_len);
int enc_barcTOpure_sscc(char *barcode, char *pure_epc, int GS1_len);
int enc_tagTObin_adi(char *p, uint8_t *epc, int *len);
int enc_tagTObin_gid(char *p, uint8_t *epc, int *len);
int enc_tagTObin_usdod(char *p, uint8_t *epc, int *len);

// general usage
uint16_t epc_hdr_lkup(uint64_t hdr, int part);
uint16_t epc_scheme_lkup(char *scheme, int L);
char *pars_uri_entry(char *p, char del, char *b);
char* stristr( const char* haystack, const char* needle );
void p_printf(int level, char *format, ...);

/*******************************************************************/
/**                   Internal definitions                         */
/*******************************************************************/

// types as used in comp2_type or comp3_type
#define Integer   'i'
#define String    's'
#define NumString   'n'
#define PartUnPadInteger 'U'
#define PartInteger 'I'
#define PartString  'S'
#define Reserved  'R'   // all null

#define Cage    'c'
#define String6bit  'd'
#define PartString6bit  'D'
#define NotAvailable  '0'


typedef struct epcpart
{
  /** header value */
  uint8_t header;
  /** EPC scheme name code */
  char  scheme[50];
  /** number of bits */
  int bits;
  /** filter length (bits) */
  int filterlen;
  /** partition row */
  int partrow;
  /** comp1_ length (bits) */
  int comp1_len;
  /** comp1_ entry digits */
  int comp1_dig;
  /** comp1_ entry name */
  char comp1_name[50];

   /** second entry length (bits) */
  int comp2_len;
  /** second entry digits */
  int comp2_dig;
  /** second entry name */
  char comp2_name[50];
  /** type of translation  (see types) */
  char comp2_type;

   /** third entry length (bits) */
  int comp3_len;
  /** third entry digits. if zero : neglect entry*/
  int comp3_dig;
  /** third entry name */
  char comp3_name[50];
  /** type of translation  (see types) */
  char comp3_type;

} epcpart;

// color display enable
#define RED   1
#define GREEN   2
#define YELLOW  3
#define BLUE  4
#define WHITE 5

#define REDSTR "\e[1;31m%s\e[00m"
#define GRNSTR "\e[1;92m%s\e[00m"
#define YLWSTR "\e[1;93m%s\e[00m"
#define BLUSTR "\e[1;34m%s\e[00m"

#endif
