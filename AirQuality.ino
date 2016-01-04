/***************************************************
  Air Quality Monitoring
  Uses the Arduino Uno, Ethernet Shield, and Adafruit 1.8"
  display shield http://www.adafruit.com/products/802

  Check out the links above for our tutorials and wiring diagrams

  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  Based on code written by Limor Fried/Ladyada for Adafruit Industries.
  MIT license, all text above must be included in any redistribution
 ****************************************************/
#include <Ethernet.h>
#include <SPI.h>      

#include <Adafruit_GFX.h>      // Core graphics library
#include <Adafruit_HX8340B.h>

// Display Color definitions
#define BLACK           0x0000
#define BLUE            0x001F
#define RED             0xF800
#define GREEN           0x07E0
#define CYAN            0x07FF
#define MAGENTA         0xF81F
#define YELLOW          0xFFE0  
#define WHITE           0xFFFF
#define ORANGE          0xF500

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED }; // Ethernet shield MAC address

char server[] = "www.airnowapi.org";    
IPAddress ip(192, 168, 0, 178);  // select for your home network - it will try DHCP first though

EthernetClient client;

uint8_t lineCount = 0;

// TFT display will NOT use hardware SPI due to the SPI of the Ethernet shield
// SO other pins are used.  This makes the display a bit slow
#define SD_CS     4   // Chip select line for SD card
#define TFT_CS    9   // Chip select line for TFT display
#define TFT_SCLK  6   // set these to be whatever pins you like!
#define TFT_MOSI  7   // set these to be whatever pins you like!
#define TFT_RST   8   // Reset line for TFT (0 = reset on Arduino reset)
Adafruit_HX8340B tft(TFT_MOSI, TFT_SCLK, TFT_RST, TFT_CS);

void setup(void) {

  Serial.begin(115200);
  Serial.println(F("Air Quality Monitor"));
    
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    // no point in carrying on, so do nothing forevermore:
    // try to congifure using IP address instead of DHCP:
    Ethernet.begin(mac, ip);
  }
  // Give the Ethernet shield a second to initialize:
  delay(1000);
  Serial.println("Ethernet connecting...");
  // if you get a connection, report back via serial:
  if (client.connect(server, 80)) {
    Serial.println("Ethernet connected");
    // Make a HTTP request
    // Change the latitude/longitude for your location
    // You need your own API key from www.airnow.org pasted into the string below
    // If you will go mobile, get lat/long from GPS or from your outward IP address
    //   from a service like http://www.freegeoip.org/
    client.println("GET /aq/forecast/latLong/?format=text/csv&latitude=38.8&longitude=-77.3&distance=25&API_KEY=FE62B688-EE27-4214-8BD8-9903E1AC5881 HTTP/1.1");
    client.println("Host: www.airnowapi.org");
    client.println("Connection: close");
    client.println();
  }
  else {
    // If you didn't get a connection to the server:
    Serial.println("Ethernet connection failed");
  }
  client.setTimeout(1000);
  
  tft.begin();           // Initialize  TFT
  tft.setRotation(1);    // Landscape display
}

void loop() {
  char buffer[160];      // The HTTP read buffer - the maximum line returned by the API is 158 chars
  char partInfo[4][10];  // The air quality values are placed in 4 strings of length 10 characters
  uint8_t tokenCount, valueCount;
  char *bufValue;
  char *bufPtr;

  if (client.available()) {
    byte numChar = client.readBytesUntil((char)0x0a,buffer,159);  // Read until a line feed character
    buffer[numChar]='\0';
    lineCount = lineCount + 1;
    if(lineCount == 11) {  // Parse first record
      Serial.print("-> ");
      Serial.println(buffer);
      tokenCount=0;
      valueCount=0;
      bufPtr = strtok(buffer,",");
      while(bufPtr != NULL && valueCount < 10) {
        if(valueCount > 5) {
          strcpy(partInfo[tokenCount], bufPtr+1);
          tokenCount++;
        }
        bufPtr = strtok(NULL,",");
        valueCount++;
      }
      for( uint8_t i=0; i<4; i++) {
        for( uint8_t j=0; j<10; j++) {
          if(partInfo[i][j] == '"') {  // the second quotes is the end of the string we want
            partInfo[i][j] = '\0';     //   so replace it with the C null end of string character
          }
        }
        Serial.println(partInfo[i]);
      }
    } else {
      Serial.print(lineCount);
      Serial.print(": ");
      Serial.println(buffer);
    }
  }

  if (!client.connected()) {
    Serial.println();
    Serial.println("Ethernet disconnecting.");
    client.stop();
    /* process the values */
    uint32_t colorAQI = AQI2hex(atoi(partInfo[1]));
    tft.fillScreen(colorAQI);
    //Serial.print("Color set: ");
    //Serial.println(colorAQI,HEX);
    drawtext("Air Quality Today", BLACK, 2, 10, 10);
    drawtext(partInfo[3], WHITE, 3, 40, 40);
    drawtext(partInfo[1], WHITE, 3, 85, 80);
    drawtext("Type:", WHITE, 3, 20, 120);
    drawtext(partInfo[0], WHITE, 3, 110, 120);
    for(;;)
      ; // Infinite loop, press reset button to get daily reading
        // It's best to have a real time clock to pull the value at midnight
        // Perhaps a GPS shield would get the time and the lat/long but be sure 
        // to deconflict the data pins if necessary.
  }
}

uint32_t AQI2hex(uint16_t AQI) {   // see color tablee for mandated color coding
  if(AQI <=  50) return(GREEN); // Green
  if(AQI <= 100) return(YELLOW); // Yellow
  if(AQI <= 150) return(ORANGE); // Orange
  if(AQI <= 200) return(RED); // Red
  if(AQI <= 300) return(MAGENTA); // Purple
  return(0x8000); // Maroon
}

void drawtext(char *text, uint16_t color, uint8_t tsize, uint8_t x, uint8_t y) {
  tft.setCursor(x, y);
  tft.setTextSize(tsize);
  tft.setTextColor(color);
  tft.print(text);
}
   
