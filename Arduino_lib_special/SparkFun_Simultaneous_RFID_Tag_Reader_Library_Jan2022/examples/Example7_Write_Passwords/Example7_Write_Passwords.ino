/*
  Reading multiple RFID tags, simultaneously!
  By: Nathan Seidle @ SparkFun Electronics
  Date: October 3rd, 2016
  https://github.com/sparkfun/Simultaneous_RFID_Tag_Reader

  Obviously, be careful because this premanently and irrevocably destroys a tag.

  This shows how to send the right command (with password) to disable a tag.

  Arduino pin 2 to Nano TX
  Arduino pin 3 to Nano RX
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

  Serial.println(F("Example7: write passwords"));

  if (DEBUG) nano.enableDebugging(Serial);

  if(setupNano(38400) == false) //Configure nano to run at 38400bps
  {
    Serial.println(F("Module failed to respond. Please check wiring."));
    while(1); //Freeze!
  }

  nano.setRegion(NANOREGION); //Set to North America

  nano.setReadPower(500); //5.00 dBm. Higher values may cause USB port to brown out
  //Max Read TX Power is 27.00 dBm and may cause temperature-limit throttling

  nano.setWritePower(500); //5.00 dBm. Higher values may cause USB port to brown out
  //Max Write TX Power is 27.00 dBm and may cause temperature-limit throttling

}

void loop()
{
  Serial.println(F("Get all tags out of the area. Press a key to write PWs to first detected tag."));
  while (!Serial.available()); //Wait for user to send a character
  Serial.read(); //Throw away the user's character

  byte myKillPW[] = {0xEE, 0xFF, 0x11, 0x22};
  byte response = nano.writeKillPW(myKillPW, sizeof(myKillPW));

  if (response == RESPONSE_SUCCESS)
    Serial.println("New Kill PW Written!");
  else
    Serial.println("Failed write");

  byte myAccessPW[] = {0x12, 0x34, 0x56, 0x78};
  response = nano.writeAccessPW(myAccessPW, sizeof(myAccessPW));

  if (response == RESPONSE_SUCCESS)
    Serial.println("New Access PW Written!");
  else
    Serial.println("Failed write");
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

