/*
  Reading multiple RFID tags, simultaneously!
  By: Nathan Seidle @ SparkFun Electronics
  Date: October 3rd, 2016
  https://github.com/sparkfun/Simultaneous_RFID_Tag_Reader

  Constantly reads and outputs any tags heard

  If using the Simultaneous RFID Tag Reader (SRTR) shield, make sure the serial slide
  switch is in the 'SW-UART' position

  PAULVHA / February 2020
  THIS IS A SPECIAL EXAMPLE1 AS IT DEMONSTRATES THE USAGE OF POWERMODE

  It will loop to scan and when a good EPC has been detected, it will set maximum power saving, sleep for 2 seconds
  and go back to full powermode.
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

// set Nano speed
#define NANOSPEED 38400

// enable use maximum saving mode by setting to 1
// setting to 0 , this example will work as the default example 1
#define SLEEP 1

//***************************************************
//       NO CHANGES NEEDED BEYOND THIS POINT        *
//***************************************************

RFID nano; //Create instance

void setup()
{
  Serial.begin(115200);
  while (!Serial); //Wait for the serial port to come online

  Serial.println(F("Example10: constant read powermode"));

  if (DEBUG) nano.enableDebugging(Serial);

  if (setupNano(NANOSPEED) == false) //Configure nano to run at NANOSPEED
  {
    Serial.println(F("Module failed to respond. Please check wiring."));
    while (1); //Freeze!
  }

  nano.setRegion(NANOREGION); //Set to the right region

  nano.setReadPower(500); //5.00 dBm. Higher values may caues USB port to brown out
  //Max Read TX Power is 27.00 dBm and may cause temperature-limit throttling

  Serial.println(F("Press a key to begin scanning for tags."));
  while (!Serial.available()); //Wait for user to send a character
  Serial.read(); //Throw away the user's character

  nano.startReading(); //Begin scanning for tags
}

void loop()
{
  static bool Sleep_set = false;    // remember that maximum saving was set

  if (Sleep_set) {
    NanoWakeUp();
    Serial.println("Wake-up");
    Sleep_set = false;
  }

  if (nano.check() == true) //Check to see if any new data has come in from module
  {
    byte responseType = nano.parseResponse(); //Break response into tag ID, RSSI, frequency, and timestamp

    if (responseType == RESPONSE_IS_KEEPALIVE)
    {
      Serial.println(F("Scanning"));
    }
    else if (responseType == RESPONSE_IS_TAGFOUND)
    {
      //If we have a full record we can pull out the fun bits
      int rssi = nano.getTagRSSI(); //Get the RSSI for this tag read

      long freq = nano.getTagFreq(); //Get the frequency this tag was detected at

      long timeStamp = nano.getTagTimestamp(); //Get the time this was read, (ms) since last keep-alive message

      byte tagEPCBytes = nano.getTagEPCBytes(); //Get the number of bytes of EPC from response

      Serial.print(F(" rssi["));
      Serial.print(rssi);
      Serial.print(F("]"));

      Serial.print(F(" freq["));
      Serial.print(freq);
      Serial.print(F("]"));

      Serial.print(F(" time["));
      Serial.print(timeStamp);
      Serial.print(F("]"));

      //Print EPC bytes, this is a subsection of bytes from the response/msg array
      Serial.print(F(" epc["));
      for (byte x = 0 ; x < tagEPCBytes ; x++)
      {
        if (nano.msg[31 + x] < 0x10) Serial.print(F("0")); //Pretty print
        Serial.print(nano.msg[31 + x], HEX);
        Serial.print(F(" "));
      }
      Serial.print(F("]"));

      Serial.println();

      // demonstrate maximum saving mode (if requested)
      if (SLEEP) {

        NanoSleep();
        Sleep_set = true;
        Serial.println("Set to Sleep");
        delay(2000);  // cause a delay just for test
      }
    }
    else if (responseType == ERROR_CORRUPT_RESPONSE)
    {
      Serial.println("Bad CRC");
    }
    else
    {
      //Unknown response
      Serial.print("Unknown error");
    }
  }
}

/*
 * Set to maximum saving mode
 *
 * This can be observer by enabledebugging and a positive response :
 * sendCommand: [FF] [01] [98] [03] [44] [BE]
 * response:    [FF] [00] [98] [00] [00]
 *
 */
void NanoSleep()
{
    nano.stopReading();           // Stop scanning for tags

    while (nano.check());         // Try to flush as much as possible pending input
    while (NanoSerial.available()) NanoSerial.read();
    delay(500);
    while (NanoSerial.available()) NanoSerial.read();
    while (nano.check());

    while (!nano.setPowerMode(3)); // set Maximum Saving mode
}

/*
 * Wakeup from maximum saving mode
 *
 * Maximum Saving Mode only supports communications at 9600 baud
 *
 * The succesfull wake-up can be observed with enable debug
 * sendCommand: [FF] [01] [98] [00] [44] [BD]
 * response:  [FF] [00] [98] [00] [00]
 */
void NanoWakeUp()
{
  NanoSerial.begin(9600);       // Start serial to 9600 baud

  while (! nano.setPowerMode(0)) {
    delay(500);                 // give time to wakeup
    //Serial.println("retry wake up");
    NanoSerial.begin(NANOSPEED); // Start the serial port, this time at user's chosen baud rate
  }

  nano.startReading();          // Begin scanning for tags
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
