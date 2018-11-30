/*
  SerialPassthrough sketch

  Some boards, like the Arduino 101, the MKR1000, Zero, or the Micro, have one
  hardware serial port attached to Digital pins 0-1, and a separate USB serial
  port attached to the IDE Serial Monitor. This means that the "serial
  passthrough" which is possible with the Arduino UNO (commonly used to interact
  with devices/shields that require configuration via serial AT commands) will
  not work by default.

  This sketch allows you to emulate the serial passthrough behaviour. Any text
  you type in the IDE Serial monitor will be written out to the serial port on
  Digital pins 0 and 1, and vice-versa.

  On the 101, MKR1000, Zero, and Micro, "Serial" refers to the USB Serial port
  attached to the Serial Monitor, and "Serial1" refers to the hardware serial
  port attached to pins 0 and 1. This sketch will emulate Serial passthrough
  using those two Serial ports on the boards mentioned above, but you can change
  these names to connect any two serial ports on a board that has multiple ports.

  created 23 May 2016
  by Erik Nyquist

  **************************************************************************************
  /// Adjusted for connection with Nano RFID simultanuous reader on softserial.     ////
  /// Parts are also copied from the example sketches from Sparkfun                 ////
  **************************************************************************************
  
  It has been tested against the standard demo.c example from the Mercury API, accept 
  for the addition mentioned in point 2.
  
  !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  This is not a stable connection for a number of reasons :
  !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  
  1. SoftSerial and Nano must be at the same speed. The Nano will always be 
  at 115K after reboot. That is too fast for Softserial. During init we try connect
  with the lower speed. If that fails, we assume a reboot has happened: set
  SoftSerial to 115K and send a lower baudrate commmand and then reset SoftSerial to that 
  lower baudrate. However if the Nano was not either the requested baudrate of 115K nothing
  it will not connect. This can happen if the remote program (e.g. URA) has sent a command 
  to the reader to change baudrate. You need to remove the power from the reader and re-apply.

  2. As the USB port on an Arduino is opened the board will reset, driven by DTR.  More info 
  on https://forum.arduino.cc/index.php?topic=566529.0
  
  That means that the sketch will try to connect again to the Nano, and then loosing the first
  request sent by the remote (e.g. URA). When URA connects it will try to loop through 
  different baudrates to find one where the reader is working on. The changes are very
  high that the remote program (e.g. URA) has switched baudrate before the Nano connection ended.
  Now the Arduino and remote program are out of sync.
  if you build your own program with the Mercury API, To prevent looping. you can try to use 
  the following code BEFORE calling connect():
  
  // set baudrate
  uint32_t baudrate = 19200;                            // MUST be the same as serial speed in sketch
  ret = TMR_paramSet(rp,TMR_PARAM_BAUDRATE,&baudrate);
  
  if(ret != TMR_SUCCESS)
  {
    printf("could not set baudrate\n");
    TMR_destroy(rp);
    exit(0);
  }
  
  3. softSerial on has a default RX buffer of 64 characters. The chances of loosing characters
  due to buffer overrun is high. Keep reading Serial speed faster or equal to the Nano to prevent
  buffer overrun as much as possible

  On failure to connect the on-board led will blink every second.
  
  NOTE 1:
  You can NOT just connect on pin 0 and 1 (the standard serial). Even with an empty sketch the driver 
  on the Arduino will trigger interrupt with each character that arrives from the Nano. 
  
  NOTE 2:
  Always make sure to power the RFID shield with a good power supply. Many communication failures come from that as well


   Delivered as-is. No support, no warranty..
   
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
  

  Paul van Haastrecht November 2018
*/

#include <SoftwareSerial.h>           // Used for transmitting to the reader
#include <SparkFun_UHF_RFID_Reader.h> // Library for controlling the M6E Nano module

/* Tested on a MEGA2560 which has a spare serial port
 * remove comment // before the #define to enable the Serial1 for debug */
//#define SERIAL_DEBUG

/* define the pins to use
 * on a mega tested working with pin  11 and 12. Do NOT use pin 13 as that is the on-board led
 * on an UNO tested working with pin  8 and 9*/
#define RFID_SoftSerial_TX 8
#define RFID_SoftSerial_RX 9

/* define speeds
 * SoftSerial has default a receive buffer of 64 bytes (which is very small).
 * Keep reading Serial speed faster or equal to the Nano to prevent buffer overrun as much as possible
 * Additional one can adjust the buffer size in softserial.h to a larger size*/
#define CON_SPEED_NANO 9600
#define CON_SPEED_SER 19200

////////////////////////////////////////
// No change needed beyond this point //
////////////////////////////////////////

// RFID reader connection 
SoftwareSerial softSerial(RFID_SoftSerial_RX, RFID_SoftSerial_TX); //RX, TX
RFID nano;

void setup() {
  
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_BUILTIN, OUTPUT);

  // set serial speed
  Serial.begin(CON_SPEED_SER);
  
#ifdef SERIAL_DEBUG
  Serial1.begin(CON_SPEED_SER);
  Serial1.println("Connect with Debug Monitor");
#endif

  // try to connect the Nano reader
  if (! setupNano(CON_SPEED_NANO))
  {

#ifdef SERIAL_DEBUG
  Serial1.println("fail");
#endif   
    
    //loop forever and blink led to indicate an error
    for(;;) 
    {
        digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
        delay(1000);                       // wait for a second
        digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
        delay(1000);                       // wait for a second
    }
    
  }

#ifdef SERIAL_DEBUG
  else
    Serial1.println("here we go");
#endif 
}

//////////
// LOOP //
//////////

void loop() {
  byte c;
  
  if (Serial.available()) {            // If anything comes in softSerial (USB),
    c = Serial.read();
    softSerial.write(c);               // read it and send it out softSerial
    
#ifdef SERIAL_DEBUG
    Serial1.print("S ");               // show what Serial has sent with S in front
    Serial1.println(c, HEX);
#endif  
  }

  if (softSerial.available()) {        // If anything comes in softSerial
    c = softSerial.read();
    Serial.write(c);                   // read it and send it out Serial (USB)

#ifdef SERIAL_DEBUG
    Serial1.print("N ");               // show what Nano has sent with N in front
    Serial1.println(c, HEX);
#endif
  }
}

///////////////////////////////////////////
// initialize the connection to the Nano //
// See remark 1 at the top of sketch     //
///////////////////////////////////////////
int setupNano(long baudRate)
{

#ifdef SERIAL_DEBUG
  nano.enableDebugging(Serial1);      // provide driver messages
#endif   
  
  nano.begin(softSerial);              //Tell the library to communicate over software serial port
  
  // Test to see if we are already connected to a module
  // This would be the case if the Arduino has been reprogrammed and the module has stayed powered
  softSerial.begin(baudRate);          //For this test, assume module is already at our desired baud rate
  while(!softSerial) ;                 //Wait for port to open

  int val1 = 0;

  // retry counter
  while (val1++ < 5)
  {
    //About 200ms from power on the module will send its firmware version at 115200. We need to ignore this.
    while(softSerial.available()) softSerial.read();
    
    nano.getVersion();
    
    if (nano.msg[0] == ERROR_WRONG_OPCODE_RESPONSE)
    {
      
#ifdef SERIAL_DEBUG
  Serial1.println(F("Module continuously reading. Asking it to stop..."));
#endif    
     
      //This happens if the baud rate is correct but the module is doing a continuous read
      nano.stopReading();
      delay(1500);
    }
    else if (nano.msg[0] != ALL_GOOD) 
    {
      
#ifdef SERIAL_DEBUG
  Serial1.print(F("set speed"));
#endif   
      //The module did not respond so assume it's just been powered on and communicating at 115200bps
      softSerial.begin(115200);   // Start software serial at 115200

      nano.setBaud(baudRate);     // Tell the module to go to the chosen baud rate. Ignore the response msg (if any)

      softSerial.begin(baudRate); // Start the software serial port, this time at user's chosen baud rate
    }
    else
    {
      
#ifdef SERIAL_DEBUG
  Serial1.println("all good\n");
#endif       
      return(1);    // Responded correctly
    }
  }

  return(0);
}
