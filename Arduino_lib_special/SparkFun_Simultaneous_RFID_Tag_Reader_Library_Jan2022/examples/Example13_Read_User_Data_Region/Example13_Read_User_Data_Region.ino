/*
  Reading multiple RFID tags, simultaneously!
  By: Nathan Seidle @ SparkFun Electronics
  Date: October 3rd, 2016
  https://github.com/sparkfun/Simultaneous_RFID_Tag_Reader

  Read the user writeable data from a detected tag

  Version 1.0 / May 2020 / paulvha
  This is same as example4, but you can now select specific area to read only.
  The region start address is in words, length of area to read is in words. A word is 2 bytes.

  Version 1.0.1 / February 2021 adding bank selection
*/

#include "SparkFun_UHF_RFID_Reader.h" //Library for controlling the M6E Nano module

// if you plan to use softserial:  initalize that here, else comment it out the 2 lines
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

  Serial.println(F("Example13: read user data region"));

  if (DEBUG) nano.enableDebugging(Serial);

  if (setupNano(38400) == false) //Configure nano to run at 38400bps
  {
    Serial.println(F("Module failed to respond. Please check wiring."));
    while (1); //Freeze!
  }

  nano.setRegion(NANOREGION); //Set to the right region

  nano.setReadPower(500); //5.00 dBm. Higher values may cause USB port to brown out
  //Max Read TX Power is 27.00 dBm and may cause temperature-limit throttling
}

void loop()
{
  Serial.println(F("Press a key to read user data"));
  while (!Serial.available()); //Wait for user to send a character
  Serial.read(); //Throw away the user's character

  //Read the data from the tag
  byte responseType;
  byte myData[64];
  byte myDataLength = sizeof(myData);  //Tell readUserData to read up to 64 bytes

  uint32_t WordStartAdress = 0x1;     // skip byte 0 and 1, start at byte 2
  uint8_t RegionLength = 0x10;        // number of words to read 0x10 = 16 words = 32 bytes

  responseType = nano.readDataRegion(TMR_GEN2_BANK_USER, WordStartAdress, RegionLength, myData, myDataLength);

  if (responseType == RESPONSE_SUCCESS)
  {
    //Print User Data
    Serial.print(F("Size ["));
    Serial.print(myDataLength);
    Serial.print(F("] User data["));
    for (byte x = 0 ; x < myDataLength ; x++)
    {
      if (myData[x] < 0x10) Serial.print(F("0"));
      Serial.print(myData[x], HEX);
      Serial.print(F(" "));
    }
    Serial.println(F("]"));
  }
  else
    Serial.println(F("Error reading tag data"));

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
