/*
  Demonstration of extension to the Sparkfun RFID library enabling GPIO usage on the
  Simultaneous RFID Tag Reader from Sparkfun based on the M6e Nano!

  By: paulvha@hotmail.com
  Date: August 11, 2019
  https://github.com/paulvha/ThingMagic/gpio

  Parts of the code are based on :

  By: Nathan Seidle @ SparkFun Electronics
  Date: October 3rd, 2016
  https://github.com/sparkfun/Simultaneous_RFID_Tag_Reader

  If using the Simultaneous RFID Tag Reader (SRTR) shield with softserial directly
  connected on top of the micro, make sure the serial slide switch is in the 'SW-UART' position.

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.
 */

#include <SoftwareSerial.h> //Used for transmitting to the device
SoftwareSerial softSerial(2, 3); //RX, TX

// set NanoSerial to the serial port to use (e.g. SoftSerial, Serial2 or Serial3 depending on your choice)
#define NanoSerial softSerial

#include "SparkFun_UHF_RFID_Reader.h" //Library for controlling the M6E Nano module
RFID nano; //Create instance

void setup()
{
  Serial.begin(115200);

  while (!Serial);
  Serial.println();
  Serial.println(F("Initializing..."));

  if (setupNano(38400) == false) //Configure nano to run at 38400bps
  {
    Serial.println(F("Module failed to respond. Please check wiring."));
    while (1); //Freeze!
  }

  nano.setRegion(REGION_NORTHAMERICA); //Set to North America

  nano.setReadPower(500); //5.00 dBm. Higher values may cause USB port to brown out
  //Max Read TX Power is 27.00 dBm and may cause temperature-limit throttling

  //nano.enableDebugging(Serial);
}

/**
 *  there are a number of demo's in the loop. you can select one or have them run all
 *
 */
void loop()
{
  /* read direction of all GPIO, change to output read direction again */
  demo1();

  /*
   * Demonstrate setting of direction and levels output
   *
   * connect a wire between LV2 and LV3
   */
  demo2();

  /*
   * blink a led
   *
   * Blink parameters can be defined just before demo3() code below
   */
  demo3();

  //remove comments to only run once
  //while (1); //Freeze!

  // delay between the demo loop
  delay(2000);
}


/*
 * Display the direction and current level of the GPIO
 */
void disp_status(uint8_t gpio)
{
  bool state;

  if (nano.getGPIODirection(gpio, &state) == ALL_GOOD) {
      Serial.print(F("GPIO Direction: "));

      if (state) Serial.print(F("output"));
      else Serial.print(F("input"));

      if (nano.getGPIO(gpio, &state) == ALL_GOOD) {
        Serial.print(", level ");
        if(state) Serial.println(F("HIGH"));
        else Serial.println(F("LOW"));
      }
      else {
        Serial.println(F("Error during reading level"));
        while (1); //Freeze!
      }
    }
    else {
      Serial.println(F("Error during reading direction"));
      while (1); //Freeze!
    }
}


/* read direction of all GPIO, change to output read direction again */

void demo1()
{
  bool state;
  int i;

  Serial.println(F("Read the current direction state and level of the GPIO's"));
  for (i = GPI01 ; i <= LV4; i++) {
    Serial.print(F("GPIO "));
    Serial.print(i);
    Serial.print(F(":  "));
    disp_status(i);
  }

  Serial.println(F("\nSet the direction state of the GPIO's to out with LOW output"));
  for (i = GPI01 ; i <= LV4; i++){

    if (nano.setGPIODirection(i, GPIOout) == ALL_GOOD){
      Serial.print(F("GPIO "));
      Serial.print(i);
      Serial.println(F(" set as output and LOW"));
    }
    else {
      Serial.println(F("Error during setting direction"));
      while (1); //Freeze!
    }
  }

  Serial.println(F("\nCheck the direction state of the GPIO's"));
  for (i = GPI01 ; i <= LV4; i++){
    Serial.print(F("GPIO "));
    Serial.print(i);
    Serial.print(F(": "));
    disp_status(i);
  }

  Serial.println(F("\nSet the direction state of the GPIO's to out with HIGH output"));
  for (i = GPI01 ; i <= LV4; i++){

    if (nano.setGPIODirection(i, GPIOout, HIGH) == ALL_GOOD){
      Serial.print(F("GPIO "));
      Serial.print(i);
      Serial.println(F(" set as output and HIGH"));
    }
    else {
      Serial.println(F("Error during setting direction"));
      while (1); //Freeze!
    }
  }

  Serial.println(F("\nCheck the direction state of the GPIO's"));
  for (i = GPI01 ; i <= LV4; i++){
    Serial.print(F("GPIO "));
    Serial.print(i);
    Serial.print(F(":  "));
    disp_status(i);
  }

  Serial.println(F("\nSet the GPIO's to output LOW"));
  for (i = GPI01 ; i <= LV4; i++){

    if (nano.setGPIO(i,LOW) == ALL_GOOD){
      Serial.print(F("GPIO "));
      Serial.print(i);
      Serial.println(F(" set to LOW"));
    }
    else {
      Serial.println(F("Error during setting level"));
      while (1); //Freeze!
    }
  }

  Serial.println(F("\nCheck the direction state of the GPIO's"));
  for (i = GPI01 ; i <= LV4; i++){
    Serial.print(F("GPIO "));
    Serial.print(i);
    Serial.print(F(":  "));
    disp_status(i);
  }

  Serial.println(F("\nSet the GPIO's to output HIGH"));
  for (i = GPI01 ; i <= LV4; i++){

    if (nano.setGPIO(i,HIGH) == ALL_GOOD){
      Serial.print(F("GPIO "));
      Serial.print(i);
      Serial.println(" set to HIGH");
    }
    else {
      Serial.println(F("Error during setting level"));
      while (1); //Freeze!
    }
  }

  Serial.println(F("\nCheck the direction state of the GPIO's"));
  for (i = GPI01 ; i <= LV4; i++){
    Serial.print(F("GPIO "));
    Serial.print(i);
    Serial.print(F(":  "));
    disp_status(i);
  }
  Serial.println(F("\nSet the direction state of the GPIO's to input"));
  for (i = GPI01 ; i <= LV4; i++){

    if (nano.setGPIODirection(i, GPIOin) == ALL_GOOD){
      Serial.print(F("GPIO "));
      Serial.print(i);
      Serial.println(F(" set as input"));
    }
    else {
      Serial.println(F("Error during setting direction"));
      while (1); //Freeze!
    }
  }

  Serial.println(F("\nCheck the direction state of the GPIO's"));
  for (i = GPI01 ; i <= LV4; i++){
    Serial.print(F("GPIO "));
    Serial.print(i);
    Serial.print(F(":  "));
    disp_status(i);
  }

  Serial.println();

  Serial.println(F("End of demo1\n"));
}

/*
 * Demostrate setting of direction and levels output
 *
 * connect a wire between LV2 and LV3
 */
void demo2()
{
  bool state;

  Serial.println(F("\n********** get starting status **************"));
  Serial.print(F("\ngpio LV2 direction and level "));
  disp_status(LV2);

  Serial.print(F("gpio LV3 direction and level "));
  disp_status(LV3);

  Serial.println(F("\n********** change direction and level LV2 **************"));
  Serial.println(F("\nset direction gpio LV2 as output and HIGH"));
  if (nano.setGPIODirection(LV2, GPIOout, HIGH) != ALL_GOOD) {
     Serial.println(F("Error during setting LV2"));
     while (1); //Freeze!
  }

  Serial.println(F("set direction gpio LV3 as input"));
  if (nano.setGPIODirection(LV3, GPIOin) != ALL_GOOD) {
     Serial.println(F("Error during setting LV3"));
     while (1); //Freeze!
  }

  Serial.println(F("\n********** check for change applied **************"));
  Serial.print(F("\ngpio LV2 level"));
  disp_status(LV2);

  Serial.print(F("gpio LV3 level"));
  disp_status(LV3);

  Serial.println(F("The levels between LV2 and LV3 should BOTH be HIGH as wire should be connected"));

  Serial.println(F("\n********** set LV2 low and check  **************"));
  Serial.println(F("\nSet level gpio LV2 as LOW "));
  if (nano.setGPIO(LV2, LOW)!= ALL_GOOD) {
     Serial.println(F("Error during setting LV1"));
     while (1); //Freeze!
  }

  Serial.print(F("\ngpio LV2 level"));
  disp_status(LV2);

  Serial.print(F("gpio LV3 level"));
  disp_status(LV3);
  Serial.println(F("The levels between LV2 and LV3 should BOTH be LOW as wire should be connected"));

  Serial.println(F("\n********** set LV2 as input **************"));
  Serial.println(F("\nset direction gpio LV2 in"));
  if (nano.setGPIODirection(LV2, GPIOin) != ALL_GOOD) {
     Serial.println(F("Error during setting LV2"));
     while (1); //Freeze!
  }
  Serial.println(F("\n**********  check direction and levels **************"));
  Serial.print(F("\ngpio LV2 direction and level"));
  disp_status(LV2);

  Serial.print(F("gpio LV3 direction and level"));
  disp_status(LV3);

  Serial.println(F("End of demo2\n"));
}


/* blink on a gpio of the Nano */
#define BLINKGPIO LV2       // GPIO to use
#define BLINKNUM 10         // repeat count
#define BLINKDELAY 4000     // time ON and OFF

void demo3()
{
  uint8_t i = 0;

  /* set as output , starting LOW level (default) */
  if (nano.setGPIODirection(BLINKGPIO, GPIOout) == ALL_GOOD){
    Serial.print(F("GPIO "));
    Serial.print(BLINKGPIO);
    Serial.println(F(" set as output"));
  }
  else {
    Serial.println(F("Error during setting direction"));
    while (1); //Freeze!
  }

  while(i++ < BLINKNUM) {

    if (nano.setGPIO(BLINKGPIO,HIGH) == ALL_GOOD){
      Serial.println(F("GPIO set to HIGH"));
    }
    else {
      Serial.println(F("Error during setting level"));
      while (1); //Freeze!
    }
    //disp_status(BLINKGPIO);
    delay(BLINKDELAY);

    if (nano.setGPIO(BLINKGPIO, LOW) == ALL_GOOD){
      Serial.println(F("GPIO set to LOW"));
    }
    else {
      Serial.println(F("Error during setting level"));
      while (1); //Freeze!
    }
    //disp_status(BLINKGPIO);
    delay(BLINKDELAY);
  }

  Serial.println(F("Blink Demo ended\n"));
}


//Gracefully handles a reader that is already configured and already reading continuously
//Because Stream does not have a .begin() we have to do this outside the library
boolean setupNano(long baudRate)
{
  nano.begin(NanoSerial); //Tell the library to communicate over software serial port

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
  else
  {
    //The module did not respond so assume it's just been powered on and communicating at 115200bps
    NanoSerial.begin(115200); //Start software serial at 115200

    nano.setBaud(baudRate); //Tell the module to go to the chosen baud rate. Ignore the response msg

    NanoSerial.begin(baudRate); //Start the software serial port, this time at user's chosen baud rate
  }

  //Test the connection
  nano.getVersion();
  if (nano.msg[0] != ALL_GOOD) return (false); //Something is not right

  //The M6E has these settings no matter what
  nano.setTagProtocol(); //Set protocol to GEN2

  nano.setAntennaPort(); //Set TX/RX antenna ports to 1

  return (true); //We are ready to rock
}
