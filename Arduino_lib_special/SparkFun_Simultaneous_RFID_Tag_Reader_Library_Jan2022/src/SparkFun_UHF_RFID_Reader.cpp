/*
  Library for controlling the Nano M6E from ThingMagic
  This is a stripped down implementation of the Mercury API from ThingMagic

  By: Nathan Seidle @ SparkFun Electronics
  Date: October 3rd, 2016
  https://github.com/sparkfun/Simultaneous_RFID_Tag_Reader

  License: Open Source MIT License
  If you use this code please consider buying an awesome board from SparkFun. It's a ton of
  work (and a ton of fun!) to put these libraries together and we want to keep making neat stuff!
  https://opensource.org/licenses/MIT

  The above copyright notice and this permission notice shall be included in all copies or
  substantial portions of the Software.

  To learn more about how ThingMagic controls the module please look at the following SDK files:
    serial_reader_l3.c - Contains the bulk of the low-level routines
    serial_reader_imp.h - Contains the OpCodes
    tmr__status_8h.html - Contains the Status Word error codes

  Available Functions:
    setBaudRate
    setRegion
    setReadPower
    startReading (continuous read)
    stopReading
    readTagEPC
    writeTagEPC
    readTagData
    writeTagData
    killTag
    (nyw) lockTag
*/

#if (ARDUINO >= 100)
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#include "SparkFun_UHF_RFID_Reader.h"


//***************************************************************
#define DEBUG       //If you comment this line the DPRINT & DPRINTLN
//lines are defined as blank.
//examples:
//DPRINTLN("Testing123");
//DPRINTLN(0xC0FFEEul,DEC);
//DPRINTLN(12648430ul,HEX);

#ifdef DEBUG
#define DPRINT(...)    _debugSerial->print(__VA_ARGS__)
//OR, #define DPRINT(args...)    Serial.print(args)
#define DPRINTLN(...)  _debugSerial->println(__VA_ARGS__)
#define DRINTF(...)    _debugSerial->print(F(__VA_ARGS__))
#define DPRINTLNF(...) _debugSerial->println(F(__VA_ARGS__))

#else
#define DPRINT(...)     //blank line
#define DPRINTLN(...)   //blank line
#define DPRINTF(...)    //blank line
#define DPRINTLNF(...)  //blank line

#endif
//***************************************************************

RFID::RFID(void)
{
  // Constructor
  _continuousModeTemp = false;    // indicate measuring temperature in continuous mode
  _contTemp = 0;                  // store Temperature in continuous mode
}

//Initialize the Serial port
void RFID::begin(Stream &serialPort)
{
  _nanoSerial = &serialPort; //Grab which port the user wants us to use

  //_nanoSerial->begin(); //Stream has no .begin() so the user has to do a whateverSerial.begin(xxxx); from setup()
}

//Enable or disable the printing of sent/response HEX values.
//Use this in conjunction with 'Transport Logging' from the Universal Reader Assistant to see what they're doing that we're not
void RFID::enableDebugging(Stream &debugPort)
{
  _debugSerial = &debugPort; //Grab which port the user wants us to use for debugging

  _printDebug = true; //Should we print the commands we send? Good for debugging
}

void RFID::disableDebugging(void)
{
  _printDebug = false; //Turn off extra print statements
}

//Set baud rate
//Takes in a baud rate
//Returns response in the msg array
void RFID::setBaud(long baudRate)
{
  //Copy this setting into a temp data array
  uint8_t size = sizeof(baudRate);
  uint8_t data[size];
  for (uint8_t x = 0 ; x < size ; x++)
    data[x] = (uint8_t)(baudRate >> (8 * (size - 1 - x)));

  sendMessage(TMR_SR_OPCODE_SET_BAUD_RATE, data, size, COMMAND_TIME_OUT, true); // changed from false (no wait for response)
}

/* September 2020 special / paulvha
 *
 * read a one the memory banks on a tag while in continuous mode
 *
 * @param bank :
 *  TMR_GEN2_BANK_RESERVED
 *  TMR_GEN2_BANK_EPC
 *  TMR_GEN2_BANK_TID
 *  TMR_GEN2_BANK_USER
 *
 * @param address :
 *  Word address to start reading (normally 0)
 *
 * @param length
 *  number words to read from the bank (maximum 32)
 */
void RFID::startReadingBank(uint8_t bank, uint32_t address, uint8_t length)
{

/* Create special blob
 * ff           header (added later on)
 * 1f           message length (added later on)
 * 2f           embedded read (added later on)
 *
 * 00 00        no timeouts
 * 01           embedded command
 * 22           opcode
 * 00 00        search flag
 * 05           GEN2
 * 15           length request
 * 22           Multiple read opcode
 * 10           option byte (read continuous
 * 01 1f        search flag
 * 00 fa        timeout  250ms
 * 01 ff        metadata
 * 01 00        add temperature statics
 * 01           1 embedded command
 * 09           length embedded command
 * 28           CMD code : read tag
 * 07 d0        timeout (2000Ms)
 * 00 03        bank(s)
 * 00 00 00 00  start address
 * 00           number of words expected (zero == all)
 *
 * 84 35        CRC (added later on)
 */

   uint8_t configBlob[30] = {0x00, 0x00, 0x01, 0x22, 0x00, 0x00, 0x05, 0x15, 0x22, 0x10, 0x01, 0x1f, 0x00, 0xfa, 0x01, 0xff, 0x01,                            0x00, 0x01, 0x09, 0x28, 0x07, 0xd0, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00};

   // overwrite defaults in configBlob
   configBlob[24] = bank;

   // set start address to read
   for (uint8_t x = 0 ; x < sizeof(address) ; x++)
    configBlob[25 + x] = address >> (8 * (3 - x)) & 0xFF;

   /*
    * 32 words is the maximum length with embedded read (datasheet page 58)
    * If more than 32 words an incorrect message is returned. (tried that and failed!)
    *
    * The length of TID, RESERVE or EPC memory will fit. However if
    * you request 32 words and the bank is not able to deliver,
    * zero are returned. Hence on these memory locations: keep the
    * length to zero. (default set in SparkFun_UHF_RFID_Reader.h)
    *
    * BUT most of the time USER memory is larger, hence is kept to max 32.
    *
    * However if your memory bank has less than 32 words to read, due to
    * offset, and you request 32 words you will NOT receive any words !!
    * You have to request less words.
    */

   if (bank == TMR_GEN2_BANK_USER) {
        if (length == 0 || length > 32) length = 32;
   }
   else {
        if (length > 32) length = 32;
   }

   configBlob[29] = length;

   // Sending:  ff 03 9a 01 08 00 a7 5d
   // This allows tags with the same EPC ID but different values in the specified memory
   // bank to be treated as unique tags during inventories, else the readcount is just increased

   uint8_t c2[] ={ 0x01, 0x08, 0x00};
   sendMessage(TMR_SR_OPCODE_SET_READER_OPTIONAL_PARAMS, c2, sizeof(c2));

   //ff 03 9a 01 0c 00 a3 5d disable TMR_SR_CONFIGURATION_ENABLE_READ_FILTER
   uint8_t c1[] ={ 0x01, 0x0c, 0x00};
   sendMessage(TMR_SR_OPCODE_SET_READER_OPTIONAL_PARAMS, c1, sizeof(c1));

  _continuousModeTemp = true;
  _contTemp = 0;         // reset temperature

   sendMessage(TMR_SR_OPCODE_MULTI_PROTOCOL_TAG_OP, configBlob, sizeof(configBlob));
}

//Begin scanning for tags
//There are many many options and features to the nano, this sets options
//for continuous read of GEN2 type tags
void RFID::startReading()
{
  disableReadFilter(); //Don't filter for a specific tag, read all tags

  //This blob was found by using the 'Transport Logs' option from the Universal Reader Assistant
  //And connecting the Nano eval kit from Thing Magic to the URA
  //A lot of it has been deciphered but it's easier and faster just to pass a blob than to
  //assemble every option and sub-opcode.

  // added for streaming to provide temperature statistics.  May 2020 paulvh
  //                                                               length           add stats                            add stats_temp
  uint8_t configBlob[] = {0x00, 0x00, 0x01, 0x22, 0x00, 0x00, 0x05, 0x09, 0x22, 0x10, 0x01, 0x1B, 0x03, 0xE8, 0x01, 0xFF, 0x01, 0x00};
  _continuousModeTemp = true;
  _contTemp = 0;         // reset temperature

  // original without temperature statistics
  //uint8_t configBlob[] = {0x00, 0x00, 0x01, 0x22, 0x00, 0x00, 0x05, 0x07, 0x22, 0x10, 0x00, 0x1B, 0x03, 0xE8, 0x01, 0xFF};

  /*
    //Timeout should be zero for true continuous reading
    SETU16(newMsg, i, 0);
    SETU8(newMsg, i, (uint8_t)0x1); // TM Option 1, for continuous reading
    SETU8(newMsg, i, (uint8_t)TMR_SR_OPCODE_READ_TAG_ID_MULTIPLE); // sub command opcode
    SETU16(newMsg, i, (uint16_t)0x0000); // search flags, only 0x0001 is supported
    SETU8(newMsg, i, (uint8_t)TMR_TAG_PROTOCOL_GEN2); // protocol ID
  */

  sendMessage(TMR_SR_OPCODE_MULTI_PROTOCOL_TAG_OP, configBlob, sizeof(configBlob));
}

//Stop a continuous read
void RFID::stopReading()
{
  //00 00 = Timeout, currently ignored
  //02 = Option - stop continuous reading
  uint8_t configBlob[] = {0x00, 0x00, 0x02};

  sendMessage(TMR_SR_OPCODE_MULTI_PROTOCOL_TAG_OP, configBlob, sizeof(configBlob), false); //Do not wait for response

  _continuousModeTemp = false;
}

//Given a region, set the correct freq
//0x04 = IN
//0x05 = JP
//0x06 = PRC
//0x08 = EU3
//0x09 = KR2
//0x0B = AU
//0x0C = NZ
//0x0D = NAS2 (North America)
//0xFF = OPEN
void RFID::setRegion(uint8_t region)
{
  sendMessage(TMR_SR_OPCODE_SET_REGION, &region, sizeof(region));
}

/* SPECIAL ADD / February 2020
 *
 * Set powerMode :
 *
 * 0 = Full Mode
 * 1 = Minimal saving mode.
 * 2 = Medium saving mode.
 * 3 = Maximum saving mode.
 *
 * NOTE :
 * Maximum Saving Mode only supports communications at 9600 baud
 */
bool RFID::setPowerMode(uint8_t mode)
{
  if(mode > 3) return false;

  sendMessage(TMR_SR_OPCODE_SET_POWER_MODE, &mode, sizeof(mode));

  // Good response
  if (msg[0] == ALL_GOOD) return true;

  return false;
}

//Sets the TX and RX antenna ports to 01
//Because the Nano module has only one antenna port, it is not user configurable
void RFID::setAntennaPort(void)
{
  uint8_t configBlob[] = {0x01, 0x01}; //TX port = 1, RX port = 1
  sendMessage(TMR_SR_OPCODE_SET_ANTENNA_PORT, configBlob, sizeof(configBlob));
}

//This was found in the logs. It seems to be very close to setAntennaPort
//Search serial_reader_l3.c for cmdSetAntennaSearchList for more info
void RFID::setAntennaSearchList(void)
{
  uint8_t configBlob[] = {0x02, 0x01, 0x01}; //logical antenna list option, TX port = 1, RX port = 1
  sendMessage(TMR_SR_OPCODE_SET_ANTENNA_PORT, configBlob, sizeof(configBlob));
}

//Sets the protocol of the module
//Currently only GEN2 has been tested and supported but others are listed here for reference
//and possible future support
//TMR_TAG_PROTOCOL_NONE              = 0x00
//TMR_TAG_PROTOCOL_ISO180006B        = 0x03
//TMR_TAG_PROTOCOL_GEN2              = 0x05
//TMR_TAG_PROTOCOL_ISO180006B_UCODE  = 0x06
//TMR_TAG_PROTOCOL_IPX64             = 0x07
//TMR_TAG_PROTOCOL_IPX256            = 0x08
//TMR_TAG_PROTOCOL_ATA               = 0x1D
void RFID::setTagProtocol(uint8_t protocol)
{
  uint8_t data[2];
  data[0] = 0; //Opcode expects 16-bits
  data[1] = protocol;

  sendMessage(TMR_SR_OPCODE_SET_TAG_PROTOCOL, data, sizeof(data));
}

void RFID::enableReadFilter(void)
{
  setReaderConfiguration(0x0C, 0x01); //Enable read filter
}

//Disabling the read filter allows continuous reading of tags
void RFID::disableReadFilter(void)
{
  setReaderConfiguration(0x0C, 0x00); //Diable read filter
}

//Sends optional parameters to the module
//See TMR_SR_Configuration in serial_reader_imp.h for a breakdown of options
void RFID::setReaderConfiguration(uint8_t option1, uint8_t option2)
{
  uint8_t data[3];

  //These are parameters gleaned from inspecting the 'Transport Logs' of the Universal Reader Assistant
  //And from serial_reader_l3.c
  data[0] = 1; //Key value form of command
  data[1] = option1;
  data[2] = option2;

  sendMessage(TMR_SR_OPCODE_SET_READER_OPTIONAL_PARAMS, data, sizeof(data));
}

//Gets optional parameters from the module
//We know only the blob and are not able to yet identify what each parameter does
void RFID::getOptionalParameters(uint8_t option1, uint8_t option2)
{
  //These are parameters gleaned from inspecting the 'Transport Logs' of the Universal Reader Assistant
  //During setup the software pings different options
  uint8_t data[2];
  data[0] = option1;
  data[1] = option2;
  sendMessage(TMR_SR_OPCODE_GET_READER_OPTIONAL_PARAMS, data, sizeof(data));
}

//Get the version number from the module
void RFID::getVersion(void)
{
  sendMessage(TMR_SR_OPCODE_VERSION);
}

//Set the read TX power
//Maximum power is 2700 = 27.00 dBm
//1005 = 10.05dBm
void RFID::setReadPower(int16_t powerSetting)
{
  if (powerSetting > 2700) powerSetting = 2700; //Limit to 27dBm

  //Copy this setting into a temp data array
  uint8_t size = sizeof(powerSetting);
  uint8_t data[size];
  for (uint8_t x = 0 ; x < size ; x++)
    data[x] = (uint8_t)(powerSetting >> (8 * (size - 1 - x)));

  sendMessage(TMR_SR_OPCODE_SET_READ_TX_POWER, data, size);
}

//Get the read TX power
void RFID::getReadPower()
{
  uint8_t data[] = {0x00}; //Just return power
  //uint8_t data[] = {0x01}; //Return power with limits

  sendMessage(TMR_SR_OPCODE_GET_READ_TX_POWER, data, sizeof(data));
}

//Set the write power
//Maximum power is 2700 = 27.00 dBm
//1005 = 10.05dBm
void RFID::setWritePower(int16_t powerSetting)
{
  uint8_t size = sizeof(powerSetting);
  uint8_t data[size];
  for (uint8_t x = 0 ; x < size ; x++)
    data[x] = (uint8_t)(powerSetting >> (8 * (size - 1 - x)));

  sendMessage(TMR_SR_OPCODE_SET_WRITE_TX_POWER, data, size);
}

//Get the write TX power
void RFID::getWritePower()
{
  uint8_t data[] = {0x00}; //Just return power
  //uint8_t data[] = {0x01}; //Return power with limits

  sendMessage(TMR_SR_OPCODE_GET_WRITE_TX_POWER, data, sizeof(data));
}

// read Nano temperature  - special add May 2020 (update september)
int8_t RFID::getTemp()
{
  // the temperature is coming from streaming
  if (_continuousModeTemp ) {
      if (_contTemp > 0 ) return(_contTemp);
      return(-1);
  }

  sendMessage(TMR_SR_OPCODE_GET_TEMPERATURE);

  if (msg[0] == ALL_GOOD)  return(int8_t) msg[5];
  else return -1;
}

//Read a single PC + EPC word (february 2021)
//Caller must provide an array for PC + EPC to be stored in with length of 14 min
uint8_t RFID::readTagPCW(uint8_t *epc, uint8_t &epcLength, uint16_t timeOut)
{
  uint8_t bank = 0x01;    //EPC data bank
  uint8_t address = 0x01; //PC word starts at spot 1
  uint8_t ret = RESPONSE_FAIL;

  if (epcLength > 13)
    ret = readData(bank, address, epc, epcLength, timeOut);

  return (ret);
}

//This writes a new PC word + EPC to the first tag it detects (february 2021)
//Use with caution. This function doesn't control which tag hears the command.
uint8_t RFID::writeTagPCW(uint8_t *newID, uint8_t newIDLength, uint16_t timeOut)
{
  uint8_t bank = 0x01; //EPC memory
  uint8_t address = 0x01; //PC word starts at spot 1
  uint8_t ret = RESPONSE_FAIL;

  // length must be at least 2
  if (newIDLength > 1)
    ret = writeData(bank, address, newID, newIDLength, timeOut);

  return(ret);
}

//Read a single EPC
//Caller must provide an array for EPC to be stored in
uint8_t RFID::readTagEPC(uint8_t *epc, uint8_t &epcLength, uint16_t timeOut)
{
  uint8_t bank = 0x01; //User data bank
  uint8_t address = 0x02; //Starts at 2

  return (readData(bank, address, epc, epcLength, timeOut));
}

//This writes a new EPC to the first tag it detects
//Use with caution. This function doesn't control which tag hears the command.
uint8_t RFID::writeTagEPC(uint8_t *newID, uint8_t newIDLength, uint16_t timeOut)
{
  uint8_t bank = 0x01; //EPC memory
  uint8_t address = 0x02; //EPC starts at spot 4

  return (writeData(bank, address, newID, newIDLength, timeOut));
}

//This reads the user data area of the tag. 0 to 64 bytes are normally available.
//Use with caution. The module can't control which tag hears the command.
//TODO Add support for accessPassword
uint8_t RFID::readUserData(uint8_t *userData, uint8_t &userDataLength, uint16_t timeOut)
{
  uint8_t bank = 0x03; //User data bank
  uint8_t address = 0x00; //Starts at 0

  return (readData(bank, address, userData, userDataLength, timeOut));
}

// This reads the all banks available in a structure (SPECIAL August 2020)
uint8_t RFID::ReadingAllBanks(TMR_TagReadData *read)
{
  //Don't filter for a specific tag, read all tags
  disableReadFilter();

  // clear tag ID buffer
  sendMessage(TMR_SR_OPCODE_CLEAR_TAG_ID_BUFFER);

/* special blob
  0x88    TMR_SR_TAGOP_MULTI_SELECT
  0x10    option byte
  0x0017  search flag
          0001 0111
          0001 ---- large population
          ------1-- embedded command
          -------11 use configured antenna list (sent earlier)
  0x07d0  timeout (2000mS)
  0x0fff  metadata
  0x01,   embedded command count (only 1 supported)
  0x09,   length of embedded command
  0x28,   TMR_SR_OPCODE_READ_TAG_DATA
  0x07d0, timeout
  0x00,   option byte
  0x3f,   banks to read TMR_GEN2_BANK_USER | TMR_GEN2_BANK_EPC_ENABLED | TMR_GEN2_BANK_RESERVED_ENABLED |TMR_GEN2_BANK_TID_ENABLED |TMR_GEN2_BANK_USER_ENABLED
  0x00, 0x00, 0x00, 0x00, starting wordaddress
  0x00   length (read complete bank)
*/

  // timeout is 2S                                 0x07d0 = 2000 mS
  //uint8_t configBlob[] ={0x88, 0x10, 0x00, 0x17, 0x07, 0xd0, 0x0f, 0xff, 0x01, 0x09, 0x28, 0x07, 0xd0, 0x00, 0x3f, 0x00, 0x00, 0x00, 0x00,0x00};

  // time out is 500 ms                          0x01f4 = 500 mS!!
  uint8_t configBlob[] ={0x88, 0x10, 0x00, 0x17, 0x01, 0xf4, 0x0f, 0xff, 0x01, 0x09, 0x28, 0x07, 0xd0, 0x00, 0x3f, 0x00, 0x00, 0x00, 0x00,0x00};

  sendMessage(TMR_SR_OPCODE_READ_TAG_ID_MULTIPLE, configBlob, sizeof(configBlob));

  if (msg[0] != ALL_GOOD) return RESPONSE_FAIL;

/*
 * Received: ff 86 22 06 01 88 00 00 17 00 00 00 01 01 28 00
          1f 00 00 30 20 ff f1 11 22 33 44 33 34 42 44 74
          6f 20 77 72 69 74 65 74 6f 20 63 61 72 64 00 00
          00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
          00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
          00 00 00 00 00 00 04 01 23 45 67 89 ab cd ef 10
          08 3b 72 34 00 e2 00 00 15 86 0e 02 88 15 40 80
          29 20 0c e2 00 34 12 01 56 ff 00 06 2f 80 29 01
          0e 01 53 10 0d 5f fb ff ff dc 00 f2 3b
0    ff      header
1    86      length of message in bytes (134 decimal)
2    22      opcode
3    06 01   buffer is full (timeut is too long / read too long) / normally 0000)
5    88      multiselect
6    00      option byte
7    00 17   searchflag
9    00 00 00 01 one data element included
13   01      embedded feedback
14   28      from opcode 0x28
15   00 1f   successfull read counter
17   00 00   failure reading counter

19   30     user memory
20    20     length 32 words = 64 bytes
21           ff f1 11 22 33 44 33 34 42 44 74 6f 20 77 72 69
             74 65 74 6f 20 63 61 72 64 00 00 00 00 00 00 00
             00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
             00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00

85   00    reserved memory
86    04    length 4 words = 8 bytes
87          01 23 45 67 89 ab cd ef

95   10    length EPC memory
96    08    length 8 words = 16 bytes
97    3b 72 EPC CRC
99    34 00 EPC PCword
101   e2 00 00 15 86 0e 02 88 15 40 80 29  EPC

113   20   TID
114    0c  length 12 words = 24 bytes
115    e2 00 34 12 01 56 ff 00 06 2f 80 29 01 0e 01 53 10 0d 5f fb ff ff dc 00

139    f2 3b  msg CRC
*/
  read->TagCount = msg[11] << 8 | msg[12];     // Uint32 to Uint16
  read->succesCount = msg[15] << 8 | msg[16];
  read->failureCount = msg[17] << 8 | msg[18];

  if (_printDebug)
  {
      DPRINT(F("Tagcount: "));
      DPRINT(read->TagCount);

      DPRINT(F(" succesCount: "));
      DPRINT(read->succesCount);

      DPRINT(F(" failurecount: "));
      DPRINTLN(read->failureCount);
  }

  // was a NO tag succesfully read ?
  if(read->succesCount == 0) return RESPONSE_IS_NOTAGFOUND;

  // read the tag-id buffer from Nano (provides more details)
  uint8_t flags[] = {0xf, 0xff, 0x00};
  sendMessage(TMR_SR_OPCODE_GET_TAG_ID_BUFFER, flags, sizeof(flags));

  if (msg[0] != ALL_GOOD) return RESPONSE_FAIL;

/*
 *
 * TYPICAL RESPONSE:  [FF] [EB] [29] [00] [00] [0F] [FF] [00] [03] [01] [E9] [11] [0E] [0E] [70] [00] [00] [00] [39] [00] [AE] [05] [03] [C0] [30] [20] [00] [00] [00] [00] [00] [00] [00] [00] [00] [00] [00] [00] [00] [00] [00] [00] [00] [00] [00] [00] [00] [00] [00] [00] [00] [00] [00] [00] [00] [00] [00] [00] [00] [00] [00] [00] [00] [00] [00] [00] [00] [00] [00] [00] [00] [00] [00] [00] [00] [00] [00] [00] [00] [00] [00] [00] [00] [00] [00] [00] [00] [00] [00] [00] [00] [04] [00] [00] [00] [00] [00] [00] [00] [00] [10] [08] [78] [DE] [30] [00] [E2] [00] [00] [15] [86] [0E] [02] [68] [15] [50] [7D] [BB] [20] [0C] [E2] [00] [34] [12] [01] [5F] [FF] [00] [06] [2F] [7D] [BB] [02] [19] [01] [5C] [30] [0D] [5F] [FB] [FF] [FF] [DC] [00] [80] [02] [00] [00] [00] [80] [30] [00] [E2] [00] [00] [15] [86] [0E] [02] [68] [15] [50] [7D] [BB] [78] [DE] [01] [E9] [11] [0E] [0E] [70] [00] [00] [00] [52] [00] [81] [05] [00] [00] [80] [02] [00] [00] [00] [80] [30] [00] [E2] [00] [00] [15] [86] [0E] [02] [72] [15] [40] [80] [09] [9B] [4D] [01] [E9] [11] [0E] [0E] [70] [00] [00] [00] [66] [00] [81] [05] [00] [00] [80] [02] [00] [00] [00] [80] [30] [00] [E2] [00] [00] [15] [86] [0E] [02] [72] [15] [40] [80] [09] [9B] [4D]

response:
BYTE
 0   [FF]
 1   [EB]   LENGTH OF MESSAGE
 2   [29]   OPCODE
 3   [00] [00]  STATUS (ALL GOOD)
 5   [0F] [FF]  METADATA INCLUDED
 7   [00] [03]  Number of data entries

 9   [01]       first data entry
 10  [E9]                rssi
 11  [11]                ANTENNA (always 1 1 as Nano only has 1 antenna)
 12  [0E] [0E] [70]      FREQUENCY
 15  [00] [00] [00] [39] timestamp
 19  [00] [AE]           PHASE
 21  [05]                PROTOCOL (GEN2)
 22  [03] [C0]           LENGTH OF BANK DATA

 24 [30]   //USER BANK
 25 [20]   LENGTH 32 words = 64 bytes
 26 [00] [00] [00] [00] [00] [00] [00] [00] [00] [00] [00] [00] [00] [00] [00] [00] [00] [00] [00] [00] [00] [00] [00] [00] [00] [00] [00] [00] [00] [00] [00] [00] [00] [00] [00] [00] [00] [00] [00] [00] [00] [00] [00] [00] [00] [00] [00] [00] [00] [00] [00] [00] [00] [00] [00] [00] [00] [00] [00] [00] [00] [00] [00] [00]

 91  [00]  // reserved bank
 92  [04]  // 4 words / 8 bytes
 93  [00] [00] [00] [00] [00] [00] [00] [00]

 102 [10]  // EPC bank
 103 [08]  // 8 words / 16 bytes
 104 [78] [DE] [30] [00] [E2] [00] [00] [15] [86] [0E] [02] [68] [15] [50] [7D] [BB]


 120  [20]  TID
 121  [0C]  length ( 12 words / 24 bytes)
 122  [E2] [00] [34] [12] [01] [5F] [FF] [00] [06] [2F] [7D] [BB] [02] [19] [01] [5C] [30] [0D] [5F] [FB] [FF] [FF] [DC] [00]


 146  [80]  GPIO
 147  [02]  TMR_TRD_METADATA_FLAG_GEN2_TARGET

 108  [00] [00] [00] [80] length EPC in bits ( 128/8 = 16 bytes)
 99   [30] [00] PC word
 101  [E2] [00] [00] [15] [86] [0E] [02] [68] [15] [50] [7D] [BB]  EPC
 113  [78] [DE]  EPC CRC

 [01]  second data element
 [E9]  rssi
 [11]  antenna
 [0E] [0E] [70] frequency
 [00] [00] [00] [52] timestampe
 [00] [81] phase
 [05] protocol
 [00]  [00]  NO DATA!!
 [80]  GPIO
 [02]  TMR_TRD_METADATA_FLAG_GEN2_TARGET
 [00] [00] [00] [80] EPC length
 [30] [00]  PCword
 [E2] [00] [00] [15] [86] [0E] [02] [72] [15] [40] [80] [09]  EPC
 [9B] [4D] EPC CRC

 [01]  third data element
 [E9]  RSSI
 [11]  antenna
 [0E] [0E] [70] frequency
 [00] [00] [00] [66] timestamp
 [00] [81] phase
 [05] protocol
 [00] 00] NO data !!!
 [80] GPIO status
 [02] TMR_TRD_METADATA_FLAG_GEN2_TARGET

 [00] [00] [00] [80] EPC length
 [30] [00]  PCword
 [E2] [00] [00] [15] [86] [0E] [02] [72] [15] [40] [80] [09]  EPC
 [9B] [4D] EPC CRC

*/
  // extract the RSSI
  read->rssi = msg[10] - 256;

  // Frequency of the tag detected is loaded over three bytes
  read->frequency = 0;
  for (uint8_t x = 0 ; x < 3 ; x++)
    read->frequency |= (uint32_t)msg[12 + x] << (8 * (2 - x));

  // extract Timestamp since starting to read
  read->timestamp = 0;
  for (uint8_t x = 0 ; x < 4 ; x++)
    read->timestamp |= (uint32_t)msg[15 + x] << (8 * (3 - x));

  // extract complete bank data
  uint16_t datalength = (msg[22]<<8 | msg[23]) / 8;  // length in bits to bytes

  // starting point to extract individual banks
  uint8_t offset = 24;

  for (uint8_t i = 0; i < datalength; ){

      uint8_t bank = msg[offset + i++] >> 4 & 0xf;   // get current bank data
      uint8_t bankLength = msg[offset + i++] * 2 ;   // bank data length is in words  * 2 = bytes

      if (bank == TMR_GEN2_BANK_USER) {

            if (bankLength > read->userMemData.max) bankLength = read->userMemData.max;
            for (uint8_t j = 0; j < bankLength; j++)
                read->userMemData.list[j] = msg[offset + i + j];

            read->userMemData.len = bankLength;
            i += bankLength;
      }
      else if (bank == TMR_GEN2_BANK_TID) {

            if (bankLength > read->tidMemData.max) bankLength = read->tidMemData.max;
            for (uint8_t j = 0; j < bankLength; j++)
                read->tidMemData.list[j] = msg[offset+ i + j];

            read->tidMemData.len = bankLength;
            i += bankLength;
      }
      else if (bank == TMR_GEN2_BANK_RESERVED) {

            if (bankLength > read->reservedMemData.max) bankLength = read->reservedMemData.max;
            for (uint8_t j = 0; j < bankLength; j++)
                read->reservedMemData.list[j] = msg[offset+ i + j];

            read->reservedMemData.len = bankLength;
            i += bankLength;
      }
      else if (bank == TMR_GEN2_BANK_EPC) {

            if (bankLength > read->epcMemData.max) bankLength = read->epcMemData.max;
            for (uint8_t j = 0; j < bankLength; j++){
                read->epcMemData.list[j] = msg[offset+ i + j];

                // extract EPC seperate
                if (j > 3 ) {  // skip CRC and PCWORD

                    if (j-4 < TMR_MAX_EPC_BYTE_COUNT)
                        read->epc[j-4] = read->epcMemData.list[j];
                }

            }
            read->epcMemData.len = bankLength;
            read->epclen = bankLength - 4;

            i += bankLength;
      }
      else
        return(RESPONSE_FAIL);
  }

  return(ALL_GOOD);
}

//This writes data to the tag. 0, 4, 16 or 64 bytes may be available.
//Writes to the first spot 0x00 and fills up as much of the bytes as user provides
//Use with caution. Function doesn't control which tag hears the command.
uint8_t RFID::writeUserData(uint8_t *userData, uint8_t userDataLength, uint16_t timeOut)
{
  uint8_t bank = 0x03; //User memory
  uint8_t address = 0x00;

  return (writeData(bank, address, userData, userDataLength, timeOut));
}


/**
 * Special add May2020 (update February 2021 adding bank selection)
 *
 * Read data from the bank starting at a specific address with specific length
 * The original readUserData() will read the complete user bank.
 *
 * Address is in WORDS (word = 2 bytes)
 * Maximum length to read is in WORDS
 * DatalengthRead is in BYTES and will be returned as an even number
 *
 * bank
 * 00 reserved
 * 01 EPC bank
 * 02 TID
 * 03 User
 */
//TODO Add support for accessPassword
uint8_t RFID::readDataRegion(uint8_t bank, uint32_t address, uint8_t length, uint8_t *dataRead, uint8_t &dataLengthRead, uint16_t timeOut)
{
  uint8_t data[8];

  //Insert timeout
  data[0] = timeOut >> 8 & 0xFF; //Timeout msB in ms
  data[1] = timeOut & 0xFF; //Timeout lsB in ms

  data[2] = bank; // User data bank

  //Splice address into array
  for (uint8_t x = 0 ; x < sizeof(address) ; x++)
    data[3 + x] = address >> (8 * (3 - x)) & 0xFF;

  data[7] = length; //Number of 16-bit chunks to read.

  sendMessage(TMR_SR_OPCODE_READ_TAG_DATA, data, sizeof(data), timeOut);

  if (msg[0] == ALL_GOOD) //We received a good response
  {
    uint16_t status = (msg[3] << 8) | msg[4];

    if (status == 0x0000)
    {
      uint8_t responseLength = msg[1];

      if (responseLength < dataLengthRead) //User wants us to read more than we have available
        dataLengthRead = responseLength;

      //There is a case here where responseLegnth is more than dataLengthRead, in which case we ignore (don't load) the additional bytes
      //Load limited response data into caller's array
      for (uint8_t x = 0 ; x < dataLengthRead ; x++)
        dataRead[x] = msg[5 + x];

      return (RESPONSE_SUCCESS);
    }
  }

  //Else - msg[0] was timeout or other
  dataLengthRead = 0; //Inform caller that we weren't able to read anything

  return (RESPONSE_FAIL);

}

/**
 * Special add May2020 (update February 2021 adding bank selection)
 *
 * Write data to the user bank starting at a specific address with specific length
 * The original writeUserData() will write data to the user bank always starting at position 0.
 *
 * Address is in WORDS (word = 2 bytes)
 * DatalengtoRecord is in BYTES and MUST be an even number else the last byte is lost
 *
 *  bank
 * 00 reserved
 * 01 EPC bank
 * 02 TID
 * 03 User
 */
//Use with caution. Function doesn't control which tag hears the command.
uint8_t RFID::writeDataRegion(uint8_t bank, uint32_t address, uint8_t *dataToRecord, uint8_t dataLengthToRecord, uint16_t timeOut)
{
  dataLengthToRecord = (dataLengthToRecord / 2) * 2;    // make sure on 16 bit boundery = 2 bytes
  uint8_t data[8 + dataLengthToRecord];

  //Pre-load array options
  data[0] = timeOut >> 8 & 0xFF; //Timeout msB in ms
  data[1] = timeOut & 0xFF; //Timeout lsB in ms
  data[2] = 0x00; //Option initialize

  //Splice address into array
  for (uint8_t x = 0 ; x < sizeof(address) ; x++)
    data[3 + x] = address >> (8 * (3 - x)) & 0xFF;

  data[7] = bank;      //  //Bank 3 = User Memory

  //Splice data into array
  for (uint8_t x = 0 ; x < dataLengthToRecord ; x++)
    data[8 + x] = dataToRecord[x];

  sendMessage(TMR_SR_OPCODE_WRITE_TAG_DATA, data, sizeof(data), timeOut);

  if (msg[0] == ALL_GOOD) //We received a good response
  {
    uint16_t status = (msg[3] << 8) | msg[4];

    if (status == 0x0000)
      return (RESPONSE_SUCCESS);
  }

  //Else - msg[0] was timeout or other
  return (RESPONSE_FAIL);
}


//Write the kill password. Should be 4 bytes long
uint8_t RFID::writeKillPW(uint8_t *password, uint8_t passwordLength, uint16_t timeOut)
{
  uint8_t bank = 0x00; //Passwords bank
  uint8_t address = 0x00; //Kill password address

  return (writeData(bank, address, password, passwordLength, timeOut));
}

//Read the kill password. Should be 4 bytes long
uint8_t RFID::readKillPW(uint8_t *password, uint8_t &passwordLength, uint16_t timeOut)
{
  uint8_t bank = 0x00; //Passwords bank
  uint8_t address = 0x00; //Kill password address

  return (readData(bank, address, password, passwordLength, timeOut));
}

//Write the access password. Should be 4 bytes long
uint8_t RFID::writeAccessPW(uint8_t *password, uint8_t passwordLength, uint16_t timeOut)
{
  uint8_t bank = 0x00; //Passwords bank
  uint8_t address = 0x02; //Access password address

  return (writeData(bank, address, password, passwordLength, timeOut));
}

//Read the access password. Should be 4 bytes long
uint8_t RFID::readAccessPW(uint8_t *password, uint8_t &passwordLength, uint16_t timeOut)
{
  uint8_t bank = 0x00; //Passwords bank
  uint8_t address = 0x02; //Access password address

  return (readData(bank, address, password, passwordLength, timeOut));
}

//Read the unique TID of the tag. Should be 20 bytes long
uint8_t RFID::readTID(uint8_t *tid, uint8_t &tidLength, uint16_t timeOut)
{
  uint8_t bank = 0x02; //Bank for TID,
  uint8_t address = 0x00; //TID starts at 0

  return (readData(bank, address, tid, tidLength, timeOut));
}

//Writes a data array to a given bank and address
//Allows for writing of passwords and user data
//TODO Add support for accessPassword
//TODO Add support for writing to specific tag
uint8_t RFID::writeData(uint8_t bank, uint32_t address, uint8_t *dataToRecord, uint8_t dataLengthToRecord, uint16_t timeOut)
{
  //Example: FF  0A  24  03  E8  00  00  00  00  00  03  00  EE  58  9D
  //FF 0A 24 = Header, LEN, Opcode
  //03 E8 = Timeout in ms
  //00 = Option initialize
  //00 00 00 00 = Address
  //03 = Bank
  //00 EE = Data
  //58 9D = CRC

  uint8_t data[8 + dataLengthToRecord];

  //Pre-load array options
  data[0] = timeOut >> 8 & 0xFF; //Timeout msB in ms
  data[1] = timeOut & 0xFF; //Timeout lsB in ms
  data[2] = 0x00; //Option initialize

  //Splice address into array
  for (uint8_t x = 0 ; x < sizeof(address) ; x++)
    data[3 + x] = address >> (8 * (3 - x)) & 0xFF;

  //Bank 0 = Passwords
  //Bank 1 = EPC Memory Bank
  //Bank 2 = TID
  //Bank 3 = User Memory
  data[7] = bank;

  //Splice data into array
  for (uint8_t x = 0 ; x < dataLengthToRecord ; x++)
    data[8 + x] = dataToRecord[x];

  sendMessage(TMR_SR_OPCODE_WRITE_TAG_DATA, data, sizeof(data), timeOut);

  if (msg[0] == ALL_GOOD) //We received a good response
  {
    uint16_t status = (msg[3] << 8) | msg[4];

    if (status == 0x0000)
      return (RESPONSE_SUCCESS);
  }

  //Else - msg[0] was timeout or other
  return (RESPONSE_FAIL);
}

//Reads a given bank and address to a data array
//Allows for writing of passwords and user data
//TODO Add support for accessPassword
//TODO Add support for writing to specific tag
uint8_t RFID::readData(uint8_t bank, uint32_t address, uint8_t *dataRead, uint8_t &dataLengthRead, uint16_t timeOut)
{
  //Bank 0
  //response: [00] [08] [28] [00] [00] [EE] [FF] [11] [22] [12] [34] [56] [78]
  //[EE] [FF] [11] [22] = Kill pw
  //[12] [34] [56] [78] = Access pw

  //Bank 1
  //response: [00] [08] [28] [00] [00] [28] [F0] [14] [00] [AA] [BB] [CC] [DD]
  //[28] [F0] = CRC
  //[14] [00] = PC
  //[AA] [BB] [CC] [DD] = EPC

  //Bank 2
  //response: [00] [18] [28] [00] [00] [E2] [00] [34] [12] [01] [6E] [FE] [00] [03] [7D] [9A] [A3] [28] [05] [01] [6B] [00] [05] [5F] [FB] [FF] [FF] [DC] [00]
  //[E2] = CIsID
  //[00] [34] [12] = Vendor ID = 003, Model ID == 412
  //[01] [6E] [FE] [00] [03] [7D] [9A] [A3] [28] [05] [01] [69] [10] [05] [5F] [FB] [FF] [FF] [DC] [00] = Unique ID (TID)

  //Bank 3
  //response: [00] [40] [28] [00] [00] [41] [43] [42] [44] [45] [46] [00] [00] [00] [00] [00] [00] ...
  //User data

  uint8_t data[8];

  //Insert timeout
  data[0] = timeOut >> 8 & 0xFF; //Timeout msB in ms
  data[1] = timeOut & 0xFF; //Timeout lsB in ms

  data[2] = bank; //Bank

  //Splice address into array
  for (uint8_t x = 0 ; x < sizeof(address) ; x++)
    data[3 + x] = address >> (8 * (3 - x)) & 0xFF;

  data[7] = dataLengthRead / 2; //Number of 16-bit chunks to read.
  //0x00 will read the entire bank but may be more than we expect (both Kill and Access PW will be returned when reading bank 1 from address 0)

  //When reading the user data area we need to read the entire bank
  if(bank == 0x03) data[7] = 0x00;

  sendMessage(TMR_SR_OPCODE_READ_TAG_DATA, data, sizeof(data), timeOut);

  if (msg[0] == ALL_GOOD) //We received a good response
  {
    uint16_t status = (msg[3] << 8) | msg[4];

    if (status == 0x0000)
    {
      uint8_t responseLength = msg[1];

      if (responseLength < dataLengthRead) //User wants us to read more than we have available
        dataLengthRead = responseLength;

      //There is a case here where responseLegnth is more than dataLengthRead, in which case we ignore (don't load) the additional bytes
      //Load limited response data into caller's array
      for (uint8_t x = 0 ; x < dataLengthRead ; x++)
        dataRead[x] = msg[5 + x];

      return (RESPONSE_SUCCESS);
    }
  }

  //Else - msg[0] was timeout or other
  dataLengthRead = 0; //Inform caller that we weren't able to read anything

  return (RESPONSE_FAIL);
}

//Send the appropriate command to permanently kill a tag. If the password does not
//match the tag's pw it won't work. Default pw is 0x00000000
//Use with caution. This function doesn't control which tag hears the command.
//TODO Can we add ability to write to specific EPC?
uint8_t RFID::killTag(uint8_t *password, uint8_t passwordLength, uint16_t timeOut)
{
  uint8_t data[4 + passwordLength];

  data[0] = timeOut >> 8 & 0xFF; //Timeout msB in ms
  data[1] = timeOut & 0xFF; //Timeout lsB in ms
  data[2] = 0x00; //Option initialize

  //Splice password into array
  for (uint8_t x = 0 ; x < passwordLength ; x++)
    data[3 + x] = password[x];

  data[3 + passwordLength] = 0x00; //RFU

  sendMessage(TMR_SR_OPCODE_KILL_TAG, data, sizeof(data), timeOut);

  if (msg[0] == ALL_GOOD) //We received a good response
  {
    uint16_t status = (msg[3] << 8) | msg[4];

    if (status == 0x0000)
      return (RESPONSE_SUCCESS);
  }

  //Else - msg[0] was timeout or other
  return (RESPONSE_FAIL);
}

//Checks incoming buffer for the start characters
//Returns true if a new message is complete and ready to be cracked
bool RFID::check()
{
  while (_nanoSerial->available())
  {
    uint8_t incomingData = _nanoSerial->read();

    //Wait for header byte
    if (_head == 0 && incomingData != 0xFF)
    {
      //Do nothing. Ignore this byte because we need a start byte
    }
    else
    {
      //Load this value into the array
      msg[_head++] = incomingData;

      _head %= MAX_MSG_SIZE; //Wrap variable

      if ((_head > 0) && (_head == msg[1] + 7))
      {
        //We've got a complete sentence!

        //Erase the remainder of the array
        for (uint8_t x = _head ; x < MAX_MSG_SIZE ; x++)
          msg[x] = 0;

        _head = 0; //Reset

        //Used for debugging: Does the user want us to print the command to serial port?
        if (_printDebug == true)
        {
          DPRINT(F("response: "));
          printMessageArray();
        }
#ifdef ARDUINO_ARCH_ESP32
        else
          delay(ESP32_DELAY);
#endif

        if (_continuousModeTemp){        // added for temperature special (may2020 / paulvha)
/*
 *
 * September 2020
 * [FF] [3D] [22] [00] [00] [10] [01] [1F] [0F] [FF] [01] [01] [DD] [11] [0D] [37] [FC] [00] [00] [00] [B3] [00] [AE] [05] [00] [90] [FF] [00] [06] [2F] [80] [29] [01] [0E] [01] [54] [00] [0D] [5F] [FB] [FF] [FF] [DC] [00] [80] [02] [00] [00] [00] [80] [34] [00] [E2] [00] [00] [15] [86] [0E] [02] [88] [15] [40] [80] [29] [3B] [72]


[FF]                header
[3D]                length
[22]                opcode
[00] [00]           status
[10]                option byte / continuous reading
[01] [1F]           searchflag
[0F] [FF]           metadata to return
[01]                number of tags
[01]                count
[DD]                RSSI
[11]                antenna
[0D] [37] [FC]      frequency
[00] [00] [00] [B3] Timestamp in ms since last keep alive msg
[00] [AE]           phase
[05]                GEN5
[00] [90]           data length in bits = 144 / 8 = 18
[FF] [00] [06] [2F] [80] [29] [01] [0E] [01] [54] [00] [0D] [5F] [FB] [FF] [FF] [DC] [00]

[80]                GPIO status
[02]                initial Q
[00]                GEN2_LINKFREQUENCY
[00]                GEN2_TARGET_A
[00] [80]           length EPC in bits = 128 / 8 = 16 bytes
[34] [00]           PC word
[E2] [00] [00] [15] [86] [0E] [02] [88] [15] [40] [80] [29] EPC
[3B] [72]           EPC CRC
*/

            if (msg[5] == 0x10) return true;        // we have valid data

// May 2020 & September 2020
// positie   14  (dus 1A is de temperatuure = 26C)
//             0    1    2    3    4    5   6    7    8    9    10   11   12   13   14
// response:  [FF] [0A] [22] [00] [00] [00] [01] [1F] [02] [82] [00] [82] [00] [01] [1A]
            /*  0  [FF]
             *  1  [0A]         length
             *  2  [22]         opcode
             *  3  [00] [00]    status
             *  5  [00]         port
             *  6  [01] [1F]    search flag
             *  8  [02]         STATS UPDATE
             *  9  [82] [00]    EBV is set ( > 0x80)
             *  11 [82] [00]    the real indicter (temperature)
             *  13 [01]         1 data byte
             *  14 [1A]         TEMP !!!! (26 C)
             */
            else if (msg[8] == 0x02)  {             // stats update
                if (msg[13] == 1 && msg[11] == 0x82) _contTemp = msg[14];  // get temperature
            }

            return(false);
        }
      }
    }
  }

  return (false);
}

//See parseResponse for breakdown of fields
//Pulls the number of EPC bytes out of the response
//Often this is 12 bytes
uint8_t RFID::getTagEPCBytes(void)
{
  uint16_t epcBits = 0; //Number of bits of EPC (including PC, EPC, and EPC CRC)

  uint8_t tagDataBytes = getTagDataBytes(); //We need this offset

  for (uint8_t x = 0 ; x < 2 ; x++)
    epcBits |= (uint16_t)msg[27 + tagDataBytes + x] << (8 * (1 - x));
  uint8_t epcBytes = epcBits / 8;
  epcBytes -= 4; //Ignore the first two bytes (PC word) and last two bytes (CRC EPC)

  return (epcBytes);
}

// get the data that was received in continuous mode from the msg
// See parseResponse for breakdown of fields
// added september 2020
uint8_t RFID::getTagData(uint8_t *buf, uint8_t len)
{
    uint8_t tagDataBytes = getTagDataBytes();

    // no data
    if (tagDataBytes == 0 ) return(0);

    // prevent buffer overrun
    if (tagDataBytes > len ) tagDataBytes = len;

    // copy the data
    for (uint8_t x = 0 ; x < tagDataBytes ; x++)
        buf[x] = msg[26 + x];

    // return number of bytes in buffer
    return(tagDataBytes);
}

//See parseResponse for breakdown of fields
//Pulls the number of data bytes out of the response
//Often this is zero
uint8_t RFID::getTagDataBytes(void)
{
  //Number of bits of embedded tag data
  uint16_t tagDataLength = 0;

  // get number of data bits
  for (uint8_t x = 0 ; x < 2 ; x++)
    tagDataLength |= (uint16_t)msg[24 + x] << (8 * (1 - x));

  // turn to bytes
  uint8_t tagDataBytes = tagDataLength / 8;
  if (tagDataLength % 8 > 0) tagDataBytes++; //Ceiling trick

  return (tagDataBytes);
}

//See parseResponse for breakdown of fields
//Pulls the timestamp since last Keep-Alive message from a full response record stored in msg
uint16_t RFID::getTagTimestamp(void)
{
  //Timestamp since last Keep-Alive message
  uint32_t timeStamp = 0;
  for (uint8_t x = 0 ; x < 4 ; x++)
    timeStamp |= (uint32_t)msg[17 + x] << (8 * (3 - x));

  return (timeStamp);
}

//See parseResponse for breakdown of fields
//Pulls the frequency value from a full response record stored in msg
uint32_t RFID::getTagFreq(void)
{
  //Frequency of the tag detected is loaded over three bytes
  uint32_t freq = 0;
  for (uint8_t x = 0 ; x < 3 ; x++)
    freq |= (uint32_t)msg[14 + x] << (8 * (2 - x));

  return (freq);
}

//See parseResponse for breakdown of fields
//Pulls the RSSI value from a full response record stored in msg
int8_t RFID::getTagRSSI(void)
{
  return (msg[12] - 256);
}

//This will parse whatever response is currently in msg into its constituents
//Mostly used for parsing out the tag IDs and RSSI from a multi tag continuous read
uint8_t RFID::parseResponse(void)
{
  //See http://www.thingmagic.com/images/Downloads/Docs/AutoConfigTool_1.2-UserGuide_v02RevA.pdf
  //for a breakdown of the response packet

  //Example response:
  //FF  28  22  00  00  10  00  1B  01  FF  01  01  C4  11  0E  16
  //40  00  00  01  27  00  00  05  00  00  0F  00  80  30  00  00
  //00  00  00  00  00  00  00  00  00  15  45  E9  4A  56  1D
  //  [0] FF = Header
  //  [1] 28 = Message length
  //  [2] 22 = OpCode
  //  [3, 4] 00 00 = Status
  //  [5 to 11] 10 00 1B 01 FF 01 01 = RFU 7 bytes
  //  [12] C4 = RSSI
  //  [13] 11 = Antenna ID (4MSB = TX, 4LSB = RX)
  //  [14, 15, 16] 0E 16 40 = Frequency in kHz
  //  [17, 18, 19, 20] 00 00 01 27 = Timestamp in ms since last keep alive msg
  //  [21, 22] 00 00 = phase of signal tag was read at (0 to 180)
  //  [23] 05 = Protocol ID
  //  [24, 25] 00 00 = Number of bits of embedded tag data [M bytes]
  //  [26 to M] (none) = Any embedded data
  //  [26 + M] 0F = RFU reserved future use
  //  [27, 28 + M] 00 80 = EPC Length [N bytes]  (bits in EPC including PC and CRC bits). 128 bits = 16 bytes
  //  [29, 30 + M] 30 00 = Tag EPC Protocol Control (PC) bits
  //  [31 to 42 + M + N] 00 00 00 00 00 00 00 00 00 00 15 45 = EPC ID
  //  [43, 44 + M + N] 45 E9 = EPC CRC
  //  [45, 46 + M + N] 56 1D = Message CRC

/* september 2020 read all
 *
 * response:
 *
 *
 * September 2020
 * [FF] [3D] [22] [00] [00] [10] [01] [1F] [0F] [FF] [01] [01] [DD] [11] [0D] [37] [FC] [00] [00] [00] [B3] [00] [AE] [05] [00] [90] [FF] [00] [06] [2F] [80] [29] [01] [0E] [01] [54] [00] [0D] [5F] [FB] [FF] [FF] [DC] [00] [80] [02] [00] [00] [00] [80] [34] [00] [E2] [00] [00] [15] [86] [0E] [02] [88] [15] [40] [80] [29] [3B] [72]


0 [FF]                 header
1 [3D]                 length
2 [22]                 opcode
3 [00] [00]            status
5 [10]                 option byte / continuous reading
6 [01] [1F]            searchflag
8 [0F] [FF]            metadata to return
10 [01]                number of tags
11 [01]                count
12 [DD]                RSSI
13 [11]                antenna
14 [0D] [37] [FC]      frequency
17 [00] [00] [00] [B3] Timestamp in ms since last keep alive msg
21 [00] [AE]            phase
23 [05]                 GEN5
24 [00] [90]            data length in bits = 144 / 8 = 18   [M bytes]
26 to M [FF] [00] [06] [2F] [80] [29] [01] [0E] [01] [54] [00] [0D] [5F] [FB] [FF] [FF] [DC] [00]

26 + M [80]                 GPIO status
27 + M [00] [80]            length EPC in bits = 128 / 8 = 16 bytes
29 + M [34] [00]            PC word
31 + M to N [E2] [00] [00] [15] [86] [0E] [02] [88] [15] [40] [80] [29] EPC
32 + M + N [3B] [72]            EPC CRC
*/

  uint8_t msgLength = msg[1] + 7; //Add 7 (the header, length, opcode, status, and CRC) to the LEN field to get total bytes
  uint8_t opCode = msg[2];
  uint16_t statusMsg = 0;

  //Check the CRC on this response
  uint16_t messageCRC = calculateCRC(&msg[1], msgLength - 3 ); //Ignore header (start spot 1), remove 3 bytes (header + 2 CRC)
  if ((msg[msgLength - 2] != (messageCRC >> 8)) || (msg[msgLength - 1] != (messageCRC & 0xFF)))
  {
    return (ERROR_CORRUPT_RESPONSE);
  }

  if (opCode == TMR_SR_OPCODE_READ_TAG_ID_MULTIPLE) //opCode = 0x22
  {
    //Based on the record length identify if this is a tag record, a temperature sensor record, or a keep-alive?
    if (msg[1] == 0x00) //Keep alive
    {
      //We have a Read cycle reset/keep-alive message
      //Sent once per second

      for (uint8_t x = 0 ; x < 2 ; x++)
        statusMsg |= (uint32_t)msg[3 + x] << (8 * (1 - x));

      if (statusMsg == 0x0400)
      {
        return (RESPONSE_IS_KEEPALIVE);
      }
      else if (statusMsg == 0x0504)
      {
        return (RESPONSE_IS_TEMPTHROTTLE);
      }
    }
    else if (msg[1] == 0x08) //Unknown
    {
      return (RESPONSE_IS_UNKNOWN);
    }
    //response:  [FF] [0E] [22] [04] [00] [00] [01] [1F] [00] [00] [00] [00] [00] [01] [28] [00] [00] [00] [00]
    else if (msg[1] == 0x0E)  // added September 2020
    {
      if (statusMsg == 0x0400)
      {
        return (RESPONSE_IS_KEEPALIVE);
      }
    }
    //response:  [FF] [0A] [22] [00] [00] [00] [01] [1F] [02] [82] [00] [82] [00] [01] [1B]
    else if (msg[1] == 0x0A)  // added September 2020 (should not get here as false is returned on check()
    {
        return (RESPONSE_IS_TEMPERATURE);
    }
    else //Full tag record
    {
      // This is a full tag response
      // User can now pull out RSSI, frequency of tag, timestamp, EPC, Protocol control bits, EPC CRC, CRC
      return (RESPONSE_IS_TAGFOUND);
    }
  }

  if (_printDebug == true)
  {
      DPRINT(F("Unknown opcode in response: 0x"));
      DPRINTLN(opCode, HEX);
  }

  return (ERROR_UNKNOWN_OPCODE);
}

//Given an opcode, a piece of data, and the size of that data, package up a sentence and send it
void RFID::sendMessage(uint8_t opcode, uint8_t *data, uint8_t size, uint16_t timeOut, boolean waitForResponse)
{
  msg[1] = size; //Load the length of this operation into msg array
  msg[2] = opcode;

  //Copy the data into msg array
  for (uint8_t x = 0 ; x < size ; x++)
    msg[3 + x] = data[x];

  sendCommand(timeOut, waitForResponse); //Send and wait for response
}

//Given an array, calc CRC, assign header, send it out
//Modifies the caller's msg array
void RFID::sendCommand(uint16_t timeOut, boolean waitForResponse)
{
  msg[0] = 0xFF; //Universal header
  uint8_t messageLength = msg[1];

  uint8_t opcode = msg[2]; //Used to see if response from module has the same opcode

  //Attach CRC
  uint16_t crc = calculateCRC(&msg[1], messageLength + 2); //Calc CRC starting from spot 1, not 0. Add 2 for LEN and OPCODE bytes.
  msg[messageLength + 3] = crc >> 8;
  msg[messageLength + 4] = crc & 0xFF;

  //Used for debugging: Does the user want us to print the command to serial port?
  if (_printDebug == true)
  {
    //_debugSerial->prinxt(F("sendCommand: "));
    DPRINTLN(F("sendCommand: "));
    printMessageArray();
  }
#ifdef ARDUINO_ARCH_ESP32
  else
   delay(ESP32_DELAY);
#endif

  //Remove anything in the incoming buffer
  //TODO this is a bad idea if we are constantly readings tags
  while (_nanoSerial->available()) _nanoSerial->read();

  //Send the command to the module
  for (uint8_t x = 0 ; x < messageLength + 5 ; x++)
    _nanoSerial->write(msg[x]);

  //There are some commands (setBaud) that we can't or don't want the response
  if (waitForResponse == false)
  {
    _nanoSerial->flush(); //Wait for serial sending to complete
    return;
  }

  //For debugging, probably remove
  //for (uint8_t x = 0 ; x < 100 ; x++) msg[x] = 0;

  //Wait for response with timeout
  uint32_t startTime = millis();

  while (_nanoSerial->available() == false)
  {
    if (millis() - startTime > (timeOut * 2))
    {
      if (_printDebug == true) DPRINTLN(F("Time out 1: No response from module"));
      msg[0] = ERROR_COMMAND_RESPONSE_TIMEOUT;
      return;
    }
    delay(1);
  }

  // Layout of response in data array:
  // [0] [1] [2] [3]      [4]      [5] [6]  ... [LEN+4] [LEN+5] [LEN+6]
  // FF  LEN OP  STATUSHI STATUSLO xx  xx   ... xx      CRCHI   CRCLO
  messageLength = MAX_MSG_SIZE - 1; //Make the max length for now, adjust it when the actual len comes in
  startTime = millis();
  uint8_t spot = 0;
  while (spot < messageLength)
  {
    if (millis() - startTime > timeOut)
    {
      if (_printDebug == true){
           DPRINT(F("Time out 2: Incomplete response "));
           DPRINTLN(spot);
       }
      msg[0] = ERROR_COMMAND_RESPONSE_TIMEOUT;
      return;
    }

    if (_nanoSerial->available())
    {
      msg[spot] = _nanoSerial->read();

      if (spot == 1) //Grab the length of this response (spot 1)
        messageLength = msg[1] + 7; //Actual length of response is ? + 7 for extra stuff (header, Length, opcode, 2 status bytes, ..., 2 bytes CRC = 7)

      spot++;

      //There's a case were we miss the end of one message and spill into another message.
      //We don't want spot pointing at an illegal spot in the array
      spot %= MAX_MSG_SIZE; //Wrap condition
    }
  }

  //Used for debugging: Does the user want us to print the command to serial port?

  if (_printDebug == true)
  {
    DPRINT(F("response: "));
    printMessageArray();
  }
#ifdef ARDUINO_ARCH_ESP32
  else
    delay(ESP32_DELAY);
#endif

  //Check CRC
  crc = calculateCRC(&msg[1], messageLength - 3); //Remove header, remove 2 crc bytes
  if ((msg[messageLength - 2] != (crc >> 8)) || (msg[messageLength - 1] != (crc & 0xFF)))
  {
    msg[0] = ERROR_CORRUPT_RESPONSE;
    if (_printDebug == true) DPRINTLN(F("Corrupt response"));
    return;
  }

  //If crc is ok, check that opcode matches (did we get a response to the command we sent or a different one?)
  if (msg[2] != opcode)
  {
    msg[0] = ERROR_WRONG_OPCODE_RESPONSE;
    if (_printDebug == true) {
         DPRINT("Wrong opcode response expected ");
         DPRINT(opcode, HEX);
         DPRINT(" got ");
         DPRINTLN(msg[2],HEX);
     }
      //  DPRINTLN(F("Wrong opcode response"));
    return;
  }

  //If everything is ok, load all ok into msg array
  msg[0] = ALL_GOOD;

}

//Print the current message array - good for debugging, looking at how the module responded
//TODO Don't hardcode the serial stream
void RFID::printMessageArray(void)
{
  if(_printDebug == true) //If user hasn't enabled debug we don't know what port to debug to
  {
    uint16_t amtToPrint = msg[1] + 5;
    if (amtToPrint > MAX_MSG_SIZE) amtToPrint = MAX_MSG_SIZE; //Limit this size

    for (uint16_t x = 0 ; x < amtToPrint ; x++)
    {
      DPRINT(" [");
      if (msg[x] < 0x10) DPRINT("0");
      DPRINT(msg[x], HEX);
      DPRINT("]");
    }
    DPRINTLN();
  }
}

//Comes from serial_reader_l3.c
//ThingMagic-mutated CRC used for messages.
//Notably, not a CCITT CRC-16, though it looks close.
static uint16_t crctable[] =
{
  0x0000, 0x1021, 0x2042, 0x3063,
  0x4084, 0x50a5, 0x60c6, 0x70e7,
  0x8108, 0x9129, 0xa14a, 0xb16b,
  0xc18c, 0xd1ad, 0xe1ce, 0xf1ef,
};

//Calculates the magical CRC value
uint16_t RFID::calculateCRC(uint8_t *u8Buf, uint8_t len)
{
  uint16_t crc = 0xFFFF;

  for (uint8_t i = 0 ; i < len ; i++)
  {
    crc = ((crc << 4) | (u8Buf[i] >> 4)) ^ crctable[crc >> 12];
    crc = ((crc << 4) | (u8Buf[i] & 0x0F)) ^ crctable[crc >> 12];
  }

  return crc;
}

/*********************************************************************
 * Section added for GPIO control
 * version 1.0 paulvha August 2019
 *
 * Be aware the pins can only handle 3.3V (as the rest of the Nano)
 *********************************************************************/
/* Set GPIO output level
 *
 * @param gpio: GPIO to set between GPIO1 and LV4
 *
 * @param high
 *  HIGH set to high / 1 .
 *  LOW  set to low / 0 .
 *
 * Return code :
 * OK = ALL_GOOD
 * else error
 */
uint8_t RFID::setGPIO(uint8_t gpio, bool high)
{
  uint8_t data[2];
  if (gpio < GPI01 || gpio > LV4) return(ERROR_UNKNOWN_OPCODE);

  data[0] = gpio;
  data[1] = (high == true);
  sendMessage(TMR_SR_OPCODE_SET_USER_GPIO_OUTPUTS, data, sizeof(data));

  return(msg[0]);
}

/* read the GPIO status
 *
 * @param gpio: between GPIO1 and LV4
 *
 * @param *state
 *  HIGH is current reading is high.
 *  LOW  if current reading is low.
 *
 * Return code :
 * OK = ALL_GOOD
 * else error (ignore *state)
 *
 * answer looks like
 * 0  [FF]             header
 * 1  [0D]             length of data
 * 2  [66]             opcode
 * 3  [00] [00]        response code
 * 5  [01]             block count
 * 6  [01] [00] [01]   ID = 01 , output (1) / input (0), state high / low
 * 9  [02] [00] [00]    ""
 * 12 [03] [00] [00]    ""
 * 15 [04] [00] [01]    ""
 */
uint8_t RFID::getGPIO(uint8_t gpio, bool *state)
{
  uint8_t data = 0x01;

  if (gpio < GPI01 || gpio > LV4) return(ERROR_UNKNOWN_OPCODE);

  sendMessage(TMR_SR_OPCODE_GET_USER_GPIO_INPUTS, &data, 1);

  *state = (msg[(8 + (gpio-1) * 3)] == 1);

  return(msg[0]);
}

/* set GPIO direction
 *
 * @param gpio :  requested gpio / pin between GPIO1 and LV4
 *
 * @param out :
 *  GPIOout(1) for output
 *  GPIOin (0) for input
 *
 * @value :
 *  set in case of output (default = low/false)
 *
 * Return code :
 * OK = ALL_GOOD
 * else error
 */
uint8_t RFID::setGPIODirection(uint8_t gpio, bool out, bool value)
{
  uint8_t data[4];

  if (gpio < GPI01 || gpio > LV4) return(ERROR_UNKNOWN_OPCODE);

  data[0] = 1;  /* Option flag */
  data[1] = gpio;
  data[2] = (out == true) ? 1 : 0;
  data[3] = (value == true) ? 1 : 0;  /* New value if output */

  sendMessage(TMR_SR_OPCODE_SET_USER_GPIO_OUTPUTS, data, sizeof(data));

  return(msg[0]);
}

/* get the GPIO direction
 *
 * @param gpio :  requested gpio / pin
 *
 * return ALL_GOOD of succesfull (else error):
 *  *out = 1 or GPIOout if OUTPUT
 *  *out = 0 or GPIOin if input
 *
 * response looks like:
 * 0 [FF] header
 * 1 [02] length data
 * 2 [96] opcode
 * 3 [00] [00] repsonse code
 * 5 [01] always 1
 * 6 [01] input (0) or output (1)
*/
uint8_t RFID::getGPIODirection( uint8_t gpio, bool *out)
{
  if (gpio < GPI01 || gpio > LV4) return(ERROR_UNKNOWN_OPCODE);

  sendMessage(TMR_SR_OPCODE_SET_USER_GPIO_OUTPUTS, &gpio, 1);

  *out = (msg[6] == 1);

  return(msg[0]);
}

/** end section GPIO control *****************************************/
