

#include <ESP8266WiFi.h>
#include "time.h"
#include "Wire.h"
#include <NTPClient.h>
#include <WiFiUdp.h>
#define DS3231_I2C_ADDRESS 0x68

#define bcd2dec(bcd_in) (bcd_in >> 4) * 10 + (bcd_in & 0x0f)
#define dec2bcd(dec_in) ((dec_in / 10) << 4) + (dec_in % 10)

#define latchPin 16
#define clockPin 14
#define dataPin 12
#define ledPin 13

// Replace with your network credentials
const char* ssid = "";
const char* password = "";

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

byte mByte[0x07]; // holds the array from the DS3231 register
byte tByte[0x07]; // holds the array from the NTP server

// change the values here to suit your timezone. This is for Perth, Western Australia.

const long gmtOffset = 3600 * 8; // 3600 seconds is +1 hours
const long summerTimeOffset = 0; // change to 0 in winter

//Week Days
String weekDay[7]={"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

//Month names
String month[12]={"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};

void setup(){
  //Set the pins for the 74HC595
  pinMode(latchPin, OUTPUT);
  pinMode(dataPin, OUTPUT);  
  pinMode(clockPin, OUTPUT);

  //output for the led colons
  pinMode(ledPin, OUTPUT);

  Serial.begin(115200); 
  Serial1.begin(1200);
  
  while(!Serial){} // Wait for serial connection

  Wire.begin();
  
  // set up wireless internet connection

  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n");

// Initialize a NTPClient to get the time
  timeClient.begin();
  // Set offset time in seconds to adjust for your timezone, for example:
  // GMT +1 = 3600
  // GMT +8 = 28800
  // GMT -1 = -3600
  // add another 3600 for Summertime if appropriate
  // GMT 0 = 0
  // Example CET = GMT/UTC+1 and CEST = GMT/UTC+2 i.e. 2 x 3600 = 7200
  timeClient.setTimeOffset(gmtOffset + summerTimeOffset);
  
  //get current time from the RTC registers and store in the mByte[] array
  Serial.println(F("Old DS3231 register content........\n"));  
  getRTCdatetime();

  // get the datetime from the NTP server
  
  timeClient.update();

  unsigned long epochTime = timeClient.getEpochTime();  
  tByte[0] = (int)timeClient.getSeconds();  
  tByte[1] = (int)timeClient.getMinutes();  
  tByte[2] = (int)timeClient.getHours();  
  tByte[3] = (int)timeClient.getDay();
  
  // create a struct to hold date, month and year values  
  struct tm *ptm = gmtime ((time_t *)&epochTime);
  tByte[4] = (int)ptm->tm_mday;  
  tByte[5] = (int)ptm->tm_mon+1;  
  tByte[6] = (int)ptm->tm_year-100;

  Serial.print(F("NTP Webtime..........\n\n"));

  Serial.print("Seconds: "); Serial.println(tByte[0]);
  Serial.print("Minutes: "); Serial.println(tByte[1]);
  Serial.print("Hours:   "); Serial.println(tByte[2]);
  Serial.print("DoW:     "); Serial.println(weekDay[tByte[3]]);
  Serial.print("Day:     "); Serial.println(tByte[4]);
  Serial.print("Month:   "); Serial.println(month[tByte[5]-1]);
  Serial.print("Year:    "); Serial.println(tByte[6]);
  Serial.print("\n");

  /* if the time stored in the DS3231 register does not match
   *  the time retrieved from the NTP server, update the DS3231
   *  register to the current time.
   */
  if(mByte != tByte){
    Wire.beginTransmission (0x68);
    // Set device to start read reg 0
    Wire.write (0x00);
      for (int idx = 0; idx < 7; idx++){
        Wire.write (dec2bcd(tByte[idx]));
      }   
    Wire.endTransmission ();
    }
    
  Serial.println(F("New DS3231 register content........\n"));  
  getRTCdatetime();
  analogWrite(ledPin, 3);
  
}

void loop() {
  //debugvalues();
  updateNixies();
  debugvalues();
  delay(10);
}

void getRTCdatetime(){
  uint16_t a = 0x68;
  size_t s = 0x07;
  bool ssb = true;
  Wire.beginTransmission (0x68);
  // Set device to start read reg 0
  Wire.write (0x00); 
  Wire.endTransmission ();
  // request 7 bytes from the DS3231 and release the I2C bus
 // Wire.requestFrom(a, s, ssb); THIS LINE RAISES AN ERROR
  Wire.requestFrom(a, s);
  int idx = 0;
  // read the first seven bytes from the DS3231 module into the array
  while(Wire.available()) {
    byte input = Wire.read(); // read each byte from the register
    mByte[idx] = input;    // store each single byte in the array
    idx++;
  }
  // display the current values to the serial monitor
  Serial.print(F("Register[0] Seconds: ")); Serial.println(bcd2dec(mByte[0]));
  Serial.print(F("Register[1] Minutes: ")); Serial.println(bcd2dec(mByte[1]));
  Serial.print(F("Register[2] Hours:   ")); Serial.println(bcd2dec(mByte[2]));
  Serial.print(F("Register[3] DoW:     ")); Serial.println(weekDay[bcd2dec(mByte[3])]);
  Serial.print(F("Register[4] Day:     ")); Serial.println(bcd2dec(mByte[4]));
  Serial.print(F("Register[5] Month:   ")); Serial.println(month[bcd2dec(mByte[5]-1)]);
  Serial.print(F("Register[6] Year:    ")); Serial.println(bcd2dec(mByte[6]));
  Serial.println("\n");
}

void updateNixies(){
  uint16_t a = 0x68;
  size_t s = 0x07;
  bool ssb = true;
  Wire.beginTransmission (0x68);
  // Set device to start read reg 0
  Wire.write (0x00); 
  Wire.endTransmission ();
  // request 7 bytes from the DS3231 and release the I2C bus
  //Wire.requestFrom(a, s, ssb); CAUSES AN EXCEPTION!
  Wire.requestFrom(a, s);
  int idx = 0;
  // read the first seven bytes from the DS3231 module into the array
  while(Wire.available()) {
    byte input = Wire.read(); // read each byte from the register
    mByte[idx] = input;    // store each single byte in the array
    idx++;
  }
    digitalWrite(latchPin, LOW);
   shiftOut(dataPin, clockPin, LSBFIRST, mByte[0]);
   shiftOut(dataPin, clockPin, LSBFIRST, mByte[1]);
   shiftOut(dataPin, clockPin, LSBFIRST, mByte[2]);
   digitalWrite(latchPin, HIGH);
  }
void debugvalues(){
  Serial.println(mByte[2]);
  Serial.println(mByte[1]);
  Serial.println(mByte[0]);
  Serial1.println(mByte[2]);
  delay(1000);
}
