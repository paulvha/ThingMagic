/*********************************************************************************
 * Copyright (c) December 2018, version 1.0     Paul van Haastrecht
 *
 *  =========================  highlevel Description =================================
 *
 *  This sketch will connect the ESP32 thing (https://www.sparkfun.com/products/13907 ) to the local network as a server and constant tries to
 *  read RFID tags with a Sparkfun/nano/simultaneousreader ( https://www.sparkfun.com/products/14066 ) as well as check for client connection.
 *  The detected unique EPC's are stored in an array on the board and can be accessed with the following commands:
 *
 *  epc_cnt        : return the number of unique EPC's detected
 *  epc_num        : returns the unique EPC numbers comma seperated(EPC NUMBER,)
 *  epc_dct        : same as epc_num, but will add to each EPC how often detected (EPC NUMBER:##,)
 *  epc_all        : get_cnt + epc_dct commands
 *  epc_clr        : will remove the EPC's from the array after the next epc_num command
 *  force_epc_clr  : will clear all the EPC's from the array immediately
 *
 *  Optional if you had also connected a DS18x20 temperature sensor :
 *  tmp_c          : returns temperature in celsius
 *  tmp_f          : returns temperature in Fahrenheit
 *
 *  command looks like : http://192.168.1.137/epc_cnt or http://rfid.local/epc_cnt
 *
 *  =========================  Hardware connections =================================
 *
 *  ===  RFID connection
 *
 *  Set the onboard switch to HW-UART and connect with lose wires.
 *
 *  RFID reader     ESP32 thing (Serial2 is used)
 *  GND -----------------GND
 *  UART TX ---- LS------ 16      (LS = level shifter !!)
 *  UART RX ------------- 17
 *
 *  The reader is external powered in my case.
 *
 *  !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 *  WARNING!! WARNING !!WARNING!! WARNING !!WARNING!! WARNING !! WARNING!! WARNING !!
 *  !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 *
 *  The NANO M6E needs at least 3.7V VCC. It will not work stable on 3.3V.
 *
 *  If you connect to the 5V (USB-V) you MUST use a level shifter for the for RX line from RFID reader to the ESP32.
 *  The pins on the ESP-32 can only handle 3V3 signals !!
 *
 *  The RFID RX line, TX / pin 17 from ESP32, actually does not need a level shifter. The 3V3 that is coming from the ESP32 works good for the Nano
 *  HOWEVER for the TX FROM the Nano, did not work well with that level shifter (https://www.sparkfun.com/products/12009)
 *  Apparently the level shifter on the Nano and this level shifter do not work well together. So I made it working with resistors:
 *
 *            ________           _______
 *  GND ------| 10K  |-----!---- | 5k6 |------  TX from Nano
 *            --------     !     -------
 *                         !
 *                         pin 16 (ESP32)
 *
 *
 *  ====  Optional temperature sensor (DS18x20 family)
 *
 *             DS18x20           ESP32 thing
 *   ------      GND   ------------- GND
 * |-| 4k7 |--   VCC /red ---------  3V3
 * | ------
 * |-----------  data/yellow ------- 4
 *
 *  You MUST connect a resistor of 4K7 between data and VCC for pull-up.
 *
 *  !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 *  WARNING!! WARNING !!WARNING!! WARNING !!WARNING!! WARNING !! WARNING!! WARNING !!
 *  !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 *  The DS18B20 works with VCC between 3 and 5V. It works fine on 3.3V however if connected to 5V you MUST include a level shifter or making
 *  a bridge with resistors like below
 *
 *            -------            -------
 *  GND ------| 10K  |-----!---- | 5k6 |------  data/yellow from DS18x20
 *            --------     !     -------
 *                         !
 *                         pin 4 (ESP32)
 *
 *
 *  ================================= PARMAMETERS =====================================
 *
 *  From line 179 there are parameters that MUST of CAN be changed in order to configure the program
 *
 *  ================================== SOFTWARE ========================================
 *
 *  The sketch is depending on the following libraries and they must have been installed before
 *
 *  for Array handling     https://github.com/janelia-arduino/Vector.git
 *  for RFID NANO          https://github.com/sparkfun/SparkFun_Simultaneous_RFID_Tag_Reader_Library
 *  for Temperature sensor https://github.com/PaulStoffregen/OneWire
 *
 *
 *  Code for the DS18x20 is based on the DS18x20 example in the onewire library
 *  Code for the RFID is based on example the Sparkfun library
 *  Code for the WIFI is based on the ESP32 example AdvancedWebservers sketch. The original copyright information is included below
 *
 *  Make sure :
 *  - To select the Sparkfun ESP32 thing board before compiling
 *  - The serial monitor is NOT active (will cause upload errors)
 *  - Press GPIO 0 switch during connecting after compile to start upload to the board
 *
 *  NO support, delivered as is, have fun, good luck !!
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

#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>

#include <OneWire.h>                  // needed for the temperature sensor
#include <SparkFun_UHF_RFID_Reader.h> // Library for controlling the M6E Nano module
#include <Vector.h>                   // storage handling

// The ESP32 wants to know the subroutines up front (normally in header file)

// ==== remote command handling routines
void handleRoot();
void epc_cnt();
void epc_num();
void epc_dct();
void epc_all();
void epc_clr();
void force_epc_clr();
void get_temp_c();
void get_temp_f();
void handleNotFound();

// ==== supporting routines
void serialTrigger(String message);

// ==== DS18x20 routines
void init_tmp(void);
float read_Temperature(bool temperature);

// ==== array handling routines
void Array_Init();
int Array_Cnt();
void Array_Clr();
void Array_Add(uint8_t *msg, byte mlength);
void Array_Rmv_Entry(int i);

// ==== Wifi routines
void Init_Wifi();

// ===== RFID routines
void Init_RFID();
int setupNano(long baudRate);
void Check_EPC();

  /////////////////////////////////////////////////
  //            WiFi Network Definitions        //
  ////////////////////////////////////////////////

  /* MUST: Replace these two character strings with the name & password of your WiFi network.*/
const char mySSID[] = "SSID";
const char myPSK[] = "PASSWORD";

  /* Option: ESP8266Server port definition
   * port 80 must be used for browser */
#define WIFI_PORT 80

  /* Option: name on local network instead of connecting with IP address one can also
   * connect:  http://rfid.local  */
const char* MDNS_NAME = "rfid";

  /////////////////////////////////////////////////
  //            RFID shield Definitions          //
  /////////////////////////////////////////////////

  /* Option: define the serial speed to communicate with the shield */
#define RFID_SERIAL_SPEED 38400

 /* MUST: Define the region in the world you are in
  * It defines the frequency to use that is available in your part of the world
  *
  * Valid options are :
  * REGION_INDIA, REGION_JAPAN, REGION_CHINA, REGION_EUROPE, REGION_KOREA,
  * REGION_AUSTRALIA, REGION_NEWZEALAND, REGION_NORTHAMERICA
  */
#define RFID_Region REGION_EUROPE

  /* Option: define the Readpower between 0 and 27dBm. if only using the
   * USB power, do not go above 5dbm (=500) */
#define RFID_POWER 500

  /////////////////////////////////////////////////
  //            DS18x20  Definitions             //
  /////////////////////////////////////////////////

 /* MUST: to which pin is the data wire of the DS18x20 connected
  * 0 means NO temperature sensor */
#define TEMP_PIN 4          // see remark in hardware section in begin sketch

  /////////////////////////////////////////////////
  //           Program Definitions               //
  /////////////////////////////////////////////////

  /* Option: speed of serial /console connection */
#define SERIAL 115200

  /* MUST: define the maximum table size for discovered / unique EPC */
#define EPC_COUNT 50        // Max number of unique EPC's
#define EPC_ENTRY 12        // max bytes in EPC

 /* Option: display debug level/information
  * 0 = no debug information
  * 1 = tag info found
  * 2 = 1 + programflow + temperature info
  * 3 = 2 + Nano debug  */
#define PRMDEBUG 0

 /* Option: size of HTML buffer */
#define HTML_BUFFER 400

//////////////////////////////////////////////////////////////
// ======  NO CHANGES NEEDED BEYOND THIS POINT ============ //
//////////////////////////////////////////////////////////////

// Create instances
RFID nano;
WebServer server(80);
OneWire ds(TEMP_PIN);

// create structure & place to hold the discovered EPC's
typedef struct Epcrecv {
    uint8_t epc[EPC_ENTRY];         // save EPC
    uint8_t cnt;                    // how often detected
} Epcrecv;

Epcrecv epc_space[EPC_COUNT];       // allocate the space
Vector <Epcrecv> epcs;              // create vector

// use to store data to be send
char HtmlBuf[HTML_BUFFER];

// adding how often the same EPC was detected (epc_dct call)
bool detect_cnt = false;

// show the total count (epc_cnt) + Add how often detected the same EPC (epc_dct call)
bool get_all = false;

// clr_epc must be called followed by epc_num / epc_dct / epc_all call remove from list after display
bool enable_clr = false;

// store temperature DS18x20 type and address
byte type_s;
byte addr[8];


void setup(void) {

  Serial.begin(SERIAL);

  Init_RFID();        // initialize RFID shield

  init_tmp();         // try detect temperature sensor

  Array_Init();       // initialize array

  Init_Wifi();        // initialize different WIFI components

  Serial.print(F("Array initialized\nNow connect to the ipaddress: "));
  Serial.print(WiFi.localIP());
  Serial.print(F(" OR\n connect as http://"));
  Serial.print(MDNS_NAME);
  Serial.println(".local\n");
}

void loop(void) {

  Check_EPC();              // check for EPC

  server.handleClient();    // check and handle client command
}

////////////////////////////////////////////////////////////
//           temperature sensor routines                  //
////////////////////////////////////////////////////////////

/* initialize the temperature sensor */
void init_tmp(void) {

  type_s = 0xf;            // indicate no sensor

  if (TEMP_PIN == 0) return;

  Serial.print(F("Try to detect temperature sensor. "));

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
    Serial.print(F("ROM = "));
    for(byte i = 0; i < 8; i++) {Serial.write(' '); Serial.print(addr[i], HEX);}
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
 * temperature : 1 = return celsius, 0 = return fahrenheit */
float read_Temperature(bool temperature)
{
  byte i;
  byte present = 0;
  byte data[12];
  float celsius, fahrenheit;

  ds.reset();
  ds.select(addr);
  ds.write(0x44, 1);          // start conversion, with parasite power on at the end

  delay(1000);                // maybe 750ms is enough, maybe not

  present = ds.reset();
  ds.select(addr);
  ds.write(0xBE);             // Read Scratchpad

  for ( i = 0; i < 9; i++) {  // we need 9 bytes
    data[i] = ds.read();
  }

  // Convert the data to actual temperature because the result is a 16 bit signed integer, it should
  // be stored to an "int16_t" type, which is always 16 bits even when compiled on a 32 bit processor.
  int16_t raw = (data[1] << 8) | data[0];
  if (type_s) {
    raw = raw << 3;            // 9 bit resolution default
    if (data[7] == 0x10) {
      // "count remain" gives full 12 bit resolution
      raw = (raw & 0xFFF0) + 12 - data[6];
    }
  } else {
    byte cfg = (data[4] & 0x60);
    // at lower res, the low bits are undefined, so let's zero them
    if (cfg == 0x00) raw = raw & ~7;      // 9 bit resolution, 93.75 ms
    else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
    else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
    //// default is 12 bit resolution, 750 ms conversion time
  }
  celsius = (float)raw / 16.0;
  fahrenheit = celsius * 1.8 + 32.0;

  if(PRMDEBUG > 1)
  {
    Serial.print(F("temperature = "));
    Serial.print(celsius);
    Serial.print(F(" Celsius, "));
    Serial.print(fahrenheit);
    Serial.println(F(" Fahrenheit"));
  }

  if (temperature)  return(celsius);
  else return(fahrenheit);
}

////////////////////////////////////////////////////////////
//                   ARRAY routines                       //
////////////////////////////////////////////////////////////

/* initialize the array to capture detected EPC */
void Array_Init()
{
  epcs.setStorage(epc_space);
}

/* count number of entries with unique EPC received */
int Array_Cnt()
{
  return((int) epcs.size());
}

/* Empty the EPC array */
void Array_Clr()
{
  epcs.clear();
}

/* Remove a single entry from the array */
void Array_Rmv_Entry(int i)
{
  epcs.remove(i);
}

/* add entry to array (if not there already) */
void Array_Add(uint8_t *msg, byte mlength)
{
    bool found = false;
    byte j;
    size_t i;

    for (i = 0; i < epcs.size(); i++)
    {
        found = true;

        // check each entry for a match
        for (j = 0 ; j < mlength, j < EPC_ENTRY ; j++)
        {
          if (epcs[i].epc[j] != msg[j])
          {
            found = false;  // indicate not found
            break;
          }
        }

        if (found)
        {
          if (PRMDEBUG > 1)
          {
            Serial.print(F("FOUND EPC in array entry "));
            Serial.println(i);
          }
          epcs[i].cnt++;
          return;
        }
    }

  // debug info
  if (PRMDEBUG > 1) Serial.println(F("Not found adding"));

  if ( i >= epcs.max_size())
  {
    Serial.println(F("Can not add more"));
    return;
  }

  // add new entry in array
  Epcrecv newepc;
  for (j = 0 ; j < EPC_ENTRY ; j++) newepc.epc[j] = msg[j];
  newepc.cnt = 0;
  epcs.push_back(newepc);     // append to end
}

////////////////////////////////////////////////////////////
//                   WIFI routine                         //
////////////////////////////////////////////////////////////

/* initialize the different WIFI settings */
void Init_Wifi()
{
  int retry_connect;

retry_wifi:
  retry_connect = 0;
  Serial.println(F("Trying to connect to WIFI network"));
  WiFi.mode(WIFI_STA);
  WiFi.begin(mySSID, myPSK);

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {

    delay(500);

    if(++retry_connect > 50)
    {
      Serial.println(F("Failure to connect. Retry"));
      WiFi.disconnect();
      WiFi.mode(WIFI_OFF);
      goto retry_wifi;
    }

    Serial.print(".");
  }

  Serial.println();
  Serial.print("Connected to ");
  Serial.println(mySSID);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (MDNS.begin(MDNS_NAME))  Serial.println("MDNS responder started");

  // set subroutines to call based on the extension
  server.on("/", handleRoot);
  server.on("/epc_cnt", epc_cnt);
  server.on("/epc_num", epc_num);
  server.on("/epc_clr", epc_clr);
  server.on("/epc_dct", epc_dct);
  server.on("/epc_all", epc_all);
  server.on("/force_epc_clr", force_epc_clr);
  server.on("/tmp_c", get_temp_c);
  server.on("/tmp_f", get_temp_f);

  server.on("/inline", []() {       // left for demo only
    server.send(200, "text/plain", "Why you try ????");
    });
  server.onNotFound(handleNotFound);

  server.begin();

  Serial.println("HTTP server started");
}

////////////////////////////////////////////////////////////
//                   RFID routines                        //
////////////////////////////////////////////////////////////

void Init_RFID()
{

retry:
  Serial.println(F("try to connect to RFID reader"));

  // Configure Nano to run at certain speed
  int RunStatus = setupNano(RFID_SERIAL_SPEED);

  if (RunStatus == 0)
  {
    serialTrigger(F("Module failed to respond. Please check wiring."));
    yield();
    goto retry;
  }

  if (RunStatus  == 1)           // was not yet in continuoos mode
  {
    nano.setRegion(RFID_Region); // Set to correct region
    nano.setTagProtocol();       // Set protocol to GEN2
    nano.setAntennaPort();       // Set TX/RX antenna ports to 1
    nano.setReadPower(RFID_POWER); //5.00 dBm. Higher values may caues USB port to brown out
    //Max Read TX Power is 27.00 dBm and may cause temperature-limit throttling

    nano.startReading();         // Begin scanning for tags
  }

  Serial.println("RFID shield initialized");
}

/* Gracefully handles a reader that is already configured and already reading continuously */
int setupNano(long baudRate)
{
  if (PRMDEBUG == 3) nano.enableDebugging(Serial);

  nano.begin(Serial2); //Tell the library to communicate over software serial port

  //Test to see if we are already connected to a module
  //This would be the case if the Arduino has been reprogrammed/rebooted and the module has stayed powered
  Serial2.begin(baudRate);       // For this test, assume module is already at our desired baud rate

  uint8_t val1 = 0;

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

      // The module did not respond so assume it's just been powered on and communicating at 115200bps
      Serial2.begin(115200);    // Start software serial at 115200

      nano.setBaud(baudRate);   // Tell the module to go to the chosen baud rate.

      Serial2.begin(baudRate);  // Start the software serial port, this time at user's chosen baud rate
      delay(1000);              // the driver is normally not waiting for response from the Nano. So we need to delay and wait
    }
    else
      return(1);                // Responded, but have to be set in the right mode

    val1++;
  }

  return(0);
}

/* try to read EPC from tag and add to array */
void Check_EPC()
{
  uint8_t myEPC[EPC_ENTRY];     // Most EPCs are 12 bytes

  if (nano.check() == true)     // Check to see if any new data has come in from module
  {
    byte responseType = nano.parseResponse(); //Break response into tag ID, RSSI, frequency, and timestamp

    if (responseType == RESPONSE_IS_KEEPALIVE)
    {
      if (PRMDEBUG > 1) Serial.println(F("Scanning"));
    }
    else if (responseType == RESPONSE_IS_TAGFOUND)
    {
      // extract EPC
      for (byte x = 0 ; x < EPC_ENTRY ; x++)   myEPC[x] = nano.msg[31 + x];

      // add to array (if not already there)
      Array_Add(myEPC, EPC_ENTRY);

      // display for debug information
      if (PRMDEBUG > 0)
      {
        int rssi = nano.getTagRSSI();            // Get the RSSI for this tag read
        long freq = nano.getTagFreq();           // Get the frequency this tag was detected at
        long timeStamp = nano.getTagTimestamp(); // Get the time this was read, (ms) since last keep-alive message

        Serial.print(F(" rssi["));
        Serial.print(rssi);
        Serial.print(F("] freq["));

        Serial.print(freq);
        Serial.print(F("] time["));

        Serial.print(timeStamp);
        Serial.print(F("] epc["));

        // Print EPC bytes,
        for (byte x = 0 ; x < EPC_ENTRY ; x++)
        {
          if (myEPC[x] < 0x10) Serial.print(F("0")); // Pretty print adding zero
          Serial.print(myEPC[x], HEX);
          Serial.print(F(" "));
        }
        Serial.println("]");
      }
    }
    else                                             // Unknown response
    {
      if (PRMDEBUG > 1) nano.printMessageArray();    // Print the response message. Look up errors in tmr__status_8h.html
    }
  }
}

///////////////////////////////////////////////////////////////////////////////
///                           command routines                               //
///////////////////////////////////////////////////////////////////////////////

/* This will be called if extension is /
 * like http://192.168.1.7/
 */
void handleRoot() {
  String  header = F("<html>\r\n  <head>\r\n<title>RFID server</title> \r\n<style>\r\n"
                     "body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }\r\n"
                     "</style>\r\n  </head>\r\n  <body>\r\n<h1>Welcome to the RFID server!</h1>\r\n"
                     "\r\n<p> valid commands : <br><br>"
                     "epc_cnt&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;: returns the number of unique EPC's in the detected<br>"
                     "epc_num&emsp;&emsp;&emsp;&emsp;&emsp;&nbsp;: returns the unique EPC numbers<br>"
                     "epc_dct&emsp;&emsp;&emsp;&emsp;&emsp;&ensp;&nbsp;: returns the unique EPC numbers + detection count<br>"
                     "epc_all&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;: perform get_cnt + epc_dct commands<br>"
                     "epc_clr&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;: will enable (or disable) remove the EPC's from the list after the next epc_num or epc_dct command<br>"
                     "force_epc_clr&emsp;&emsp;&emsp;&nbsp;: will remove all the detected EPC's immediately <br> ");

  String HtmlBuf_tmp= F("tmp_f&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&ensp;&nbsp;: returns temperature in Fahrenheit<br>"
                     "tmp_c&emsp;&emsp;&emsp;&emsp;&emsp;&emsp;&ensp;: returns temperature in Celsius<br>");

  String hdr_close= F("<br>Usage example: IP_address/epc_cnt<br></p>\r\n</body>\r\n</html>\r\n");

  // only add if temperature sensor was detected
  if (type_s != 0xf) header += HtmlBuf_tmp;

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

/* This will be called if extension is /epc_clr
 * like http://192.168.1.7/epc_clr
 *
 * enable/disable to clear the list of unique EPC with next epc_num/ epc_dct call */
void epc_clr()
{
   String header = F("<html>\r\n  <head>\r\n<title>Enable clear EPC</title> \r\n<style>\r\n"
                     "body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }\r\n"
                     "</style>\r\n  </head>\r\n  <body>\r\n<h1>EPC list clear has been ");

   const String trailer1 = F("ENABLED.<br>Use epc_num or epc_dct and the displayed EPC's will be removed from list.</h1>\r\n  </body>\r\n</html>\r\n");
   const String trailer2 = F("DISABLED. </h1>\r\n  </body>\r\n</html>\r\n");

  if (enable_clr == false)
  {
    // enable clear of entry with next epc_num call
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

/* This will be called if extension is /force_epc_clr
 * like http://192.168.1.7/force_epc_clr
 *
 *  perform a cold/forced clear of the EPC list
 *  no checks, just clear
 */
void force_epc_clr()
{
  String header = F("<html>\r\n  <head>\r\n<title>Forced clear EPC</title> \r\n<style>\r\n"
                     "body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }\r\n"
                     "</style>\r\n  </head>\r\n  <body>\r\n<h1>EPC list has been forced cleared</h1>\r\n</body>\r\n</html>\r\n");
   Array_Clr();

  // send to client
  server.send(200, "text/html", header);
}

/* This will be called if extension is /epc_cnt
 * like http://192.168.1.7/epc_cnt
 *
 * return the count of unique EPC
 */
void epc_cnt()
{
  String header = F("<html>\r\n  <head>\r\n<title>Unique EPC count</title> \r\n<style>\r\n"
                     "body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }\r\n"
                     "</style>\r\n  </head>\r\n  <body>\r\n<h1>Unique EPC count: ");

  snprintf(HtmlBuf, HTML_BUFFER, "%s %02d</h1>\r\n</body>\r\n</html>\r\n", header.c_str(), Array_Cnt());

  // set to client
  server.send(200, "text/html", HtmlBuf);
}

/* This will be called if extension is /epc_num
 * like http://192.168.1.7/epc_all
 *
 * return the EPC count + unique EPC's + how often detected count */
void epc_all()
{
  get_all= true;
  epc_dct();
  get_all= false;
}

/* This will be called if extension is /epc_num
 * like http://192.168.1.7/epc_dct
 *
 * return the unique EPC's + how often detected count */
void epc_dct()
{
  detect_cnt = true;
  epc_num();
  detect_cnt = false;
}

/* This will be called if extension is /epc_num
 * like http://192.168.1.7/epc_num
 *
 * return the unique EPC's */
void epc_num()
{
  String header = F("<html>\r\n  <head>\r\n<title>Unique EPC overview</title> \r\n<style>\r\n"
                   "body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }\r\n"
                   "</style>\r\n  </head>\r\n  <body>\r\n");

  String epc_cnt = F("<h1>Unique EPC count: ");
  String epc_num = F("<h1>Detected EPC's</h1>     <p>");

  int j, esize, i = 0;
  bool sent_comma = false;
  HtmlBuf[0] = 0x0;

  // if command van epc_all, then add count
  if (get_all) snprintf(HtmlBuf, HTML_BUFFER, "%s%s %02d</h1><br>", header.c_str(), epc_cnt.c_str(), Array_Cnt());

  snprintf(HtmlBuf, HTML_BUFFER, "%s%s", HtmlBuf, epc_num.c_str());

  header += HtmlBuf;
  esize = Array_Cnt();  // get count

  // loop through the complete list
  while ( i < esize ) {

    // add comma in between EPC's
    if (sent_comma) header += ",";
    else sent_comma = true;

    // add epc
    HtmlBuf[0] = 0x0;
    for (j = 0 ; j < EPC_ENTRY; j++)
      sprintf(HtmlBuf, "%s %02x",HtmlBuf, epcs[i].epc[j]);

    // add detection count ?
    if (detect_cnt) sprintf(HtmlBuf, "%s:%02d",HtmlBuf, epcs[i].cnt);

    header += HtmlBuf;

    // if epc_clr was called before, it will clear entry after adding EP
    if (enable_clr){
      Array_Rmv_Entry(i);
      esize--;
      i=0;
    }
    else
      i++;    // next EPC
  }

  // add trailer
  if (enable_clr){
    header += "\n</p>\n<h1>These EPC's have now been removed. </h1>\n</body>\n</html>\n";

    // reset / disable clear EPC entries for next call
    enable_clr = false;
  }
  else
    header += "\n</p>\n</body>\n</html>\n";

  // set to client
  server.send(200, "text/html", header);
}

/* obtain and send the temperature
 * temperature : 1 = celsius, 0 = Fahrenheit
 */
void get_temp(bool Temperature)
{
  float Temperature1;

  String header = F("<html>\r\n  <head>\r\n<title>Get temperature</title> \r\n<style>\r\n"
                     "body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }\r\n"
                     "</style>\r\n  </head>\r\n  <body>\r\n <h1>");

  if (type_s == 0xf)
  {
    snprintf(HtmlBuf, HTML_BUFFER, "%s No Temperature sensor connected.</h1>\r\n</body>\r\n</html>\r\n", header.c_str());
  }
  else
  {
    // get Temperature
    Temperature1 = read_Temperature(Temperature);

    // celcius or Fahrenheit
    if (Temperature)
      snprintf(HtmlBuf, HTML_BUFFER, "%sTemperature: %02.2f *C</h1>\r\n</body>\r\n</html>\r\n", header.c_str(), Temperature1);
    else
      snprintf(HtmlBuf, HTML_BUFFER, "%sTemperature: %02.2f *F</h1>\r\n</body>\r\n</html>\r\n", header.c_str(), Temperature1);
  }

  // set to client
  server.send(200, "text/html", HtmlBuf);
}

/* This will be called if extension is /HtmlBuf_f
 * like http://192.168.1.7/tmp_f
 *
 * return the Temperature in Fahrenheit */
void get_temp_f()
{
  get_temp(0);
}

/* This will be called if extension is /HtmlBuf_c
 * like http://192.168.1.7/tmp_c
 *
 * return the Temperature in celsius */
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
