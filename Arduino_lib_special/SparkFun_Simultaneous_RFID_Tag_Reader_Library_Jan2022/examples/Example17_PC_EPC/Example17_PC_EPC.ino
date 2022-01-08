/*

  February  2021 / paulvha
  Single shot read - Read EPC + Protocol Code Word (PC), able to WRITE PC + optional EPC

  WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING
  Updating a PC can have a big impact on the ability to use a TAG. Only do this if you
  really know what you are doing. If the tag gets useable.. it is your responsibility !
  WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING

  Based on original examples :
  Reading multiple RFID tags, simultaneously!
  By: Nathan Seidle @ SparkFun Electronics
  Date: October 3rd, 2016
  https://github.com/sparkfun/Simultaneous_RFID_Tag_Reader

  If using the Simultaneous RFID Tag Reader (SRTR) shield, make sure the serial slide
  switch is in the 'SW-UART' position.
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

  Serial.println(F("Example17: read (and potential write) Protocol Control Word + EPC"));

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

  Serial.println(F("Press a key to scan for a tag to start reading PC and EPC"));
  while (!Serial.available());               // Wait for user to send a character
  while (Serial.available()) {delay(10);Serial.read();}  // Throw away the user's characters

  byte myEPC[14]; //Most EPCs are 12 bytes + PC word = 14
  byte myEPClength;
  byte responseType = 0;

  do
  {
    myEPClength = sizeof(myEPC); //Length of EPC is modified each time .readTagPCW is called

    // read EPC + PC (length MUST be 14 characters)
    responseType = nano.readTagPCW(myEPC, myEPClength, 500); //Scan for a new tag up to 500ms

    Serial.println(F("Searching for tag"));
    delay(500);

  } while (responseType != RESPONSE_SUCCESS);

  //Print PC
  Decode_PC(myEPC);

  //Print EPC
  Serial.print(F("EPC["));
  for (byte x = 2 ; x < myEPClength ; x++)
  {
    if (myEPC[x] < 0x10) Serial.print(F("0"));
    Serial.print(myEPC[x], HEX);
    Serial.print(F(" "));
  }
  Serial.println(F("]"));

  // UNCOMMENT THIS SECTION AND APPLY THE CHANGE YOU WANT TO WRITE TO THE PC
/*
  Serial.println (F("Updating a PC can have a big impact on the ability to use a TAG."));
  Serial.println (F("ONLY CONTINUE if you really know what you are doing. !!"));
  Serial.println (F("You have to make uncomment this part, apply changes before compile"));
  Serial.println (F("If the tag gets UN-useable.. it is your own responsibility !!!\n"));

  // Set NEW PC bytes.. Make sure you understand how a PC works
  if (myEPC[0] == 0x34) {  // if OLD PC
    myEPC[0] = 0x7D;       // set new PC
    myEPC[1] = 0x01;
    myEPClength = 2;       // only write PC
  }

  // RESTORE ORIGINAL PC
  else {
    myEPC[0] = 0x34;
    myEPC[1] = 0x00;
    myEPClength = 2;
  }

  // WRITE PC (optional + EPC) Length MUST be at least 2 (the PC)
  responseType = nano.writeTagPCW(myEPC, myEPClength);

  if (responseType == RESPONSE_SUCCESS)
    Serial.println("New PC Written!");
  else
    Serial.println("Failed write");
*/
}

//
// Display Protocol code word and details
//
void Decode_PC(uint8_t *myEPC)
{
  Serial.print(F("Protocol Word (PC) ["));
  for (byte x = 0 ; x < 2 ; x++)
  {
    if (myEPC[x] < 0x10) Serial.print(F("0"));
    Serial.print(myEPC[x], HEX);
    Serial.print(F(" "));
  }
  Serial.println(F("]"));

  uint8_t EPC_len = myEPC[0] >> 3;
  Serial.print(F("\tLength (PC + EPC) "));
  Serial.print(EPC_len);
  Serial.print(F(" words or "));
  Serial.print(EPC_len * 2);
  Serial.println(F(" bytes\r"));

  if (myEPC[0] & 0x04)  Serial.println(F("\tUser Memory Indicator (UMI) is set\r"));
  else Serial.println(F("\tUser Memory Indicator (UMI) is NOT set\r"));

  if (myEPC[0] & 0x02)  Serial.println(F("\tExtended Protocol control word indicator is set\r"));
  else Serial.println(F("\tExtended Protocol control word indicator is NOT set\r"));

  if (myEPC[0] & 0x01) {
    Serial.println(F("\tNumbering System Indicator (ISO flag) is set\r"));
    Serial.print(F("\tAFI is "));
    Serial.print(myEPC[1], HEX);
    Serial.println("\r\n");
  }
  else
    Serial.println(F("\tNumbering System Indicator is NOT set (GEN2 EPC)\r\n"));
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
