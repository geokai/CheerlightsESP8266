/* Cheerlights on the ESP8266, tested on Wemos D1 mini V2.
   Russ Winch - December 2016
   https://github.com/russwinch/CheerlightsESP8266

   Based on the ESP8226 WifiClient example and Adafruit Neopixels examples
   Using a code snippet to identify the hex value from bbx10: https://github.com/bbx10/esp-cheer-lights/blob/master/esp-cheer-lights/esp-cheer-lights.ino
   Implementation of the suggested delay for slow connections from Frogmore42: https://github.com/esp8266/Arduino/issues/722


  ****implement offline functionality

*/

#include <ESP8266WiFi.h>
#include <Adafruit_NeoPixel.h>

// Which data pin on the Arduino is connected to the NeoPixels?
#define PIN D3

// How many NeoPixels are attached to the Arduino?
#define NUMPIXELS 1

// When we setup the NeoPixel library, we tell it how many pixels, and which pin to use to send signals.
// Note that for older NeoPixel strips you might need to change the third parameter--see the strandtest
// example for more information on possible values.
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

// IMPORTANT: To reduce NeoPixel burnout risk, add 1000 uF capacitor across
// pixel power leads, add 300 - 500 Ohm resistor on first pixel's data input
// and minimize distance between Arduino and first pixel.  Avoid connecting
// on a live circuit...if you must, connect GND first.

// Wifi settings
const char ssid[]     = "aaaaaaaaaa";
const char password[] = "bbbbbbbbbb";

// Location of the Cheerlights API
const char host[] = "api.thingspeak.com";
const int httpPort = 80;

// Delay between updates (in miliseconds). 5-10 seconds recommended
int wait = 5000;

int RGB[3]; //current value of leds

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(9600);
  pixels.begin(); // This initializes the NeoPixel library.
  WiFi.begin(ssid, password); //connect to wifi
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (millis() > 60000) { //give up if not connected after 60 seconds
      confirm(false);
      return; //**********implement offline functionality here
    }
  }
  confirm(true); //confirm wifi is connected
}

void loop() {
  getColour();
  delay(wait); //wait between updates
}

void getColour() {
  Serial.print("connecting to ");
  Serial.println(host);

  // Use WiFiClient class to create TCP connections
  WiFiClient client;
  const int httpPort = 80;
  if (!client.connect(host, httpPort)) {
    Serial.println("connection failed");
    return;
  }

  // We now create a URI for the request
  String url = "/channels/1417/field/2/last.txt";

  // This will send the request to the server
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "Connection: close\r\n\r\n");
  // Read all the lines of the reply from server and scan for hex color
  unsigned long colour;
  unsigned long startTime = millis();
  unsigned long httpResponseTimeOut = 10000; // 10 sec
  while (client.connected() && ((millis() - startTime) < httpResponseTimeOut)) {
    if (client.available()) {
      String line = client.readStringUntil('\n');
      Serial.println(line);
      if (line[0] == '#') { //&& (line.length() == 8)) {
        Serial.println(line);
        colour = strtoul(line.c_str() + 1, NULL, 16);
        Serial.println(colour);
        digitalWrite(LED_BUILTIN, LOW);
        delay(10);
        digitalWrite(LED_BUILTIN, HIGH);
      }
    }
    else {
      Serial.print(".");
      delay(100);
    }
  }
  int targetRGB[3];
  targetRGB[0] = (colour & 0xFF0000) >> 16;
  targetRGB[1] = (colour & 0x00FF00) >> 8;
  targetRGB[2] = (colour & 0x0000FF);

  while (!compare(RGB, targetRGB)) {
    for (int i = 0; i < 3; i++) {
      if (RGB[i] < targetRGB[i]) RGB[i]++;
      else if (RGB[i] > targetRGB[i]) RGB[i]--;
    }
    updateColour(RGB[0], RGB[1], RGB[2]);
    delay(30); //smooth the transition
  }
}


void updateColour (int r, int g, int b) {
  for (int i = 0; i < NUMPIXELS; i++) { // For a set of NeoPixels the first NeoPixel is 0, second is 1, all the way up to the count of pixels minus one.
    pixels.setPixelColor(i, pixels.Color(r, g, b)); // pixels.Color takes RGB values, from 0,0,0 up to 255,255,255
    pixels.show(); // This sends the updated pixel color to the hardware.
  }
}

boolean compare (int *a, int *b) {
  for (int n = 0; n < 3; n++) {
    if (a[n] != b[n]) return false;
  }
  return true;
}

void confirm (boolean success) {
  int del = 200;
  for (int c = 0; c < 5; c++) {
    if (success) {
      updateColour(0, 255, 0);
      digitalWrite(LED_BUILTIN, LOW);
    }
    else updateColour(255, 0, 0);
    delay(del);
    updateColour(0, 0, 0);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(del);
  }
}

