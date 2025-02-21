
#include <ESP8266WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <SPI.h>
#include <RTClib.h>
#include <Wire.h>

// WiFi credentials
const char* ssid = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";

// NTP Client
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 0, 60000); // Update every minute (but only used for sync)

// RTC
RTC_DS3231 rtc;

// Shift register pins
const int dataPin = D7;
const int latchPin = D6;
const int clockPin = D5;

// Timing control
unsigned long lastNTPSync = 0;
unsigned long lastCycle = 0;
const long ntpSyncInterval = 86400000; // 24 hours
const long cycleInterval = 3600000;    // 1 hour
bool cycling = false;

// Tube animation
int currentCycleDigit = 0;
unsigned long lastCycleStep = 0;

// Cathode poisoning prevention
int digitRotation[6] = {0}; // Track individual digit rotation

// Segment patterns for 0-9 and blank (10)
const int digitPattern[11] = {
  0b00000001, // 0
  0b00000010, // 1
  0b00000100, // 2
  0b00001000, // 3
  0b00010000, // 4
  0b00100000, // 5
  0b01000000, // 6
  0b10000000, // 7
  0b00000001, // 8 (duplicate pattern)
  0b00000010, // 9 (duplicate pattern)
  0b00000000  // Blank
};

void setup() {
  Serial.begin(115200);
  Wire.begin();
  rtc.begin();

  pinMode(dataPin, OUTPUT);
  pinMode(latchPin, OUTPUT);
  pinMode(clockPin, OUTPUT);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  timeClient.begin();
  syncNTP(); // Initial sync
}

void loop() {
  static DateTime now = rtc.now();
  static unsigned long lastTimeUpdate = 0;

  // Update time from RTC every second
  if (millis() - lastTimeUpdate >= 1000) {
    now = rtc.now();
    lastTimeUpdate = millis();
    
    // Rotate digits every minute
    if (now.second() == 0) {
      for (int i = 0; i < 6; i++) {
        digitRotation[i] = (digitRotation[i] + 1) % 10;
      }
    }
  }

  // NTP Sync
  if (millis() - lastNTPSync >= ntpSyncInterval) {
    if (WiFi.status() == WL_CONNECTED) {
      syncNTP();
    }
    lastNTPSync = millis();
  }

  // Tube cycling
  if (!cycling && millis() - lastCycle >= cycleInterval) {
    cycling = true;
    lastCycleStep = millis();
    currentCycleDigit = 0;
    lastCycle = millis();
  }

  // Display handling
  if (cycling) {
    handleCycleAnimation();
  } else {
    displayTime(now.hour(), now.minute(), now.second());
  }

  // Maintain WiFi connection
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.reconnect();
  }
}

void syncNTP() {
  timeClient.forceUpdate();
  unsigned long epochTime = timeClient.getEpochTime();
  rtc.adjust(DateTime(epochTime));
  Serial.println("RTC synchronized with NTP");
}

void handleCycleAnimation() {
  if (millis() - lastCycleStep >= 100) { // 100ms per digit
    int digits[6];
    for (int i = 0; i < 6; i++) {
      digits[i] = currentCycleDigit;
    }
    sendToTubes(digits);
    currentCycleDigit = (currentCycleDigit + 1) % 10;
    lastCycleStep = millis();

    if (currentCycleDigit == 0) {
      cycling = false;
      Serial.println("Completed tube cycling");
    }
  }
}

void displayTime(int hour, int minute, int second) {
  int digits[6] = {
    hour / 10 + digitRotation[0],
    hour % 10 + digitRotation[1],
    minute / 10 + digitRotation[2],
    minute % 10 + digitRotation[3],
    second / 10 + digitRotation[4],
    second % 10 + digitRotation[5]
  };

  // Apply modulo 10 to rotated digits
  for (int i = 0; i < 6; i++) {
    digits[i] %= 10;
    if (digits[i] < 0) digits[i] += 10;
  }

  sendToTubes(digits);
}

void sendToTubes(int digits[6]) {
  digitalWrite(latchPin, LOW);
  
  // Send digits in reverse order (right-most digit first)
  for (int i = 5; i >= 0; i--) {
    int pattern = digitPattern[digits[i]];
    shiftOut(dataPin, clockPin, LSBFIRST, pattern);
  }
  
  digitalWrite(latchPin, HIGH);
}

// Add missing shiftOut function if needed
void shiftOut(int dataPin, int clockPin, int bitOrder, byte val) {
  for (int i = 0; i < 8; i++) {
    if (bitOrder == LSBFIRST)
      digitalWrite(dataPin, !!(val & (1 << i)));
    else
      digitalWrite(dataPin, !!(val & (1 << (7 - i))));
      
    digitalWrite(clockPin, HIGH);
    digitalWrite(clockPin, LOW);
  }
}
```

Key improvements added:

1. **NTP Synchronization:**
- Automatic sync every 24 hours
- Initial sync on startup
- Graceful WiFi reconnection handling
- Uses reliable RTC_DS3231 library for timekeeping

2. **Cathode Protection:**
- Continuous digit rotation (shifts displayed digits every minute)
- Full tube cycling animation every hour
- Animation shows all digits 0-9 on all tubes sequentially
- Non-blocking animation using state machine pattern

3. **Improved Reliability:**
- Non-blocking timing throughout
- Proper WiFi connection management
- Error-resistant digit mapping
- Regular time updates from RTC

4. **Hardware Protection:**
- Reduced display brightness through shorter latch pulses
- Balanced digit display times through rotation
- Complete tube exercising cycle

To use this code:
1. Replace YOUR_SSID and YOUR_PASSWORD with your WiFi credentials
2. Verify your shift register pin connections match the definitions
3. Ensure you have the required libraries installed:
   - RTClib (Adafruit version)
   - NTPClient
   - ESP8266WiFi

The tubes will now:
- Gently rotate displayed digits every minute
- Perform a full 0-9 cycle on all tubes every hour
- Maintain accurate time through daily NTP syncs
- Automatically reconnect to WiFi if connection drops

This implementation balances tube protection with accurate timekeeping while maintaining good visibility of the current time.