/*
  NANO SerialPassthrough sketch
  
  **************************************************************************************
  Created for using an Arduino as a bridge by connecting a Nano RFID simultaneous 
  reader on softSerial and communicate of USB/Serial to a remote program (like the URA).     
  */
  //                 ====== READ ALL THE INFORMATION IN THE SKETCH =======
  /*                
  By Paul van Haastrecht
  
  Note : Part of the Nano connection code is taken from the example sketch from Sparkfun   
  ***************************************************************************************
  
  BACKGROUND
  The Arduino will reset based on a reset pulse, which is generated from the DTR signal of the USB OR at power-on 
  OR pressing the reset-button. The DTR signal from the USB is causing a reset everytime the USB port is opened.
  This reset will start the bootloader which will check for a certain time (typically 1 second) for any
  incoming data on the serial line. If received it will try to start the protocol to load a new sketch
  in flash memory. If not received, it will timeout and start the program that is already loaded (if any)
    
  A problem with the URA (and other applications) is that they do not wait for the 1 second after opening the port, 
  but start sending immediately. This will cause the bootloader to think it is getting a new program to load. Only after
  NOT getting valid data and MAX_TIME_OUT after the last character received, it will start the program in flash. (if any)

  BOOTLOADER
  I am not an expert on bootloaders, but have been reading a lot about it as well as study the source code.
  I have tested with an MEGA2560, with an STK500V2 bootloader. The nice aspect is that I can connect another serial 
  debug monitor and can check how the program is behaving (not the bootloader !). 
  The STK500V2 bootloader has an initial time-out of about 1 second. When I used the test program I was able to 
  detect the same timing. If any character received within the 1 second it will loop to get the full command. 
  During that get_command_loop it will call recchar_timeout(). When a character is received within MAX_TIME_COUNT 
  (defined as (F_CPU >> 1)) it will return that character, if however it exceeds MAX_TIME_OUT it
  will start the program in flash (if any)

  CONNECTING URA
  During my test I connected a scope and the first message (requesting version info) is sent after 4ms.
  After 100ms this is repeated. This is defined in the mercuryAPI as part of the OpenSerialPort() call, where it 
  calls for CmdVersion() and the transporttimeout is set.
  The transport timeout is defined as :  how long we expect the command itself to take (milliseconds).
  The first message sent (after 4ms) is lost. The bootloader is not ready. After 100mS the bootloader is ready and triggered: now
  the trouble starts. The received information is not valid for bootloader, but the Arduino stays in bootloader trying to get valid data. 
  The URA (and other MercuryAPI based applications) will repeat sending the version request every 100ms. 
  Typically 9 times on different speeds : 9600, 9600, 115200, 921600, 19200, 38400, 57600, 230400, 460800.
  If not connected after 9 times the URA will give up and show an error message. 
  Now the bootloader will time-out and the program in flash is started.... but no URA looking for it.
  
  CONNECTING A TEST PROGRAM
  In order to prove the point, I have created a simple C-program (pulsetest.c) that I run on my Raspberry PI.
  I had to remove a number of error-checks and printf() statements in order to achieve the timing as the URA has.
  With that I could demonstrate the points I am making above.
  I have tried to sent a completely different request than a version request, but the result is the same.
  The URA actually sent 2 x DTR spike (so 2 times reset when opening for some reason) after each other within 400mS. 
  I have simulated that, but there is no impact, the result is the same.

  I then starting extending the time for the second message and discovered that this needs to be 1 second. (as defined in the STK500V2 source code)
  
  WHAT CAN BE DONE?
  
  1. Change the MercuaryAPI source code so it will wait for more than 1 second before trying to connect.
     A high level description on the changes needed are in a separate document
  2. Disable the RESET signal happening through software 
     A. Use a GPIO to set RESET high (used in this sketch)
     B. Adjust the High Fuse to ignore reset. Not recommended ! (see http://www.martyncurrey.com/arduino-atmega-328p-fuse-settings/ for some background)
  3. Disable the RESET signal happening through hardware. 
     A. On some boards there is a way to cut a link between DTR and RESET.
     B. Put a wire in between RESET and +5V
  */
  //                        =====================================================================
  // ====================== MAKE SURE TO READ THE INFORMATION ABOUT THE BLOCKER STARTING LINE 328 ================
  //                        =====================================================================
  /*
  
  !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  This might not be a stable connection for a number of reasons :
  !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  
  1. softSerial and Nano must be at the same speed. The Nano will always be at 115K after reboot. That is too fast for softSerial. 
  During init we try connect with the lower speed. If that fails, we assume a reboot has happened: set softSerial to 115K and 
  send a lower baudrate command and then reset softSerial to that lower baudrate. 
  However if the Nano was not either the requested baudrate or 115K nothing will not connect. 

  2. softSerial has a default RX buffer of 64 characters. The chances of loosing characters due to buffer overrun is high. 
  Keep reading Serial speed faster than the Nano to prevent buffer overrun as much as possible

  3. Next to the softSerial buffer, the Nano also has it's buffer and when in continues mode I got many errors with a NANO speed below
  38400. That was not caused by softSerial but the Nano responding with rubbish. It seems that 38400 is a save speed to use on an Arduino.
  The ESP32 was working stable on 19200.

  4. Changing the serial baudrate by the remote program during setting and running the connection is not stable.
  The sketch will NOT change the Nano speed, but the speed of the serial connection. The Arduino needs time to change the 
  baudrate (with a serial.begin() call) and the remote program might already be sending before that time has passed. 
  During testing it worked sometime, but failed more often.
  It seems the URA is getting lost also as it would indicate the new requested speed of 38400, but continue to communicate on 115K

  Set the CON_SPEED_SER to the same speed as the remote program wants to communicate and avoid baudrate changes as much as possible. 
  Try to keep that to 115200.
  */
  
  //                        ================================
  //========================          SKETCH USAGE          ================
  //                        ================================ 
  
  /*
  CONNECT THE HARDWARE ACCORDING TO THE HARDWARE SECTION BELOW
  
  MAKE SURE TO SET THE PARAMETERS CORRECT (STARTING LINE 267)

  DISCONNECT THE WIRE BETWEEN RESET AND THE DISABLE_RESET_PIN.(if any)
  
  UPLOAD THE SKETCH
 
  RECONNECT THE WIRE BETWEEN RESET AND THE DISABLE_RESET_PIN. (if wanted)
  
  ALWAYS WAIT AFTER THE POWER-ON TO CONNECT THE REMOTE PROGRAM
  The on-board LED will blink after applying power and should within 15 seconds lid up stable. Now you are good to go!
  On failure to connect and initialize the Nano theafter 1- - 15 seconds on-board led will blink every second.
  
  NOTE 1:
  You can NOT just connect on pin 0 and 1 (the standard serial). Even with an empty sketch the driver 
  on the Arduino will trigger interrupt with each character.
  
  NOTE 2:
  Always make sure to power the RFID shield with a good power supply. Many communication failures come from that as well.

  NOTE 3:
  You will need the right driver on the remote system (Windows, Linux etc) to communicate with your Arduino/ ESP32

  NOTE 4:
  If you use the RESET BLOCKER and want to upload a new sketch, you have to do that before the sketch has connected successfully
  with the Nano. Otherwise the reset will be blocked and the bootloader will not be started. 
  You can also temporary remove the wire between RESET and DISABLE_RESET_PIN.

  NOTE 5:              
  This sketch has been tested against the standard demo.c, URA from Jadak (current and older versions).
  
  To use 'demo' on Windows from the samples directory :
  start cmd
  go to demo directory in the mercury API
  demo tmr:///COM4 shell                                           // or other valid COM-port to start interactive
  >>> set /reader/read/plan SimpleReadPlan:[TagProtocol:GEN2,[1]]  // enable GEN2 and antenna 1
  >>> read                                                         // will read tags for 1 second and and display
  >>> help set                                                     // will provide help information
  
  To use 'URA'  on Windows : select antenna 1, read/write power to max 5DB (if on VUSB)
  */
  
  //                        ================================
  //========================        HARDWARE CONNECTION     ================
  //                        ================================ 
  
  /*
  Connected to an Arduino
  Using lose wires to connect (switch on shield to HW-UART) 
  
  RFID shield       Arduino
  GND                GND
  NC (not connected)
  VCC                5V
  RX                 RFID_TX
  TX                 RFID_RX
  NC (not connected)
                     RESET  <> DISABLE_RESET_PIN   (For RESET BLOCKER)
  EN (not connected)
  
                    ****************
                    
  Putting the shield on top of ARDUINO ( switch on shield to SW-UART)
  
  RFID_RX 2
  RFID_TX 3
  RESET  <> DISABLE_RESET_PIN   (for RESET BLOCKER)
  EN (not connected)

  This will not work on an MEGA2560 as it does not have interrupts on pin 2.
  
                    ******************
                    
  Connected to ESP32 Thing Sparkfun
  
  Set the onboard switch to HW-UART and connect with lose wires.
 
  RFID reader     ESP32 thing  (Serial2 is used)
  GND ----------------- GND
  NC (not connected)
  VCC ----------------- VUSB    (OR external power)
  UART TX ---- LS------ 16      (LS = level shifter !!)
  UART RX ------------- 17
  NC (not connected)
  
  EN (not connected)
  RESET  <> DISABLE_RESET_PIN   OPTIONAL (see RESET BLOCKER information in sketch below)

  !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
   WARNING!! ESP32 !!WARNING!! ESP32 !!WARNING!! ESP32 !! WARNING!! ESP32 !!
  !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  
  The NANO M6E needs at least 3.7V VCC. It will not work stable on 3.3V.
  
  If you connect to the 5V (VUSB) you MUST use a level shifter for the for RX line from RFID reader to the ESP32.
  The pins on the ESP-32 can only handle 3V3 signals !!
  
  The RFID RX line, TX / pin 17 from ESP32, actually does not need a level shifter. The 3V3 that is coming from the ESP32 works good for the Nano
  HOWEVER for the TX FROM the Nano, did not work well with that level shifter (https://www.sparkfun.com/products/12009)
  Apparently the level shifter on the Nano and this level shifter do not work well together. So I made it working with resistors:
  
            ________           _______
  GND ------| 10K  |-----!---- | 5k6 |------  TX from Nano
            --------     !     -------
                         !
                    pin 16 (ESP32)
  */
  
  //                 =================
  //=================     VERSION     ================
  //                 ================= 
  
  /*
  November 2018, Paul van Haastrecht
  # initial version

  April 2019, Paul van Haastrecht
  # added support for baudrate change (see remark 4 earlier)
  # added blocking reset signal
  # tested with URA and demo running from Windows PC
  # added support for ESP32 THing Sparkfun
  */

  //                 =================
  //=================   Prerequisites ================
  //                 ================= 
  
  /*
   You must have installed the Sparkfun RFID Simultaneous reader library:
   
   https://github.com/sparkfun/SparkFun_Simultaneous_RFID_Tag_Reader_Library/archive/master.zip
   */
  
  //                 ==================
  //=================    DISCLAIMER    ================
  //                 ================== 
  //
  //
  // Delivered as-is. No support, no warranty, good luck.
  /*
   This program is distributed in the hope that it will be useful,
   
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
   See the GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <SoftwareSerial.h>           // Used for transmitting to the reader
#include <SparkFun_UHF_RFID_Reader.h> // Library for controlling the M6E Nano module

/*  Enable debug monitor
 *   
 *  Tested on a MEGA2560 which has a spare serial port (Serial1)
 *  
 *  Arduino       debug monitor
 *  GND            GND 
 *  pin 18 TXD     RXD  
 *  
 *  On an ESP32 Serial1 is used and pin 13 = RX, pin 12 = TX
 *  
 *  ESP32       debug monitor
 *  GND            GND 
 *  pin 12 TXD     RXD    
 * 
 * remove comment // before the ' #define SERIAL_DEBUG' to enable debug */
//#define SERIAL_DEBUG 

/* Define the pins to use for M6E NANO
 * ============ ARDUINO ============= 
 * Lose wires : (switch HW-UART)
 * on a mega tested working with pin TX=11 and RX=12. Do NOT use pin 13 as that is the on-board led
 * on an UNO tested working with pin TX=11 and RX=12
 * 
 * Placed on top of Arduino (switch SW-UART)
 * on an UNO tested working with pin TX=3  and RX= 2
 * on a MEGA2560 this does not work as there is NO interrupt on pin 2.
 * 
 * =========== ESP32 ====================
 * Lose wires : (switch HW-UART)
 * serial2 is used for the Nano and thus pin 16 and 17 are used by default
 * The RFID_TX and RFID_RX are ignored.
 * Make sure to remove "//" before //#define ESP32  to enable Serial2
 */
//#define ESP32             // comment out for Arduino
#define RFID_TX 12
#define RFID_RX 11

/* define speeds (see notes 1 - 3 in sketch above)
 * softSerial has default a receive buffer of 64 bytes (which is small but often enough).
 * Keep reading Serial speed faster or equal to the Nano to prevent buffer overrun as much as possible
 * Additional one can adjust the buffer size in softSerial.h to a larger size*/
#define CON_SPEED_NANO  38400     // M6E Nano speed (ESP32 best : 19200, Mega2560 & UNO  best: 38400)
#define CON_SPEED_SER   115200    // serial/USB speed
#define CON_SPEED_DEBUG 115200    // debug monitor

/* define the size of the buffer to the requests send to the Nano
 * Optional to change */
#define REC_BUF_SIZE  80

/* if in continuous mode the Nano will keep on firing data. However the remote program
 * may want to stop that. Hence after MAX_RESPONSE_COUNT messages in response of 1 request, 
 * it will check whether a new command from the remote program was received  
 * Optional to change */
#define MAX_RESPONSE_COUNT 3

/* if M6E Nano is powered with USB power, give the Nano time to settle. For my ESP32 this was
 * necessary. The Arduino did not seem to have an issue.
 * The value is in mS, thus 2000 = 2 second delay.
 * If not necessary comment '//' out the line before #define WAIT_AFTER_POWER_ON 2000  */
#define WAIT_AFTER_POWER_ON 2000

 //                     ==================================
 // ===================  RESET BLOCKER !!! READ FIRST !!! =====================================
 //                     ==================================
 /* In this sketch I have choosen for option 2A to block RESET (see top of sketch):
 * 
 * Connecting a wire between pin DISABLE_RESET_PIN and RESET pin on the POWER connectors will 
 * set the RESET signal HIGH for as long as the (always) first request for version information 
 * is not received. The system will not reset when the USB port is opened. 
 * If the version information request is detected the RESET will return to normal operation, 
 * UNLESS the KEEP_BLOCK_RESET is defined, it will keep blocking reset. This is preferred.
 */
 // =================================================================================================================
 // ============ DO NOT RESET THE SYSTEM WITH THE RESET BUTTON AS THAT WILL CAUSE SHORT CIRCUIT AND DAMAGE TO AVR. =
 // ============ TO RESET: REMOVE AND RECONNECT THE POWER                                                           =
 // =================================================================================================================
 // 
 // ESP32
 /* This is not really necessary of an ESP32. The Windows driver for the ESP32 is sending a number(4) of resets when  
  * power is applied to the board. When I then press "connect" in the URA it fails the first time (as it sends another 
  * reset) but subsequent "connect" all worked (as no reset is send). 
  * 
  * If you do NOT want to use the reset blocker: put a '//' before #define ENABLE_BLOCK_RESET.
  */

#define ENABLE_BLOCK_RESET      // enables the software-RESET-block 
#define DISABLE_RESET_PIN 5     // the pin to connect with the RESET PIN
#define KEEP_BLOCK_RESET        // keeps reset blocked (no matter the version request was received)

////////////////////////////////////////
// No change needed beyond this point //
////////////////////////////////////////

// sometimes the pre-processor does not work
int setupNano(long Nano_baudRate);
bool read_from_serial();
bool sent_to_serial(int c);

// RFID reader connection
#ifdef ESP32
  #define NANO_COM  Serial2
#else
  SoftwareSerial softSerial(RFID_RX, RFID_TX); //RX, TX
  #define NANO_COM  softSerial
#endif

// constructor
RFID nano;

// Define debug port
#ifdef SERIAL_DEBUG
  #define DEBUG_COM Serial1
#endif

// Buffer to store request from remote program
int rec_index;
byte rec_buffer[REC_BUF_SIZE];

///////////
// Setup //
///////////
void setup() {
  
  // set serial speed
  Serial.begin(CON_SPEED_SER);

  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_BUILTIN, OUTPUT);

#ifdef SERIAL_DEBUG

#ifdef ESP32
  DEBUG_COM.begin(CON_SPEED_DEBUG, SERIAL_8N1, 13, 12, false);  // pin 13 = RX, pin 12 = TX
#else
  DEBUG_COM.begin(CON_SPEED_DEBUG);
#endif // ESP32

  DEBUG_COM.println(F("Connected with Debug Monitor"));
#endif // SERIAL_DEBUG

#ifdef WAIT_AFTER_POWER_ON
  // if Nano powered with USB, give time time to settle
  delay(WAIT_AFTER_POWER_ON);
#endif

#ifdef ENABLE_BLOCK_RESET 
  // initialize digital pin for RESET as INPUT.
  pinMode(DISABLE_RESET_PIN, INPUT);
#endif

  // try to connect the Nano reader
  if (! setupNano(CON_SPEED_NANO))
  {

#ifdef SERIAL_DEBUG
    DEBUG_COM.println(F("failed to connect Nano"));
#endif   
    
    //loop forever and blink led to indicate an error
    for(;;) 
    {
        digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on 
        delay(1000);                       // wait for a second
        digitalWrite(LED_BUILTIN, LOW);    // turn the LED off 
        delay(1000);                       // wait for a second
    }
  }

#ifdef ENABLE_BLOCK_RESET 
  // PULL RESET HIGH and thus block trigger DTR signal.
  digitalWrite(DISABLE_RESET_PIN, HIGH);
  pinMode(DISABLE_RESET_PIN, OUTPUT);
#endif
  
  // reset rec_buffer rec_index
  rec_index=0;

  // set led to indicate we are good
  digitalWrite(LED_BUILTIN, HIGH);          // turn the LED on
  
#ifdef SERIAL_DEBUG
  DEBUG_COM.println(F("Nano connected. Good to go"));
#endif 
}

///////////////
// MAIN LOOP //
///////////////

void loop() {
  int i, resp_count;

  // if complete request received from remote application
  if (read_from_serial())
  {
    // send to softSerial / Nano
    for (i = 0; i < rec_index; i++)
      NANO_COM.write(rec_buffer[i]);    
     
    rec_index = 0;
  }
  
  resp_count = 0;  
  
  // If anything comes in softSerial / Nano
  while (NANO_COM.available()) {
    
    // send it out Serial (USB). Return true if new response was starting
    if (sent_to_serial((int) NANO_COM.read())) {

       /* if in continuous mode the Nano will keep on firing data. However the remote program
        * may want to stop that. Hence after MAX_RESPONSE_COUNT messages in response of 1 request, 
        * it will check whether a new command from the remote program was received.
        * 
        * Even if this is received by the Nano, it will take sometime before the pending data in 
        * the UART buffer and Nano buffer are empty (e.g. the URA may not like that )*/
      if (++resp_count > MAX_RESPONSE_COUNT) {
        delay(5);       // cause a delay to allow USART interrupts (seems to be necessary)
        break;
      }
    }
  }
  
/*  
#ifdef SERIAL_DEBUG
    if (resp_count > 0) DEBUG_COM.println(F("\nCheck for request"));
#endif   
*/
}
///////////////////////////////////////////
// initialize the connection to the Nano //
// See remark 1 - 3 at the top of sketch //
// Modified from Sparkfun Samples sketch //
///////////////////////////////////////////
int setupNano(long Nano_baudRate)
{

#ifdef SERIAL_DEBUG
  nano.enableDebugging(DEBUG_COM);    // enable driver messages
#endif   

  nano.begin(NANO_COM);               // Tell the library the communicate port
   
  // Test to see if we are already connected to a module
  // This would be the case if the Arduino has been reprogrammed and the module has stayed powered
  NANO_COM.begin(Nano_baudRate);      // For this test, assume module is already at our desired baud rate
  while(!NANO_COM) ;                  // Wait for port to open  

  int val1 = 0;

  // retry counter
  while (val1++ < 5)
  {
    //About 200ms from power on the module will send its firmware version at 115200. We need to ignore this.
    while(NANO_COM.available()) NANO_COM.read();
  
    nano.getVersion();
    
    if (nano.msg[0] == ERROR_WRONG_OPCODE_RESPONSE)
    {
      
#ifdef SERIAL_DEBUG
      DEBUG_COM.println(F("Module continuously reading. Asking it to stop..."));
#endif    
     
      // This happens if the baud rate is correct but the module is doing a continuous read
      nano.stopReading();
      delay(1500);
    }
    else if (nano.msg[0] != ALL_GOOD) 
    {
      
#ifdef SERIAL_DEBUG
      DEBUG_COM.print(F("Try setting Nano speed to "));
      DEBUG_COM.println(Nano_baudRate);
#endif
    
      //The module did not respond so assume it's just been powered on and communicating at 115200bps
      NANO_COM.begin(115200);         // Start communication at 115200
      nano.setBaud(Nano_baudRate);    // Tell the module to go to the chosen baudrate. Ignore the response msg (if any)
      NANO_COM.begin(Nano_baudRate);  // Start the communication port, this time at user's chosen baud rate
    }
    else
    {
      
#ifdef SERIAL_DEBUG
      DEBUG_COM.println(F("all good\n"));
#endif       
      return(1);    // Responded correctly
    }
  }
  return(0);
}

/////////////////////////////////////////////////////////////////////////////////////
// A remote application (like the URA) can sent a command to change the baudrate.  //
// This is NOT forwarded to the reader but the Serial port speed will be adjusted. //
// see Note 4 in the top of sketch
/////////////////////////////////////////////////////////////////////////////////////
void handle_speed_change()
{
  long br = 0;
  
  // last requested change in speed
  static long last_serial_request = CON_SPEED_SER;
  
  // check opcode and length
  if (rec_buffer[2] != 0x6 || rec_buffer[1] == 0) return;

  // sometimes the length is 16 bit (old firmware ?)
  if (rec_buffer[1] == 2)
      br = (long)  rec_buffer[3] << 8 | (long) rec_buffer[4];
  else
      br = (long) rec_buffer[3] << 24 | (long) rec_buffer[4] << 16 | (long) rec_buffer[5] << 8 | (long) rec_buffer[6];
  
  // send baudrate change acknowledgement for remote program
  sent_to_serial(0xff);
  sent_to_serial(0x00);
  sent_to_serial(0x06);
  sent_to_serial(0x00);
  sent_to_serial(0x00);
  sent_to_serial(0xE4);
  sent_to_serial(0x06);
  
  if (br != last_serial_request) {

#ifdef SERIAL_DEBUG
  DEBUG_COM.print(F("change baudrate to "));
  DEBUG_COM.println(br);
#endif
   Serial.begin(br);
   last_serial_request = br;       // remember last request
  }
  else {
    
#ifdef SERIAL_DEBUG
    DEBUG_COM.print(F("No change to baudrate "));
    DEBUG_COM.println(br);
#endif
   }
}

//////////////////////////////////////////////////////////////////
// Reads full request from the serial line / remote program
//     
// returns:
// true  : if full request for Nano is received 
// false : no request message ready for Nano 
//////////////////////////////////////////////////////////////////
bool read_from_serial()
{
  static byte rec_length = 0;         // hold request length
  static bool Speed_request = false;  // check for baudrate change
  byte c;
  
  while (Serial.available()) {        // If anything comes in Serial (USB),
    c = Serial.read();
    
    if (rec_index == 0) {             // check for correct start header
      if (c != 0xff) continue;
      rec_length = 0;
    }
    
    // get data length
    else if (rec_index == 1) rec_length = c;

    // check for OPCODE
    else if (rec_index == 2) {

      // check for request change in connection speed
      if (c == 0x6)  Speed_request = true; 

#ifdef ENABLE_BLOCK_RESET 

#ifndef KEEP_BLOCK_RESET                // only if NOT requested to remove RESET blocker  
      // check for the ALWAYS first request(get version info)
      // 0xff 0x0 0x3 0x1d 0x0c
      else if (c == 0x3 && rec_length == 0){

#ifdef SERIAL_DEBUG          
       DEBUG_COM.println(F("Remove reset blocker"));
#endif  // SERIAL_DEBUG 
   
       // Disable RESET signal blocker.
       pinMode(DISABLE_RESET_PIN, INPUT);
      }
#endif  // KEEP_BLOCK_RESET
  
#endif  // ENABLE_BLOCK_RESET  
    }
    
    rec_buffer[rec_index++] = c;    // store received character in buffer

    // if buffer overrun disregard what has been received
    if (rec_index == REC_BUF_SIZE) {
      rec_index = 0;
      return(false);
    }
  }
  
  // if all data : header + length + OPCODE + data + CRC + CRC => rec_length + 5 overhead
  if(rec_index == rec_length + 5) {

#ifdef SERIAL_DEBUG
   DEBUG_COM.print(F("\nRX from program :"));
     for (byte i = 0; i < rec_index; i++){
       DEBUG_COM.print(F(" 0x"));    
       DEBUG_COM.print(rec_buffer[i], HEX);
   }
   DEBUG_COM.println();
#endif
   
   if (Speed_request) {           // was request to change speed sent by remote
      handle_speed_change();
      rec_index = 0;
      Speed_request = false;
   }
   else
      return(true);
  }
  
  return(false);
}

///////////////////////////////////////////////////////////
// sent a character to the serial / remote program
// 
// Returns: 
// True  : if starting header was received from Nano 
// False : if data within response was received from Nano
//////////////////////////////////////////////////////////
bool sent_to_serial(int c)
{
  bool ret = false;                     // set return code
  Serial.write(c);                      // send it out Serial (USB)

  static bool first_time = true;        // print debug line start
  static bool next_is_length = false;   // catch the length of the response
  static int  res_length = 0;           // length of response

#ifdef SERIAL_DEBUG
  if (first_time) {
    DEBUG_COM.print(F("TX to program:"));
    first_time = false;
  }

  DEBUG_COM.print(" 0x");  
  DEBUG_COM.print(c, HEX);
#endif
  
  /* There might a flood of data coming from the Nano
   * 
   * As a header-character is detected and a debug line has not been started
   * this means start of response
   */
  
  /* number of bytes to receive : opcode + 2 x status + data (=length) + 2 x CRC => length + 5 overhead
   * it will follow after the header */
  if (next_is_length) {
    res_length =  c + 5;
    next_is_length = false;
  }

  /* check on length to receive is valid */
  else if (res_length > 0) {
 
    // did we handle the last byte of the response 
    if (res_length-- == 1) {
#ifdef SERIAL_DEBUG  
        first_time = true;    // reprint debug linestart
        DEBUG_COM.println();  // terminate current debug line
#endif
    }
  }
  else
  {
    // header detected, Start of new response.
    if (c == 0xff) {
      next_is_length = true;  // trigger to use next received byte as length
      ret = true;             // return to indicate new response was received
    }
  }
  return(ret);
}
