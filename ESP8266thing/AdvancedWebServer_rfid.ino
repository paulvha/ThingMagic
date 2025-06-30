/*  Copyright (c) November 2018, Paul van Haastrecht
 *  
 *  version 1.0
 *  
 *  This sketch will connect the ESP8266 thing (https://www.sparkfun.com/products/13231 )to the local network as a server and constant tries to 
 *  read RFID tags with a Sparkfun/nano/simultaneousreader ( https://www.sparkfun.com/products/14066 ) as well as check for client connection. 
 *  The detected unique EPC's are stored in an array on the board and can be accessed with the following commands:
 *  
 *  get_cnt        : return the number of unique EPC's in the array
 *  get_epc        : returns the unique EPC numbers
 *  clr_epc        : will remove the EPC's from the array after the next get_epc command
 *  FORCED_clr_epc : will clear all the EPC's from the array immediately 
 *    
 *  Optional if you had also connected a DS18x20 temperature sensor :
 *  temp_c : will return temperature in celsius
 *  temp_f : will return temperature in Fahrenheit
 *  
 *  ///////////////////////////////////////////////////////////////
 *  Hardware connections
 *  
 *  RFID reader         ESP8266 thing
 *  GND ------------------GND
 *  UART RX ------------- 13
 *  UART TX ------------- 12
 *  
 *  The reader was external powered in my case. if you want to use the 5V from the ESP8266
 *  connect a wire between UART VCC ------ 5V ESP8266
 *  
 *  Set the switch to HW UART
 *  
 *  
 *  Optional Temperature sensor ( DS18x20 family)
 *  
 *  DS18x20           ESP8266 thing
 *  GND   ------------- GND
 *  5V /red -----------  5V
 *  data/yellow -------- 4
 *  
 *  You MUST connect a resistor of 4K7 between data and +5v
 *  
 *  ////////////////////////////////////////////////////////////////
 *  
 *  From line 103 there are parameters that MUST of CAN be changed in order to configure the program
 *  
 *  !!!!!!!!!!!!!! NO support  !!!!!!!!!!!!!!!!!!!!
 *  
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 *  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 *  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 *  ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 *  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 *  ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *  
 *  This sketch is based on the AdvancedWebservers sketch. 
 *  The original copyright information is included below
 */
 
/*
   Copyright (c) 2015, Majenko Technologies
   All rights reserved.

   Redistribution and use in source and binary forms, with or without modification,
   are permitted provided that the following conditions are met:

 * * Redistributions of source code must retain the above copyright notice, this
     list of conditions and the following disclaimer.

 * * Redistributions in binary form must reproduce the above copyright notice, this
     list of conditions and the following disclaimer in the documentation and/or
     other materials provided with the distribution.

 * * Neither the name of Majenko Technologies nor the names of its
     contributors may be used to endorse or promote products derived from
     this software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
   ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
   WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
   DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
   ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
   (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
   LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
   ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <OneWire.h>                  // needed for the temperature sensor
#include <SoftwareSerial.h>           // Used for transmitting to the reader
#include <SparkFun_UHF_RFID_Reader.h> // Library for controlling the M6E Nano module

  /////////////////////////////////////////////////
  //            WiFi Network Definitions        //
  ////////////////////////////////////////////////

  /* MUST: Replace these two character strings with the name & password of your WiFi network.*/
const char mySSID[] = "yourSSIDhere";
const char myPSK[] = "yourPWDhere";

  /* MUST: ESP8266Server port definition 
   * port 80 must be used for browser */
#define WIFI_PORT 80

  /* name on local network instead of connecting with IP address one can also
   * connect:  http://rfid.local  */
const char* MDNS_NAME = "rfid";

  /////////////////////////////////////////////////
  //            RFID shield Definitions          //
  /////////////////////////////////////////////////

  /* MUST: you must define the pins to use */
#define RFID_SoftSerial_TX 12
#define RFID_SoftSerial_RX 13
  
  /* MUST: define the serial speed to communicate with the shield 
   * DO NOT SET THIS HIGHER WHEN USING SOFTSERIAL */
#define RFID_SERIAL_SPEED 38400

 /* MUST: Define the region in the world you are in
  * It defines the frequency to use that is available in your part of the world
  * 
  * Valid options are :
  * REGION_INDIA, REGION_JAPAN, REGION_CHINA, REGION_EUROPE, REGION_KOREA,
  * REGION_AUSTRALIA,  REGION_NEWZEALAND, REGION_NORTHAMERICA
  */
#define RFID_Region REGION_EUROPE

  /* MUST: define the Readpower between 0 and 27dBm. if only using the
   * USB power, do not go above 5dbm (=500) */
#define RFID_Read_Power 500

  /////////////////////////////////////////////////
  //            DS18B20  Definitions             //
  /////////////////////////////////////////////////
 /* MUST: to which pin is the data wire of the DS18b20 connected
  * 0 means NO temperature sensor */
#define TEMP_PIN 4

  /////////////////////////////////////////////////
  //           Program Definitions               //
  /////////////////////////////////////////////////

  /* MUST: define the maximum table size for discovered / unique EPC */
#define EPC_COUNT 50        // Max number of unique EPC's
#define EPC_ENTRY 12        // max EPC bytes

 /*OPTIONAL : display debug level/information 
  * 0 = no debug information
  * 1 = tag info found
  * 2 = 1 + programflow + temperature
  * 3 = 2 + Nano debug 
  */
#define PRMDEBUG 0

 // size of HTML buffer 
#define HTML_BUFFER 400

///////////////////////////////////////////////////////////////////////
//       NO CHANGES NEEDED BEYOND THIS POINT                         //
///////////////////////////////////////////////////////////////////////

// Create instances
// RFID reader connection on the ESP8266 thing
SoftwareSerial softSerial(RFID_SoftSerial_RX, RFID_SoftSerial_TX); //RX, TX
RFID nano;
 
ESP8266WebServer server(WIFI_PORT);
OneWire  ds(TEMP_PIN);  // on pin 10 (a 4.7K -10K resistor is necessary between VCC and data)

// USED TO STORE DATA TO BE SEND
char temp[HTML_BUFFER];

// create array to hold the discovered EPC's
uint8_t EPC_recv[EPC_COUNT][EPC_ENTRY];

// general variable for tmp count/return values
int val1; 

// clr_epc must be called before and then the send EPC will be removed from list
bool enable_clr = false;

// store temperature type and address
byte type_s;
byte addr[8];

void setup(void) {
   
  Serial.begin(115200);

  // initialize RFID shield
  initialize_RFID();

  // initialize array
  init_array();
  
  // if temperature sensor pin, try detect temperature sensor
  if (TEMP_PIN > 0)  init_tmp();
      
  // initialize different WIFI components
  init_wifi();
  
  Serial.print(F("Array initialized\nconnect to the ipaddress: "));
  Serial.print(WiFi.localIP());
  Serial.print(F(" OR\n connect as http://"));
  Serial.print(MDNS_NAME);
  Serial.println(".local\n");
}

void loop(void) {

  Check_EPC();

  server.handleClient();
}


////////////////////////////////////////////////////////////
//           Temperature sensor routines                   //
////////////////////////////////////////////////////////////

/* initialize the temperature sensor */
void init_tmp(void) {
  
  Serial.print(F("Try to detect temperature sensor. "));
  
  type_s = 0xf;     // indicate no sensor
  
  ds.reset_search();
  
  if ( !ds.search(addr)) {
    Serial.println(F("No temperature sensor detected."));
    return;
  }

  if (OneWire::crc8(addr, 7) != addr[7]) {
    Serial.println(F(" CRC is not valid!"));
    return;
  }
  
  if(PRMDEBUG > 1)
  {
    Serial.print(F("ROM ="));
    for(byte i = 0; i < 8; i++) {
      Serial.write(' ');
      Serial.print(addr[i], HEX);
    }
  }
 
  // the first ROM byte indicates which chip
  switch (addr[0]) {
    case 0x10:
      Serial.println(F("  Chip = DS18S20"));  // or old DS1820
      type_s = 1;
      break;
    case 0x28:
      Serial.println(F("  Chip = DS18B20"));
      type_s = 0;
      break;
    case 0x22:
      Serial.println(F("  Chip = DS1822"));
      type_s = 0;
      break;
    default:
      Serial.println(F("Device is not a DS18x20 family device."));
      return;
  } 
}

/* if DS18x20 sensor detected try to read it 
 * temperature : 1 = return celsius, 0 = return fahrenheit
 */
float read_temp(bool temperature)
{
  byte i;
  byte present = 0;
  byte data[12];
  float celsius, fahrenheit;
  
  ds.reset();
  ds.select(addr);
  ds.write(0x44, 1);     // start conversion, with parasite power on at the end
  
  delay(1000);           // maybe 750ms is enough, maybe not
  // we might do a ds.depower() here, but the reset will take care of it.
  
  present = ds.reset();
  ds.select(addr);    
  ds.write(0xBE);       // Read Scratchpad

  if(PRMDEBUG > 1)
  {
    Serial.print("Data = ");
    Serial.print(present, HEX);
    Serial.print(" ");
    for ( i = 0; i < 9; i++) {           // we need 9 bytes
      data[i] = ds.read();
      Serial.print(data[i], HEX);
      Serial.print(" ");
    }
    
    Serial.print(" CRC=");
    Serial.print(OneWire::crc8(data, 8), HEX);
    Serial.println();
  }
  
  // Convert the data to actual temperature
  // because the result is a 16 bit signed integer, it should
  // be stored to an "int16_t" type, which is always 16 bits
  // even when compiled on a 32 bit processor.
  int16_t raw = (data[1] << 8) | data[0];
  if (type_s) {
    raw = raw << 3; // 9 bit resolution default
    if (data[7] == 0x10) {
      // "count remain" gives full 12 bit resolution
      raw = (raw & 0xFFF0) + 12 - data[6];
    }
  } else {
    byte cfg = (data[4] & 0x60);
    // at lower res, the low bits are undefined, so let's zero them
    if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
    else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
    else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
    //// default is 12 bit resolution, 750 ms conversion time
  }
  celsius = (float)raw / 16.0;
  fahrenheit = celsius * 1.8 + 32.0;

  if(PRMDEBUG > 1)
  {
    Serial.print(F("Temperature = "));
    Serial.print(celsius);
    Serial.print(F(" Celsius, "));
    Serial.print(fahrenheit);
    Serial.println(F(" Fahrenheit"));
  }

  if (temperature)  return(celsius);
  else return(fahrenheit);
}

////////////////////////////////////////////////////////////
//                   WIFI routine                         //
////////////////////////////////////////////////////////////

/* initialize the different WIFI settings */
void init_wifi()
{
  Serial.println(F("try to connect to WIFI network"));
  WiFi.mode(WIFI_STA);
  WiFi.begin(mySSID, myPSK);

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.print("Connected to ");
  Serial.println(mySSID);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (MDNS.begin(MDNS_NAME)) {
    Serial.println("MDNS responder started");
  }

  // set subroutines to call based on the extension
  server.on("/", handleRoot);
  server.on("/get_cnt", count_epc);
  server.on("/get_epc", get_epc);
  server.on("/clr_epc", clr_epc);
  server.on("/FORCED_clr_epc", forced_clr_epc);
  server.on("/tmp_c", get_temp_c);
  server.on("/tmp_f", get_temp_f);
  
  server.on("/inline", []() {       // left for demo only
    server.send(200, "text/plain", "this works as well");
    });
  server.onNotFound(handleNotFound);
  
  server.begin();
  
  Serial.println("HTTP server started");
}

////////////////////////////////////////////////////////////
//                   RFID routines                        //
////////////////////////////////////////////////////////////

void initialize_RFID()
{
retry:
  Serial.println(F("try to connect to RFID reader"));
  
  //Configure nano to run at certain speed
  int RunStatus = setupNano(RFID_SERIAL_SPEED);
  
  if (RunStatus == 0) 
  {
    serialTrigger(F("Module failed to respond. Please check wiring."));
    yield();
    goto retry;
  }
  
  if (RunStatus  == 1) // was not yet in continues mode
  {
    nano.setRegion(RFID_Region); //Set to correct region

    // The M6E has this settings no matter what
    nano.setTagProtocol(); //Set protocol to GEN2

    nano.setAntennaPort(); //Set TX/RX antenna ports to 1
    
    nano.setReadPower(RFID_Read_Power); //5.00 dBm. Higher values may caues USB port to brown out
    //Max Read TX Power is 27.00 dBm and may cause temperature-limit throttling

    nano.startReading(); //Begin scanning for tags
  }
  
  Serial.println("RFID shield initialized");
}
 
//Gracefully handles a reader that is already configured and already reading continuously
int setupNano(long baudRate)
{
  if (PRMDEBUG == 3) nano.enableDebugging(softSerial);

  nano.begin(softSerial); //Tell the library to communicate over software serial port
  
  //Test to see if we are already connected to a module
  //This would be the case if the Arduino has been reprogrammed and the module has stayed powered
  softSerial.begin(baudRate); //For this test, assume module is already at our desired baud rate

  while(!softSerial)      yield(); //Wait for port to open

  //About 200ms from power on the module will send its firmware version at 115200. We need to ignore this.
  while(softSerial.available()) softSerial.read();

  val1 = 0;

  while(val1 < 2)
  {
    nano.getVersion();
    
    if (nano.msg[0] == ERROR_WRONG_OPCODE_RESPONSE )
    {
      if (PRMDEBUG > 1) Serial.println(F("Module continuously reading. Not Asking it to stop..."));
      return(2);
    }
  
    else if (nano.msg[0] != ALL_GOOD) 
    {
      if (PRMDEBUG > 1) Serial.println(F("Try reset RFID speed"));
    
      //The module did not respond so assume it's just been powered on and communicating at 115200bps
      softSerial.begin(115200); //Start software serial at 115200

      nano.setBaud(baudRate); //Tell the module to go to the chosen baud rate. Ignore the response msg

      softSerial.begin(baudRate); //Start the software serial port, this time at user's chosen baud rate
    }

    else
    {
      return(1);    // Responded, but have to be set in the right mode
    }
   
    val1++;
  }

  return(0);
}

/* try to read EPC from tag and add to array */
void Check_EPC()
{
  uint8_t myEPC[EPC_ENTRY];  // Most EPCs are 12 bytes
  
  if (nano.check() == true)  // Check to see if any new data has come in from module
  {
    byte responseType = nano.parseResponse(); //Break response into tag ID, RSSI, frequency, and timestamp

    if (responseType == RESPONSE_IS_KEEPALIVE)
    {
      if (PRMDEBUG > 1) Serial.println(F("Scanning"));
    }
    
    else if (responseType == RESPONSE_IS_TAGFOUND)
    {
      // extract EPC
      for (byte x = 0 ; x < EPC_ENTRY ; x++)
      {
        myEPC[x] = nano.msg[31 + x];
      }

      // add to array (if not already there)
      add_ECP_entry(myEPC, EPC_ENTRY);

      // display for debug information
      if (PRMDEBUG > 0)
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

        //Print EPC bytes, 
        Serial.print(F(" epc["));
        for (byte x = 0 ; x < EPC_ENTRY ; x++)
        {
          if (myEPC[x] < 0x10) Serial.print(F("0")); //Pretty print adding zero
          Serial.print(myEPC[x], HEX);
          Serial.print(F(" "));
        }
        Serial.println("]");
      }
    }
    else
    {
      //Unknown response
      if (PRMDEBUG > 1)nano.printMessageArray(); //Print the response message. Look up errors in tmr__status_8h.html
    }
  }
}

////////////////////////////////////////////////////////////
//                   ARRAY routines                        //
////////////////////////////////////////////////////////////

/* initialize/empty the array to capture detected EPC */
void init_array()
{
  for (val1 = 0 ; val1 < EPC_COUNT; val1++)  EPC_recv[val1][0] = 0;
}

/* add entry to array (if not there already) */
void add_ECP_entry(uint8_t *msg, byte mlength)
{
  byte j;
  val1=0;
  bool found;
  
  // as long as not end of list or end of array
  while(val1 < EPC_COUNT && EPC_recv[val1][0] != 0)
  {
    found = true;

    // check each entry for a match
    for (j = 0 ; j < mlength, j < EPC_ENTRY ; j++)
    {
      if (EPC_recv[val1][j] != msg[j]) 
      {
        found = false;  // indicate not fuond
        j = EPC_ENTRY;
      }
    }
   
    // if found
    if (found == true)
    {
      if (PRMDEBUG > 1)
      {
        Serial.print(F("FOUND EPC in array entry "));
        Serial.println(val1);
      }
      return;
    }
    
    val1++;  // next entry
  }
  
  // Not found, can we still add?
  if (val1 == EPC_COUNT)
  {
    if (PRMDEBUG > 0) Serial.println(F("Can not add more to array"));
    return;
  }
  
  // debug info
  if (PRMDEBUG > 1)
  {
     Serial.print(F("New entry number is "));
     Serial.println(val1);
  }
  
  // add entry to the array
  for (j = 0 ; j < mlength, j < EPC_ENTRY ; j++)   EPC_recv[val1][j] = msg[j];
}

/* count number of entries with unique EPC received */
int count_entries()
{
  val1 = 0;
  
  while(val1 < EPC_COUNT)
  {
    if( EPC_recv[val1][0] == 0) break;
    val1++;
  }

  return(val1);
}


///////////////////////////////////////////////////////////////////////////////
///                           command routines                               //
///////////////////////////////////////////////////////////////////////////////

/* This will be called if extension is /
 * like http://192.168.1.7/
 */
void handleRoot() {
  String  header = F("<html>\r\n  <head>\r\n"
                        "<title>RFID server</title> \r\n<style>\r\n"
                        "body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }\r\n"
                        "</style>\r\n  </head>\r\n  <body>\r\n"
                        "<h1>Welcome to the RFID server!</h1>\r\n"
                        "\r\n<p> valid commands : <br>"
                        "get_cnt&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;: returns the number of unique EPC's in the array<br>"
                        "get_epc&emsp;&emsp;&emsp;&emsp;&emsp;&ensp;&nbsp;: returns the unique EPC numbers<br>"
                        "clr_epc&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;: will enable (or disable)remove the EPC's from the array after the next get_epc command<br>"
                        "FORCED_clr_epc&emsp;: will clear all the EPC's from the array immediately <br> ");
  String temp_dct= F("tmp_f&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&ensp;&nbsp;: returns temperature in Fahrenheit<br>"
                        "tmp_c&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&ensp;: returns temperature in Celsius<br>");
                        
  String hdr_close= F("</p>\r\n</body>\r\n</html>\r\n");

  // only add if temperature sensor was detected
  if (type_s != 0xf) header += temp_dct;

  // close the header
  header += hdr_close;
  
  // send it to client
  server.send(200, "text/html", header);
}
/* This will be called if extension is unknown/ not registered
 * like http://192.168.1.7/bad
 */
void handleNotFound() {
  
  String message = F("Command Not valid\n\nURI: ");
  message += server.uri();
  message += "\nArguments: ";
  message += server.args();
  message += "\n";

  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }

  server.send(404, "text/plain", message);
}

/* This will be called if extension is /clr_epc
 * like http://192.168.1.7/clr_epc
 *
 * enable to clear the list of unique EPC with next get_epc call */
void clr_epc()
{
   String header = F("<html>\r\n  <head>\r\n"
                     "<title>Enable clear EPC</title> \r\n<style>\r\n"
                     "body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }\r\n"
                     "</style>\r\n  </head>\r\n  <body>\r\n");

   const String trailer1 = F("<h1>EPC list clear has been ENABLED.<br>Use get_epc and the displayed EPC's will be removed from list </h1>\r\n  </body>\r\n</html>\r\n");
   const String trailer2 = F("<h1>EPC list clear has been disabled. </h1>\r\n  </body>\r\n</html>\r\n");
  
  if (enable_clr == false)
  {
    // enable clear of entry with next get_epc call
    enable_clr = true;

    header += trailer1;
  }
  else
  {
    // if it was set, disable it
    enable_clr = false;

    header += trailer2;
  }
    
  // send to client
  server.send(200, "text/html", header);
}

/* This will be called if extension is /FORCED_clr_epc
 * like http://192.168.1.7/FORCED_clr_epc
 *   
 *  perform a cold/forced clear of the EPC list
 *  no checks, just clear
 */
void forced_clr_epc()
{
  String header = F("<html>\r\n  <head>\r\n"
                     "<title>Forced clear EPC</title> \r\n<style>\r\n"
                     "body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }\r\n"
                     "</style>\r\n  </head>\r\n  <body>\r\n"
                     "<h1>EPC list has been forced cleared</h1>\r\n</body>\r\n</html>\r\n");
  init_array();

  // send to client
  server.send(200, "text/html", header);
}

/* This will be called if extension is /get_cnt
 * like http://192.168.1.7/get_cnt
 *
 * return the count of unique EPC
 * The screen will refresh each 15 seconds
 */
void count_epc()
{
  String header = F("<html>\r\n  <head>\r\n"
                     "<title>Unique EPC count</title> \r\n<style>\r\n"
                     "body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }\r\n"
                     "</style>\r\n  </head>\r\n  <body>\r\n"
                     "<h1>counter_epc: ");

  snprintf(temp, 400, "%s %02d</h1>\r\n</body>\r\n</html>\r\n", header.c_str(), count_entries());

  // set to client
  server.send(200, "text/html", temp);
}

/* This will be called if extension is /get_epc
 * like http://192.168.1.7/get_epc
 *
 * return the unique EPC's */
void get_epc()
{
    String header = F("<html>\r\n  <head>\r\n"
                     "<title>Unique EPC overview</title> \r\n<style>\r\n"
                     "body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }\r\n"
                     "</style>\r\n  </head>\r\n  <body>\r\n"
                     "<h1>Detected EPC's</h1>     <p>");
    int j,i = 0;
    bool sent_comma = false;

    // loop through the complete list
    while ( i < EPC_COUNT )
    {
       // as long as there is a valid entry
       if ( EPC_recv[i][0] != 0)
       {
          // add comma in between EPC's
          if (sent_comma) header += ",";
          else sent_comma = true;
          
          // add epc
          temp[0] = 0x0;
          for (j = 0 ; j < EPC_ENTRY; j++)
          {
            sprintf(temp, "%s %02x",temp, EPC_recv[i][j]);
          }

          header += temp;
          
          // if clr_epc was called before, it will clear entry after sending EPC's
          if (enable_clr) EPC_recv[i][0]= 0;
       }
      
      // next EPC
      i++;
    }

  // add trailer  
  if (enable_clr)
    header += "\n</p>\n<h1> These EPC's have now been removed. </h1>\n</body>\n</html>\n";
  else
    header += "\n</p>\n</body>\n</html>\n";
  
  if (PRMDEBUG == 3)
  {
    Serial.print("length");
    Serial.println(strlen(header.c_str()));
    Serial.println(header);
  }
  
  // set to client
  server.send(200, "text/html", header);
  
  // reset / disable clear EPC entries for next call
  enable_clr = false;
}

/* obtain and send the temperature
 *  temperature : 1 = celsius, 0 = Fahrenheit
 */
void get_temp(bool temperature)
{
  float temp1;

  String header = F("<html>\r\n  <head>\r\n"
                     "<title>Get temperature</title> \r\n<style>\r\n"
                     "body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }\r\n"
                     "</style>\r\n  </head>\r\n  <body>\r\n <h1>Temperature: ");
 
  if (type_s == 0xf)
  {
    snprintf(temp, 400, "%s <h1>No temperature sensor connected</h1>\r\n</body>\r\n</html>\r\n", header.c_str());
  }
  else
  {
    // get temperature
    temp1 = read_temp(temperature);
    
    if (temperature)
      snprintf(temp, 400, "%s%02.2f *C</h1>\r\n</body>\r\n</html>\r\n", header.c_str(), temp1);
    else
      snprintf(temp, 400, "%s%02.2f *F</h1>\r\n</body>\r\n</html>\r\n", header.c_str(), temp1);
  }
  
  // set to client
  server.send(200, "text/html", temp);
}

/* This will be called if extension is /temp_f
 * like http://192.168.1.7/tmp_f
 *
 * return the temperature in Fahrenheit */
void get_temp_f()
{
  get_temp(0);
}

/* This will be called if extension is /temp_c
 * like http://192.168.1.7/tmp_c
 *
 * return the temperature in celsius */
void get_temp_c()
{
  get_temp(1);
}

///////////////////////////////////////////////////////////////////////////////
///                           Supporting routines                            //
///////////////////////////////////////////////////////////////////////////////
 
/* serialTrigger prints a message, then waits for something
 * to come in from the serial port. */
void serialTrigger(String message)
{
  Serial.println();
  Serial.println(message);
  Serial.println();
  while (!Serial.available());
  while (Serial.available())Serial.read();
}
