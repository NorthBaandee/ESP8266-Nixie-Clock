Auto-updating Nixie Clock based on ESP8266 and DS3231

When switched on this clock connects to the Wifi network defined in the code, pulls the time from the NTP server, compares it to the time stored in the DS3231 and corrects it if necessary. 

There is some code in there for an extended calender with days months and years but  they are not implemented yet. You can see the values it produces on the serial output. Some of it is a bit weird but the hours minutes and seconds are always correct.

The display hardware is just three 74HC595 shift registers and six 74141 bcd to decimal HV converters. These chips are quite rare and it is more likely you will be able to obtain the Russian substitutes K-155ID1 from somewhere like Poland or Bulgaria. For the high voltage power supply I used a MAX 1771 unit identical to this one on ebay https://www.ebay.com.au/itm/285146927359
It was very easy to implement on a bit of stripboard

The DXF file was drawn with LibreCAD. The circuit is divided into categories on different drawing layers which can be switched on and off for visibility.
