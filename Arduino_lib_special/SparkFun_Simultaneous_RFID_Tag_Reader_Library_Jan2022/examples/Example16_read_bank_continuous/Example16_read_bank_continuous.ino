/*
  Version 1.0 / September 2020 / paulvha
  This example will get the data from a memory bank while Nano operating in continuous mode. You can select the
  memory bank, the starting WORD address and the length in WORDS of the data.  This sketch will also read
  and display the temperature of the Nano.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 POSSIBILITY OF SUCH DAMAGE.

 ===================================================================================

 NO support, delivered as is, have fun, good luck !!

************************************************************************************
Original heading orginal examples as part the code is coming from SparkFun

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

// define the serial port to use (E.g. softSerial, Serial1, Serial2 etc)
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
 * in your area. Valid options :
 *
 * REGION_INDIA        REGION_JAPAN     REGION_CHINA
 * REGION_KOREA        REGION_AUSTRALIA REGION_NEWZEALAND
 * REGION_NORTHAMERICA REGION_EUROPE    REGION_OPEN
*////////////////////////////////////////////////////////////
#define NANOREGION REGION_EUROPE

/////////////////////////////////////////////////////////////////
/* Define the bank reading options :
 *
 * BANK :  select the memory bank to read
 *  TMR_GEN2_BANK_RESERVED
 *  TMR_GEN2_BANK_EPC
 *  TMR_GEN2_BANK_TID
 *  TMR_GEN2_BANK_USER
 *
 * STARTWORDADDRESS : select at which WORD address to start in the memory bank.
 *    Select a zero to start at the beginning
 *    e.g. selecting 1 will be skip first 2 bytes ( 1 word = 2 bytes)
 *
 * WORDLENGTH : select the number of words to read
 *    0 = read whole bank
 *
 *  WORDLENGTH can NOT be more than 32 words (= 64 bytes). (datasheet page 58)
 *  TID, RESERVE or EPC memory will normally fit so KEEP THIS SET TO ZERO.
 *  ZERO means read the whole bank.
 *  When selecting USER Bank the MAXIMUM returned is 32 words.
 *
 *  For ANY memory bank, you can select 32 WORDS or LESS BUT the
 *  STARTWORDADDRESS + WORDLENGTH needs to be EQUAL or LESS than the
 *  total WORDS left to read in the memory bank.
 *
 *  If your memory bank can read less than 32 words, as it is smaller or
 *  due to STARTWORDADDRESS offset, and you request 32 words you will NOT
 *  receive any words !!  You have to request less words.
 *
 *  STARTWORDADDRESS can be used to select the area of the memory bank
 *  you want to read, with WORDLENGTH you can then select the amount
 *  words ( = 2 bytes) you want to read.
*////////////////////////////////////////////////////////////////
#define BANK TMR_GEN2_BANK_TID
#define STARTWORDADDRESS 0
#define WORDLENGTH 0

//***************************************************
//       NO CHANGES NEEDED BEYOND THIS POINT        *
//***************************************************

RFID nano; //Create instance

void setup()
{
  Serial.begin(115200);
  while (!Serial); //Wait for the serial port to come online

  Serial.println(F("Example16: read a memory bank in continuous mode"));

  if (DEBUG) nano.enableDebugging(Serial);

  if (! setupNano(38400)) //Configure nano to run at 38400bps
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

  /* /reader/tagReadData/uniqueByData is automatically set in startReadingBank().
   * This allows tags with the same EPC ID but different values in the specified
   * Gen2.ReadData memory location to be treated as unique tags during inventories. */

  nano.startReadingBank(BANK, STARTWORDADDRESS, WORDLENGTH); //Begin scanning for tags
}

void loop()
{
  static uint8_t t_save = 0;

  if (nano.check()) //Check to see if any new data has come in from module
  {
    byte responseType = nano.parseResponse(); //check for correct package

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

      uint8_t databytes = nano.getTagDataBytes(); // get the number of data bytes.

      Serial.print(F("rssi["));
      Serial.print(rssi);
      Serial.print(F("]"));

      Serial.print(F(" freq["));
      Serial.print(freq);
      Serial.print(F("]"));

      Serial.print(F(" time["));    // since last keep-alive
      Serial.print(timeStamp);
      Serial.print(F("]"));

      //Print EPC bytes, this is a subsection of bytes from the response/msg array
      Serial.print(F(" epc ["));
      for (byte x = 0 ; x < tagEPCBytes ; x++)
      {
        if (nano.msg[31 + x + databytes ] < 0x10) Serial.print(F("0")); //Pretty print
        Serial.print(nano.msg[31 + x + databytes], HEX);
        Serial.print(F(" "));
      }
      Serial.println(F("]"));

      if (databytes> 0) {

        //Print data bytes
        if (BANK == TMR_GEN2_BANK_RESERVED) Serial.print(F("Reserve Bank ("));
        else if (BANK == TMR_GEN2_BANK_TID) Serial.print(F("TID Bank ("));
        else if (BANK == TMR_GEN2_BANK_USER) Serial.print(F("User Bank ("));
        else if (BANK == TMR_GEN2_BANK_EPC) Serial.print(F("EPC Bank ("));
        else Serial.print(F("data ("));

        Serial.print(databytes);
        Serial.print(F(")["));

        uint8_t MemData[databytes];
        uint8_t MemLen = nano.getTagData(MemData, databytes);

        for (byte x = 0 ; x < MemLen ; x++)
        {
          if (MemData[x] < 0x10) Serial.print(F("0")); //Pretty print
          Serial.print(MemData[x], HEX);
          Serial.print(F(" "));
        }
         Serial.print(F("]"));

      }
      else {
        Serial.print(F("No Data"));
      }

      Serial.println("\n");
    }
    else if (responseType == ERROR_CORRUPT_RESPONSE)
    {
      Serial.println(F("Bad CRC"));
    }
    else if (responseType == RESPONSE_IS_TEMPERATURE) // should not happen as check()returned false
    {
      Serial.println(F("Temperature reading"));
    }
    else
    {
      //Unknown response
      Serial.println(F("Unknown error"));
    }
  }

  // read temperature
  int8_t t = nano.getTemp();

  if (t > -1 ) {

    // Only update display if temperature has changed
    if (t_save != t) {
      Serial.print(F("Nano internal temperature "));
      Serial.print(t);
      Serial.println(F(" (*C)"));
      t_save = t;
    }
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
