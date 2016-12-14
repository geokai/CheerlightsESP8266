/* Cheerlights on the ESP8266, tested on Wemos D1 mini V2.
   Russ Winch - December 2016
   https://github.com/russwinch/CheerlightsESP8266

   - Based on the ESP8226 WifiClient example and Adafruit Neopixels examples
   - Using a code snippet to identify the hex value from bbx10: https://github.com/bbx10/esp-cheer-lights/blob/master/esp-cheer-lights/esp-cheer-lights.ino
   - Implementation of the suggested delay for slow connections from Frogmore42: https://github.com/esp8266/Arduino/issues/722
   - Cheerlights API documentation: http://cheerlights.com/cheerlights-api/
*/

#include <ESP8266WiFi.h>
#include <Adafruit_NeoPixel.h>

// Which data pin on the Arduino is connected to the NeoPixels?
#define PIN D3

// How many NeoPixels are attached to the Arduino?
#define NUMPIXELS 4

// When we setup the NeoPixel library, we tell it how many pixels, and which pin to use to send signals.
// Note that for older NeoPixel strips you might need to change the third parameter--see the strandtest
// example for more information on possible values.
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

// IMPORTANT: To reduce NeoPixel burnout risk, add 1000 uF capacitor across
// pixel power leads, add 300 - 500 Ohm resistor on first pixel's data input
// and minimize distance between Arduino and first pixel.  Avoid connecting
// on a live circuit...if you must, connect GND first.

// Wifi settings
const char ssid[]     = "aaaaaaaaaaa";
const char password[] = "bbbbbbbbbbb";

// Location of the Cheerlights API
const char host[] = "api.thingspeak.com";
const int httpPort = 80;

// Delay between updates (in miliseconds). 5+ seconds recommended
int wait = 10 * 1000;

//cheerlights colours for offline mode
unsigned long colours[11] = {0xFF0000, 0x008000, 0x0000FF, 0x00FFFF, 0xFFFFFF, 0xFDF5E6, 0x800080, 0xFF00FF, 0xFFFF00, 0xFFA500, 0xFFC0CB};
int offlineCol; //offline selected colour
boolean online; //mode selector
int RGB[3]; //current value of leds
int targetRGB[3]; //target value of leds

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(9600);
  pixels.begin(); // This initializes the NeoPixel library.
  randomSeed(analogRead(A0)); //initialise random seed on analog pin 0
  online = connectWifi(); //connect to Wifi and store result in online variable
  confirm(online); //confirm wifi connection
}

void loop() {
  if (online) { //online mode
    getColour();
    while (!compare(RGB, targetRGB)) updateColour(); //while we are out of sync with cheerlights
    delay(wait); //wait between updates
  } else {
    offlineColour();
    while (!compare(RGB, targetRGB)) updateColour(); //while we are out of sync with cheerlights
    int randDelay = random(5); //random number of minutes to wait between updates
    Serial.print("Waiting for ");
    Serial.print(randDelay);
    Serial.println(" mins");
    delay(randDelay * 60000); //convert to milliseconds
  }
  if (WiFi.status() != WL_CONNECTED) online = connectWifi(); //if connection is dropped, attempt to re-connect or go into offline mode
}

bool connectWifi() {
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  
  int attempts = 0;
  WiFi.begin(ssid, password); //connect to wifi
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
    attempts++;
    if (attempts >= 30) return false; //give up if not connected after 30 seconds
  }
  Serial.println();
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  return true;
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
  unsigned long httpResponseTimeOut = 10000; // 10 sec timeout
  while (client.connected() && ((millis() - startTime) < httpResponseTimeOut)) {
    if (client.available()) {
      String line = client.readStringUntil('\n');
      Serial.println(line);
      if (line[0] == '#') { //looking for # at the start of the line
        Serial.println(line);
        colour = strtoul(line.c_str() + 1, NULL, 16);
        Serial.println(colour);
        digitalWrite(LED_BUILTIN, LOW);
        delay(10);
        digitalWrite(LED_BUILTIN, HIGH);
      }
    } else {
      Serial.print("."); //waiting for response
      delay(100);
    }
  }
  decodeHex(colour, targetRGB); //convert hex values
}

void offlineColour() {
  offlineCol = random(10);
  Serial.println(colours[offlineCol], HEX);
  decodeHex(colours[offlineCol], targetRGB);
}

void decodeHex(unsigned long h, int *out) {
  out[0] = (h & 0xFF0000) >> 16;
  out[1] = (h & 0x00FF00) >> 8;
  out[2] = (h & 0x0000FF);
}

void updateColour() {
  for (int i = 0; i < 3; i++) { //loop through red, green, blue
    if (RGB[i] < targetRGB[i]) RGB[i]++; //increase if below target
    else if (RGB[i] > targetRGB[i]) RGB[i]--; //decrease if above target
  }
  writeColour(RGB[0], RGB[1], RGB[2]);
  delay(30); //smooth the transition
}

void writeColour (int r, int g, int b) {
  for (int i = 0; i < NUMPIXELS; i++) { // For a set of NeoPixels the first NeoPixel is 0, second is 1, all the way up to the count of pixels minus one.
    pixels.setPixelColor(i, pixels.Color(r, g, b)); // pixels.Color takes RGB values, from 0,0,0 up to 255,255,255
    pixels.show(); // This sends the updated pixel color to the hardware.
  }
}

boolean compare (int *a, int *b) {
  for (int n = 0; n < 3; n++) {
    if (a[n] != b[n]) return false;
  }
  return true; //else
}

//flash leds to confirm result of wifi connection
void confirm (boolean success) {
  int del = 200;
  for (int c = 0; c < 5; c++) { //flash 5 times
    if (success) {
      writeColour(0, 255, 0); //green
      digitalWrite(LED_BUILTIN, LOW); //built in led on
    }
    else writeColour(255, 0, 0); //red
    delay(del);
    writeColour(0, 0, 0); //leds off
    digitalWrite(LED_BUILTIN, HIGH); //built in led off
    delay(del);
  }
}

