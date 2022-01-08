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
*/

#include "Arduino.h" //Needed for Stream

// maximum receive buffer
#define MAX_MSG_SIZE 255

// opcodes
#define TMR_SR_OPCODE_VERSION 0x03
#define TMR_SR_OPCODE_SET_BAUD_RATE 0x06
#define TMR_SR_OPCODE_READ_TAG_ID_SINGLE 0x21
#define TMR_SR_OPCODE_READ_TAG_ID_MULTIPLE 0x22
#define TMR_SR_OPCODE_WRITE_TAG_ID 0x23
#define TMR_SR_OPCODE_WRITE_TAG_DATA 0x24
#define TMR_SR_OPCODE_KILL_TAG 0x26
#define TMR_SR_OPCODE_READ_TAG_DATA 0x28
#define TMR_SR_OPCODE_GET_TAG_ID_BUFFER 0x29        // special add / August 2020
#define TMR_SR_OPCODE_CLEAR_TAG_ID_BUFFER 0x2A
#define TMR_SR_OPCODE_MULTI_PROTOCOL_TAG_OP 0x2F
#define TMR_SR_OPCODE_GET_READ_TX_POWER 0x62
#define TMR_SR_OPCODE_GET_WRITE_TX_POWER 0x64
#define TMR_SR_OPCODE_GET_POWER_MODE 0x68
#define TMR_SR_OPCODE_GET_READER_OPTIONAL_PARAMS 0x6A
#define TMR_SR_OPCODE_GET_TEMPERATURE 0x72           // special add / May 2020
#define TMR_SR_OPCODE_GET_PROTOCOL_PARAM 0x6B
#define TMR_SR_OPCODE_SET_ANTENNA_PORT 0x91
#define TMR_SR_OPCODE_SET_TAG_PROTOCOL 0x93
#define TMR_SR_OPCODE_SET_READ_TX_POWER 0x92
#define TMR_SR_OPCODE_SET_WRITE_TX_POWER 0x94
#define TMR_SR_OPCODE_SET_REGION 0x97
#define TMR_SR_OPCODE_SET_POWER_MODE 0x98           // special add / February 2020
#define TMR_SR_OPCODE_SET_READER_OPTIONAL_PARAMS 0x9A
#define TMR_SR_OPCODE_SET_PROTOCOL_PARAM 0x9B

#define COMMAND_TIME_OUT  2000 //Number of ms before stop waiting for response from module

//Define all the ways functions can return
#define ALL_GOOD                        0
#define ERROR_COMMAND_RESPONSE_TIMEOUT  1
#define ERROR_CORRUPT_RESPONSE          2
#define ERROR_WRONG_OPCODE_RESPONSE     3
#define ERROR_UNKNOWN_OPCODE            4
#define RESPONSE_IS_TEMPERATURE         5
#define RESPONSE_IS_KEEPALIVE           6
#define RESPONSE_IS_TEMPTHROTTLE        7
#define RESPONSE_IS_TAGFOUND            8
#define RESPONSE_IS_NOTAGFOUND          9
#define RESPONSE_IS_UNKNOWN             10
#define RESPONSE_SUCCESS                11
#define RESPONSE_FAIL                   12

//Define the allowed regions - these set the internal freq of the module
#define REGION_INDIA        0x04
#define REGION_JAPAN        0x05
#define REGION_CHINA        0x06
#define REGION_EUROPE       0x08
#define REGION_KOREA        0x09
#define REGION_AUSTRALIA    0x0B
#define REGION_NEWZEALAND   0x0C
#define REGION_NORTHAMERICA 0x0D
#define REGION_OPEN         0xFF


/** Gen2 memory banks */  // special add / August 2020
/** Reserved bank (kill and access passwords) */
#define  TMR_GEN2_BANK_RESERVED 0x0
/** EPC memory bank */
#define   TMR_GEN2_BANK_EPC     0x1
/** TID memory bank */
#define   TMR_GEN2_BANK_TID     0x2
/** User memory bank */
#define   TMR_GEN2_BANK_USER    0x3


typedef struct TMR_uint8List  // special add / August 2020
{
  /** The array of values */
  uint8_t *list;
  /** The number of entries there is space for in the array */
  uint16_t max;
  /** The number of entries in the list - may be larger than max, indicating truncated data. */
  uint16_t len;
} TMR_uint8List;


#define TMR_MAX_EPC_BYTE_COUNT (32)  // special add / August 2020

/* For reasons unknown to me, the ESP32 needs a delay else the communication will fail.
 * Setting a delay of 1 is already doing the trick. Yield() did not help.
 * Paulvha / May 2021 - version 2.2 */
#ifdef ARDUINO_ARCH_ESP32
#define ESP32_DELAY 2
#endif

/**
 * A subset of the TagReadData as defined in tmr_tag_data.h
 *
 * A class to represent a read of an RFID tag.
 */
typedef struct TMR_TagReadData      // special add / August 2020
{
  /** The EPC tag that was read */
  uint8_t epc[TMR_MAX_EPC_BYTE_COUNT];

  /** EPC length */
  uint8_t epclen;

  /** Number of times the tag was read */
  uint16_t TagCount;

  /** Number of times the tag was read succesfully*/
  uint16_t succesCount;

   /** Number of times the tag was read in failure*/
  uint16_t failureCount;

  /** Strength of the signal received from the tag */
  int32_t rssi;

  /** RF carrier frequency the tag was read with */
  uint32_t frequency;

  /** Timestamp since starting to read*/
  uint32_t timestamp;

  /** Read EPC bank data bytes */
  TMR_uint8List epcMemData;

  /** Read TID bank data bytes */
  TMR_uint8List tidMemData;

  /** Read USER bank data bytes */
  TMR_uint8List userMemData;

  /** Read RESERVED bank data bytes */
  TMR_uint8List reservedMemData;

} TMR_TagReadData;


class RFID
{
  public:
    RFID(void);

    void begin(Stream &serialPort = Serial); //If user doesn't specify then Serial will be used

    void enableDebugging(Stream &debugPort = Serial); //Turn on command sending and response printing. If user doesn't specify then Serial will be used
    void disableDebugging(void);

    void setBaud(long baudRate);
    void getVersion(void);
    void setReadPower(int16_t powerSetting);
    void getReadPower();
    void setWritePower(int16_t powerSetting);
    void getWritePower();
    void setRegion(uint8_t region);
    void setAntennaPort();
    void setAntennaSearchList();
    void setTagProtocol(uint8_t protocol = 0x05);

    void startReading(void); //Disable filtering and start reading continuously
    void stopReading(void); //Stops continuous read. Give 1000 to 2000ms for the module to stop reading.


    void enableReadFilter(void);
    void disableReadFilter(void);

    void setReaderConfiguration(uint8_t option1, uint8_t option2);
    void getOptionalParameters(uint8_t option1, uint8_t option2);
    void setProtocolParameters(void);
    void getProtocolParameters(uint8_t option1, uint8_t option2);
    uint8_t parseResponse(void);

    uint8_t getTagEPCBytes(void); //Pull number of EPC data bytes from record response.
    uint8_t getTagDataBytes(void); //Pull number of tag data bytes from record response. Often zero.
    uint16_t getTagTimestamp(void); //Pull timestamp value from full record response
    uint32_t getTagFreq(void); //Pull Freq value from full record response
    int8_t getTagRSSI(void); //Pull RSSI value from full record response

    bool check(void);

    uint8_t readTagEPC(uint8_t *epc, uint8_t &epcLength, uint16_t timeOut = COMMAND_TIME_OUT);
    uint8_t writeTagEPC(uint8_t *newID, uint8_t newIDLength, uint16_t timeOut = COMMAND_TIME_OUT);

    // new February 2021
    uint8_t readTagPCW(uint8_t *epc, uint8_t &epcLength, uint16_t timeOut= COMMAND_TIME_OUT);
    uint8_t writeTagPCW(uint8_t *newID, uint8_t newIDLength, uint16_t timeOut = COMMAND_TIME_OUT);

    uint8_t readData(uint8_t bank, uint32_t address, uint8_t *dataRead, uint8_t &dataLengthRead, uint16_t timeOut = COMMAND_TIME_OUT);
    uint8_t writeData(uint8_t bank, uint32_t address, uint8_t *dataToRecord, uint8_t dataLengthToRecord, uint16_t timeOut = COMMAND_TIME_OUT);

    uint8_t readUserData(uint8_t *userData, uint8_t &userDataLength, uint16_t timeOut = COMMAND_TIME_OUT);
    uint8_t writeUserData(uint8_t *userData, uint8_t userDataLength, uint16_t timeOut = COMMAND_TIME_OUT);


    uint8_t readKillPW(uint8_t *password, uint8_t &passwordLength, uint16_t timeOut = COMMAND_TIME_OUT);
    uint8_t writeKillPW(uint8_t *password, uint8_t passwordLength, uint16_t timeOut = COMMAND_TIME_OUT);

    uint8_t readAccessPW(uint8_t *password, uint8_t &passwordLength, uint16_t timeOut = COMMAND_TIME_OUT);
    uint8_t writeAccessPW(uint8_t *password, uint8_t passwordLength, uint16_t timeOut = COMMAND_TIME_OUT);

    uint8_t readTID(uint8_t *tid, uint8_t &tidLength, uint16_t timeOut = COMMAND_TIME_OUT);

    uint8_t killTag(uint8_t *password, uint8_t passwordLength, uint16_t timeOut = COMMAND_TIME_OUT);

    void sendMessage(uint8_t opcode, uint8_t *data = 0, uint8_t size = 0, uint16_t timeOut = COMMAND_TIME_OUT, boolean waitForResponse = true);
    void sendCommand(uint16_t timeOut = COMMAND_TIME_OUT, boolean waitForResponse = true);

    void printMessageArray(void);

    uint16_t calculateCRC(uint8_t *u8Buf, uint8_t len);

    /**
     *  SPECIAL ADD / February 2020
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
     * see example
     */
    bool setPowerMode(uint8_t mode);

    /**
     *  SPECIAL ADD / MAY 2020
     *
     * read M6E nano temperature
     *
     * return :
     *  temperature internal = OK
     *  -1 = ERROR
     */
    int8_t getTemp();

    /** September 2020 special / paulvha
     *
     * read a one the memory banks on a tag while in continuous mode
     * @param bank :
     *  TMR_GEN2_BANK_RESERVED
     *  TMR_GEN2_BANK_EPC
     *  TMR_GEN2_BANK_TID
     *  TMR_GEN2_BANK_USER
     *
     * @param address :  Address is in WORDS (word = 2 bytes)
     *  Word address to start reading (normally 0)
     *
     * @param length:    WORDS (word = 2 bytes)
     *  number words to read from the bank (maximum 32)
     *
     * /reader/tagReadData/uniqueByData is automatically set. This
     * allows tags with the same EPC ID but different values in the
     * specified Gen2.ReadData memory location to be treated as unique
     * tags during inventories.
     *
     */
    void startReadingBank(uint8_t bank = TMR_GEN2_BANK_TID, uint32_t address = 0x0, uint8_t length = 0x0);
    uint8_t getTagData(uint8_t *buf, uint8_t len);// get the data that was received in continuous mode from the msg

    /** August 2020 special / paulvha
     * Read all the memory banks from a tag in a single timeout-read by the Nano
     * and store the memory banks in a structure
     *
     * @param read:
     *  pointer to store the memory bank data
     */
    uint8_t ReadingAllBanks(TMR_TagReadData *read);

    /**
     * Special add May 2020  (update February 2021 adding bank selection)
     *
     * Read or write data to the bank starting at a specific address with specific length
     * The original readUserData() will read the complete user bank.
     * The original writeUserData() will write data to the user bank always starting at position 0.
     *
     * Address is in WORDS (word = 2 bytes)
     * Maximum length is in WORDS
     * DatalengthRead and will be an even number
     * DatalengtoRecord is in BYTES and MUST be an even number else the last byte is lost
     *
     * Bank
     * 00 reserved
     * 01 EPC bank
     * 02 TID
     * 03 User
     */
    uint8_t readDataRegion(uint8_t bank,uint32_t address, uint8_t length, uint8_t *dataRead, uint8_t &dataLengthRead, uint16_t timeOut = COMMAND_TIME_OUT);
    uint8_t writeDataRegion(uint8_t bank,uint32_t address, uint8_t *dataToRecord, uint8_t dataLengthToRecord, uint16_t timeOut = COMMAND_TIME_OUT);

    /*********************************************************************
     * Section added for GPIO control
     * version 1.0 paulvha August 2019
     *
     * Be aware the pins can only handle 3.3V (as the rest of the Nano)
     *********************************************************************/
    // pin select (as described on the simultaneous RFID board)
    #define GPI01 1     // actually it could read LV1
    #define LV2 2
    #define LV3 3
    #define LV4 4

    // direction of the pin
    #define GPIOout 1   // set as output
    #define GPIOin 0    // set as input

    // GPIO opcodes for Nano (do not change)
    #define TMR_SR_OPCODE_GET_USER_GPIO_INPUTS    0x66
    #define TMR_SR_OPCODE_SET_USER_GPIO_OUTPUTS   0x96

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
    uint8_t setGPIO(uint8_t gpio, bool high);

    /* read the GPIO level
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
     */
    uint8_t getGPIO(uint8_t gpio, bool *state);

    /* set GPIO direction
     *
     * @param gpio: between GPIO1 and LV4
     *
     * @param out :
     *  GPIOout(1) for output
     *  GPIOin (0) for input
     *
     * @value :
     *  in case of output (default = low/false)
     */
    uint8_t setGPIODirection(uint8_t gpio, bool out, bool value = false);

    /* get the GPIO direction
     *
     * @param gpio: between GPIO1 and LV4
     *
     * @param out :
     *  *out = 1 or GPIOout if output
     *  *out = 0 or GPIOin if input
     *
     * Return code :
     * OK = ALL_GOOD
     * else error (ignore *out)
     */
    uint8_t getGPIODirection( uint8_t gpio, bool *out);

    /** end section GPIO control *****************************************/

    //Variables

    //This is our universal msg array, used for all communication
    //Before sending a command to the module we will write our command and CRC into it
    //And before returning, response will be recorded into the msg array. Default is 255 bytes.
    uint8_t msg[MAX_MSG_SIZE];

  private:

    Stream *_nanoSerial;            // The generic connection to user's chosen serial hardware

    Stream *_debugSerial;           // The stream to send debug messages to if enabled

    uint8_t _head = 0;              // Tracks the length of the incoming message as we poll the software serial

    boolean _printDebug = false;    // Flag to print the serial commands we are sending to the Serial port for debug

    boolean _continuousModeTemp;    // whether or not in continuous mode temperature measurement

    int8_t _contTemp;               // temperature measured in continuous mode
};
