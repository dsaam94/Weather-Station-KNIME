#include "arduino_secrets.h"

#include <Arduino_MKRIoTCarrier.h>
#include <SPI.h>
#include <WiFiNINA.h>
#include <Arduino_JSON.h>
#include "thingProperties.h"
MKRIoTCarrier carrier;
#define NUMPIXELS 5

int refreshCount = 0;
int previousMode = 0;
int modeSelect = 0;
int r = 0, g = 0, b = 0;

char serverAddress[] = "server address to web service";
int port = 443; //port listening on too
int status = WL_IDLE_STATUS;
char buffer[90];
//I copy pasted this from the example in MKR_Arduino
int      head  = 0, tail = -4; // Index of first 'on' and 'off' pixels
uint32_t color = 0xFF0000;      // 'On' color (starts red)

//WifiClient to connect to WiFi. Always ensure to add SSID and password 
//in the script so that device can connect to that specific router only.
//I am not sure it can be made dynamic though
WiFiSSLClient client;

//Initializing client object for communicating with server
//HttpClient client = HttpClient(wifi, serverAddress, port);


void setup() {
  //carrier.Buzzer.sound(8000);
  
  // Initialize serial and wait for port to open:
  Serial.begin(9600);
  // This delay gives the chance to wait for a Serial Monitor without blocking if none is found
  delay(1500);

  // Defined in thingProperties.h
  initProperties();

  // Connect to Arduino IoT Cloud
  ArduinoCloud.begin(ArduinoIoTPreferredConnection);

  /*
     The following function allows you to obtain more information
     related to the state of network and IoT Cloud connection and errors
     the higher number the more granular information you’ll get.
     The default is 0 (only errors).
     Maximum is 4
  */
  setDebugMessageLevel(2);
  ArduinoCloud.printDebugInfo();


  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    // don't continue
    while (true);
    
  }
  
    //while (ArduinoCloud.connected() != 1) {
  //    ArduinoCloud.update();
  //    delay(500);
  //}
  
  if (WiFi.status() != WL_CONNECTED) {
    //pretty much self explanatory I guess
    tryConnect();
    carrier.display.print("Connected!!");
  }

  Serial.println("You're connected to the network");
  //printCurrentNet();
  //printWiFiData();

  delay(500);
  CARRIER_CASE = true;
  carrier.begin();
  carrier.display.setRotation(0);
  carrier.display.fillScreen(ST77XX_BLACK);
  carrier.display.setTextColor(ST77XX_WHITE);
  carrier.display.setTextSize(3);

  carrier.display.setCursor(60, 80);
  carrier.display.print("Weather");
  carrier.display.setCursor(60, 120);
  carrier.display.print("Station");
  delay(2000);
  carrier.display.fillScreen(ST77XX_BLACK);
  carrier.display.setTextColor(ST77XX_WHITE);
  carrier.display.setTextSize(2);

  carrier.display.setCursor(70, 80);
  carrier.display.print("Connected");
  carrier.display.setCursor(50, 110);
  carrier.display.print("To IoT Cloud");
  //delay(2000);
}

//function to attempt to connect to WIFI incase no connection is established
void tryConnect() {
  // attempt to connect to WiFi network:
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to WPA SSID: ");
    Serial.println(SSID);
    // Connect to WPA/WPA2 network:
    status = WiFi.begin(SSID, PASS);
     if (client.connect(serverAddress, 443)) {
        Serial.println("connected to server");
        
      }
    // wait 5 seconds for connection:
    delay(5000);
  }
  Serial.println(status);
  printCurrentNet();
  printWiFiData();
}

void loop() {

  ArduinoCloud.update();
  carrier.Buttons.update();

  
  if(WiFi.status() == WL_DISCONNECTED || WiFi.status() == WL_CONNECTION_LOST || WiFi.status() == WL_CONNECT_FAILED){
      tryConnect();
  } 
  // Your code here
  Serial.println("WiFi Status: ");
  Serial.println(WiFi.status());
  


  while (!carrier.Light.colorAvailable()) {
    delay(5);
  }
  int none;
  carrier.Light.readColor(r, g, b);
  
  //I am using MKR_Carrier library to read sensor readings of the board
  temperature = carrier.Env.readTemperature();
  humidity = carrier.Env.readHumidity();
  pressure = carrier.Pressure.readPressure();

  // in case the sensor value is outside the range seen during calibration
  temperature = constrain(temperature, tempSensorMin, tempSensorMax);

  // apply the calibration to the sensor reading. The target low and high values were used
  // based on prior knowledge of last years minimum and maximum temperature of 2020 during Jun-Aug
  temperature = map(temperature, tempSensorMin, tempSensorMax, 15, 29);

  // print each of the sensor values
  Serial.print("Temperature = ");
  Serial.print(temperature);

  Serial.print("Humidity = ");
  Serial.print(humidity);
  

 
//connect WiFISSLClient to establish connection with web service
if (client.connect(serverAddress, 443)) {
        Serial.println("From Loop");
        // Make a HTTP request:
        
        
        String myurl = "/knime/rest/v4/repository/Users/ali.marvi/REST_API_ARDUINO/REST_API_2/updating_API_for_sheets:execution?reading_input-1=%7B%22Humidity%22%3A"
                        + String(humidity) + "%2C%22Temperature%22%3A" + temperature + "%2C%22Pressure%22%3A" + pressure + "%7D";
        
        Serial.println(myurl);
        client.println("GET " + myurl + " HTTP/1.1");
        client.println("Host: xxxx.knime.com");
        client.println("Authorization: Basic ###################/");
        client.println("Connection: close");
        client.println();
      }

//wait for 5 seconds for connection to establish
   delay(5000);
   
   
//read response in serial monitor   
   int index = 0;    
   while (client.available()) {
    char c = client.read();
    Serial.write(c);
    if(index < 13)
    {
      buffer[index] = c;
      index++;
      buffer[index] = '\0';
    }
  }

//blink green color in case status received is 200  otherwise red
  if(buffer!= nullptr){
   Serial.println(buffer);
  blinkLEDStatus(buffer);   
  }


//toggle Board screen with each specific value
  if(carrier.Button0.onTouchDown()) {
    modeSelect = 0;
  }
  else if(carrier.Button1.onTouchDown()) {
    modeSelect = 1;
  }
  else if(carrier.Button2.onTouchDown()) {
    modeSelect = 2;
  }
  else if(carrier.Button3.onTouchDown()) {
    modeSelect = 3;
  }
  else if(carrier.Button4.onTouchDown()) {
    modeSelect = 4;
  }

  if (modeSelect != previousMode) {
    updateDisplay();
    previousMode = modeSelect;
    refreshCount = 0;
  }
  else if (refreshCount >= 50) {
    updateDisplay();
    refreshCount = 0;
  }

  refreshCount++;
  
  delay(60000);
}


void updateDisplay () {
  if (modeSelect == 0) {
    carrier.display.fillScreen(ST77XX_RED);
    carrier.display.setTextColor(ST77XX_WHITE);
    carrier.display.setTextSize(4);

    carrier.display.setCursor(70, 50);
    carrier.display.print("Temp:");
    carrier.display.setCursor(40, 110);
    carrier.display.print(temperature);
    carrier.display.print(" C");
  }
  else if (modeSelect == 1) {
    carrier.display.fillScreen(ST77XX_BLUE);
    carrier.display.setTextColor(ST77XX_WHITE);
    carrier.display.setTextSize(4);

    carrier.display.setCursor(70, 50);
    carrier.display.print("Humi:");
    carrier.display.setCursor(40, 110);
    carrier.display.print(humidity);
    carrier.display.print(" %");
  }
  else if (modeSelect == 2) {
    carrier.display.fillScreen(ST77XX_GREEN);
    carrier.display.setTextColor(ST77XX_BLACK);
    carrier.display.setTextSize(4);

    carrier.display.setCursor(60, 60);
    carrier.display.print("Light:");
    carrier.display.setCursor(80, 120);
    carrier.display.print(light);
  }
  else if (modeSelect == 3) {
    carrier.display.fillScreen(ST77XX_CYAN);
    carrier.display.setTextColor(ST77XX_BLACK);
    carrier.display.setTextSize(4);

    carrier.display.setCursor(60, 60);
    carrier.display.print("Press:");
    carrier.display.setCursor(20, 110);
    carrier.display.print(pressure);
    carrier.display.print(" Pa");
  }
  else {
    carrier.display.fillScreen(ST77XX_BLACK);
  }
}




void printWiFiData() {
  // print your board's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP address : ");
  Serial.println(ip);

  Serial.print("Subnet mask: ");
  Serial.println((IPAddress)WiFi.subnetMask());

  Serial.print("Gateway IP : ");
  Serial.println((IPAddress)WiFi.gatewayIP());

  // print your MAC address:
  byte mac[6];
  WiFi.macAddress(mac);
  Serial.print("MAC address: ");
  printMacAddress(mac);
}

void printCurrentNet() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print the MAC address of the router you're attached to:
  byte bssid[6];
  WiFi.BSSID(bssid);
  Serial.print("BSSID: ");
  printMacAddress(bssid);
  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI): ");
  Serial.println(rssi);

  // print the encryption type:
  byte encryption = WiFi.encryptionType();
  Serial.print("Encryption Type: ");
  Serial.println(encryption, HEX);
  Serial.println();
}

void printMacAddress(byte mac[]) {
  for (int i = 5; i >= 0; i--) {
    if (mac[i] < 16) {
      Serial.print("0");
    }
    Serial.print(mac[i], HEX);
    if (i > 0) {
      Serial.print(":");
    }
  }
  Serial.println();
}

void shashkay() {
  carrier.leds.setPixelColor(head, color); // 'On' pixel at head
  carrier.leds.setPixelColor(tail, 0);     // 'Off' pixel at tail
  carrier.leds.show();                     // Refresh strip
  delay(20);                        // Pause 20 milliseconds (~50 FPS)


  if (++head >= NUMPIXELS) {        // Increment head index.  Off end of strip?
    head = 0;                       //  Yes, reset head index to start
    if ((color >>= 8) == 0)         //  Next color (R->G->B) ... past blue now?
      color = 0xFF0000;             //   Yes, reset to red
  }
  if (++tail >= NUMPIXELS) tail = 0; // Increment, reset tail index
}

void blinkLEDStatus(char sub[]){
  char sub_string[sizeof(sub)] = {sub[9], sub[10], sub[11]};
  Serial.println(String(sub_string));
  if(String(sub_string) == "200"){
    carrier.leds.setPixelColor(0,  20 ,  0 , 0);
    carrier.leds.show(); 
  }
  else{
    carrier.leds.setPixelColor(0, 0 ,  20 , 0 );
    carrier.leds.show(); 
  }
  
}


