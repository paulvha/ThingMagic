/*

  Version 1.0 / August 2020 / paulvha
  This example will get all the memory banks with a single NANO read from the tag. Once read the data is stored
  in the Nano buffer and obtained from there. The data is returned in a structure that is defined in
  Sparkfun_UHF_RFID_reader.h (included below). This sketch will also read the temperature of the Nano


typedef struct TMR_TagReadData
{
  // The EPC tag that was read
  uint8_t epc[TMR_MAX_EPC_BYTE_COUNT];

  // EPC length
  uint8_t epclen;

  // Number of times the tag was read
  uint16_t TagCount;

  // Number of times the tag was read succesfully
  uint16_t succesCount;

  // Number of times the tag was read in failure
  uint16_t failureCount;

  // Strength of the signal received from the tag
  int32_t rssi;

  // RF carrier frequency the tag was read with
  uint32_t frequency;

  / Timestamp since starting to read
  uint32_t timestamp;

  // Read EPC bank data bytes
  TMR_uint8List epcMemData;

  // Read TID bank data bytes
  TMR_uint8List tidMemData;

  // Read USER bank data bytes
  TMR_uint8List userMemData;

  // Read RESERVED bank data bytes
  TMR_uint8List reservedMemData;

} TMR_TagReadData

****************************************************************************************************
Original heading

  Reading multiple RFID tags, simultaneously!
  By: Nathan Seidle @ SparkFun Electronics
  Date: October 3rd, 2016
  https://github.com/sparkfun/Simultaneous_RFID_Tag_Reader


  If using the Simultaneous RFID Tag Reader (SRTR) shield, make sure the serial slide
  switch is in the 'SW-UART' position

*/

#include "SparkFun_UHF_RFID_Reader.h" //Library for controlling the M6E Nano module

/////////////////////////////////////////////////////////////
/* define Serial port to use                               */
/////////////////////////////////////////////////////////////
// if you plan to use softserial:  initialize that here, else comment it out the 2 lines
// ALSO comment out if your CPU board does not support SoftwareSerial.
#include <SoftwareSerial.h> //Used for transmitting to the device
SoftwareSerial softSerial(2, 3); //RX, TX

// define the serial port to use (E.g. softSerial, Serial1 etc)
#define NanoSerial softSerial

/////////////////////////////////////////////////////////////
/* define driver debug
 * 0 : no messages
 * 1 : request sending and receiving
*////////////////////////////////////////////////////////////
#define DEBUG 0

/////////////////////////////////////////////////////////////
/* define Region for Nano to operate
 * This will select the correct unlicensed frequency to use
 * in you area. Valid options :
 *
 * REGION_INDIA        REGION_JAPAN     REGION_CHINA
 * REGION_KOREA        REGION_AUSTRALIA REGION_NEWZEALAND
 * REGION_NORTHAMERICA REGION_EUROPE    REGION_OPEN
*////////////////////////////////////////////////////////////
#define NANOREGION REGION_NORTHAMERICA

/////////////////////////////////////////////////////////////////
/* define the size of the buffers to hold the data from the banks
*////////////////////////////////////////////////////////////////
#define BUFSIZE 128

//***************************************************
//       NO CHANGES NEEDED BEYOND THIS POINT        *
//***************************************************

RFID nano; //Create instance

// buffers to hold the tag data
uint8_t dataBuf[BUFSIZE];
uint8_t dataBuf1[BUFSIZE];
uint8_t dataBuf2[BUFSIZE];
uint8_t dataBuf3[BUFSIZE];
uint8_t dataBuf4[BUFSIZE];

struct TMR_TagReadData trd; // structure to hold the data

void setup()
{
  Serial.begin(115200);
  while (!Serial); //Wait for the serial port to come online

  Serial.println(F("Example15: read all memory banks"));

  if (DEBUG) nano.enableDebugging(Serial);

  if (setupNano(38400) == false) //Configure nano to run at 38400bps
  {
    Serial.println(F("Module failed to respond. Please check wiring."));
    while (1); //Freeze!
  }

  nano.setRegion(NANOREGION); //Set to the right region

  nano.setReadPower(500); //5.00 dBm. Higher values may caues USB port to brown out
  //Max Read TX Power is 27.00 dBm and may cause temperature-limit throttling

  init_trd_structure();  // initialize the structure to hold the data from the banks
}

void loop()
{
  // read temperature
  int8_t t = nano.getTemp();

  if (t == -1 ){
      Serial.println(F("Error getting Nano internal temperature"));
  }
  else {
    Serial.print(F("\nNano internal temperature (*C): "));
    Serial.println(t);
  }

  Serial.println(F("Press a key to begin scanning for tags."));
  while (!Serial.available()); //Wait for user to send a character
  Serial.read(); //Throw away the user's character

  // try find tag and read the banks
  uint8_t ret = nano.ReadingAllBanks(&trd);

  if(ret == ALL_GOOD)  display_data();

  else if (ret == RESPONSE_IS_NOTAGFOUND)
    Serial.println(F("No tag detected"));

  else if (ret == RESPONSE_FAIL)
    Serial.println(F("Error during reading tag"));

  else
    Serial.println(F("unknown issue"));
}

// displays the data from the structure after succesfull reading
void display_data()
{
  uint8_t i;
  char buffer[3];

  // counters
  Serial.print(F("counters:\t\t"));
  Serial.print(F("Tagcount: "));        // total read attemps
  Serial.print(trd.TagCount);
  Serial.print(F(" succesCount: "));    // number of times tag was read succesfull
  Serial.print(trd.succesCount);
  Serial.print(F(" failureCount: "));   // number of times tag was read in error
  Serial.println(trd.failureCount);

  // epc
  Serial.print(F("EPC:\t\t\t"));
  for (i = 0 ; i < trd.epclen; i++){
    sprintf (buffer, "%02X ", trd.epc[i]);
    Serial.print(buffer);
  }
  Serial.println();

  // rssi
  Serial.print(F("RSSI:\t\t\t"));
  Serial.println(trd.rssi);

  // frequency
  Serial.print(F("Frequency:\t\t"));
  Serial.println(trd.frequency);

   // timestamp
  Serial.print(F("timestamp:\t\t"));
  Serial.println(trd.timestamp);

  //user bank
  Serial.print("\nUser bank (");
  Serial.print(trd.userMemData.len);
  Serial.print("): \t");

  for( i = 0; i < trd.userMemData.len;i++)
    Serial.print(trd.userMemData.list[i], HEX);

  //reserved bank
  Serial.print("\n\nReserved bank (");
  Serial.print(trd.reservedMemData.len);
  Serial.print("):\t");
  for( i =0; i < trd.reservedMemData.len;i++)
    Serial.print(trd.reservedMemData.list[i], HEX);

  Serial.print("\n\tKill pswd:\t");
  for( i =0 ; i < 2 ; i++){
    sprintf (buffer, "%02X", trd.reservedMemData.list[i]);
    Serial.print(buffer);
  }

  Serial.print("\n\tAccess pswd:\t");
  for( i =2 ; i < 4 ; i++) {
    sprintf (buffer, "%02X", trd.reservedMemData.list[i]);
    Serial.print(buffer);
  }

  //Tid bank
  Serial.print("\n\nTid bank (");
  Serial.print(trd.tidMemData.len);
  Serial.print("):\t\t");

  for( i =0; i < trd.tidMemData.len;i++)
    Serial.print(trd.tidMemData.list[i], HEX);

  //EPC bank
  Serial.print("\n\nEPC bank (");
  Serial.print(trd.epcMemData.len);
  Serial.print("):\t\t");

  for( i =0; i < trd.epcMemData.len;i++)
    Serial.print(trd.epcMemData.list[i], HEX);

  Serial.print("\n\tEPC bank CRC:\t");
  for( i =0; i < 2; i++){
    sprintf (buffer, "%02X", trd.epcMemData.list[i]);
    Serial.print(buffer);
  }

  Serial.print("\n\tPC-WORD:\t");
  for(; i < 4; i++){
    sprintf (buffer, "%02X", trd.epcMemData.list[i]);
    Serial.print(buffer);
  }

  Serial.print("\n\tEPC:\t\t");
  for(; i < trd.epcMemData.len;i++)
    Serial.print(trd.epcMemData.list[i], HEX);

  Serial.println();
}


// initialize the structure to hold the data from the banks
void init_trd_structure()
{
    trd.userMemData.list = dataBuf1;
    trd.epcMemData.list = dataBuf2;
    trd.reservedMemData.list = dataBuf3;
    trd.tidMemData.list = dataBuf4;

    trd.userMemData.max = BUFSIZE;
    trd.userMemData.len = 0;
    trd.epcMemData.max = BUFSIZE;
    trd.epcMemData.len = 0;
    trd.reservedMemData.max = BUFSIZE;
    trd.reservedMemData.len = 0;
    trd.tidMemData.max = BUFSIZE;
    trd.tidMemData.len = 0;
}

//Gracefully handles a reader that is already configured and already reading continuously
//Because Stream does not have a .begin() we have to do this outside the library
boolean setupNano(long baudRate)
{
  nano.begin(NanoSerial); //Tell the library to communicate over serial port

  //Test to see if we are already connected to a module
  //This would be the case if the Arduino has been reprogrammed and the module has stayed powered
  NanoSerial.begin(baudRate); //For this test, assume module is already at our desired baud rate
  while(!NanoSerial); //Wait for port to open

  //About 200ms from power on the module will send its firmware version at 115200. We need to ignore this.
  while(NanoSerial.available()) NanoSerial.read();

  nano.getVersion();

  if (nano.msg[0] == ERROR_WRONG_OPCODE_RESPONSE)
  {
    //This happens if the baud rate is correct but the module is doing a ccontinuous read
    nano.stopReading();

    Serial.println(F("Module continuously reading. Asking it to stop..."));

    delay(1500);
  }

  else if (nano.msg[0] != ALL_GOOD)
  {
    //The module did not respond so assume it's just been powered on and communicating at 115200bps
    NanoSerial.begin(115200); //Start software serial at 115200

    nano.setBaud(baudRate); //Tell the module to go to the chosen baud rate. Ignore the response msg

    NanoSerial.begin(baudRate); //Start the serial port, this time at user's chosen baud rate

    //Test the connection
    nano.getVersion();
    if (nano.msg[0] != ALL_GOOD) return (false); //Something is not right
  }

  //The M6E has these settings no matter what
  nano.setTagProtocol(); //Set protocol to GEN2

  nano.setAntennaPort(); //Set TX/RX antenna ports to 1

  return (true); //We are ready to rock
}
