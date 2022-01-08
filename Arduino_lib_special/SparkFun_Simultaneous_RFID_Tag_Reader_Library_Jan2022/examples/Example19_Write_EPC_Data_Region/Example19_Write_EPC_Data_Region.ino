/*
  Reading multiple RFID tags, simultaneously!
  By: Nathan Seidle @ SparkFun Electronics
  Date: October 3rd, 2016
  https://github.com/sparkfun/Simultaneous_RFID_Tag_Reader

  Write new data to the user data area
  Some tags have 64, 16 4, or 0 bytes of user data available for writing.

  If you write more bytes than is available (10 bytes and only 4 available) module will simply timeout.

  EPC is good for things like UPC (this is a gallon of milk)
  User data is a good place to write things like the milk's best by date

  Version 1.0 /February 2021 / paulvha
  You can write the data starting from a specific address to the EPC bank
  instead of always position zero.
*/

#include "SparkFun_UHF_RFID_Reader.h" //Library for controlling the M6E Nano module

// if you plan to use softserial:  initalize that here else comment it out the 2 lines
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

//***************************************************
//       NO CHANGES NEEDED BEYOND THIS POINT        *
//***************************************************

RFID nano; //Create instance

void setup()
{
  Serial.begin(115200);
  while (!Serial); //Wait for the serial port to come online

  Serial.println(F("Example19: write EPC data region"));

  if (DEBUG) nano.enableDebugging(Serial);

  if (setupNano(38400) == false) //Configure nano to run at 38400bps
  {
    Serial.println(F("Module failed to respond. Please check wiring."));
    while (1); //Freeze!
  }

  nano.setRegion(NANOREGION); //Set to the right region

  nano.setReadPower(500); //5.00 dBm. Higher values may caues USB port to brown out
  //Max Read TX Power is 27.00 dBm and may cause temperature-limit throttling

  nano.setWritePower(600); //6.00 dBm. Higher values may caues USB port to brown out
  //Max Write TX Power is 27.00 dBm and may cause temperature-limit throttling
}

void loop()
{
  Serial.println(F("\nGet all tags out of the area. Press a key to write DATA to first detected tag."));
  Serial.println(F("MAKE SURE YOU KNOW WHAT YOU DO. thE TAG COULD BECOME UN-READABLE. NO WARRANTY !!!"));
  while (!Serial.available()); //Wait for user to send a character
  while (Serial.available()) {delay(10);Serial.read();}  // Throw away the user's characters

  //"Hello" is recorded as "Hell". You can only write even number of bytes
  uint8_t testData[2] = {0x31, 0x32}; //adjust this array with data and size to the your needs

  uint32_t WordStartAdress = 5;  // = 5 words = starting byte 10

  byte responseType = nano.writeDataRegion(TMR_GEN2_BANK_EPC, WordStartAdress, testData, sizeof(testData));

  if (responseType == RESPONSE_SUCCESS)
    Serial.println(F("New Data Written!"));
  else
  {
    Serial.println(F("\nFailed write!"));
    Serial.println(F("Did you write more data than the tag has memory?"));
    Serial.println(F("Is the tag locked?"));
  }
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
