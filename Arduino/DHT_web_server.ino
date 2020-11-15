#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_BME280.h>
#include "DHT.h"
#include <Ticker.h>

#define DHTTYPE DHT11                   // DHT 11
uint8_t DHTPin = 5;                     // GPIO05 ( D2 )


//const char* ssid = "PostOffice-l33t"; // Enter SSID here
//const char* password = "POl33tPO";    //Enter Password here
IPAddress local_IP(192, 168, 1, 239);   // Set your Static IP address
IPAddress gateway(192, 168, 1, 1);      // Set your Gateway IP address
IPAddress subnet(255, 255, 255, 0);     // Set yout Subnet Mask
IPAddress primaryDNS(8, 8, 8, 8);       //optional
IPAddress secondaryDNS(8, 8, 4, 4);     //optional

Adafruit_BME280 bme; // use I2C interface
Adafruit_Sensor *bme_temp = bme.getTemperatureSensor();
Adafruit_Sensor *bme_pressure = bme.getPressureSensor();
Adafruit_Sensor *bme_humidity = bme.getHumiditySensor();


char FW_version[] = "hello";

float RAW_Temperature;
float RAW_Humidity;

bool buttonState = 0;
bool Mode = 0;

Ticker ticker;
ESP8266WebServer server(80);
DHT dht(DHTPin, DHTTYPE);

// SETUP ///////////////////////////////////////////////////////

void setup() {
  pinMode(DHTPin, INPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  dht.begin();
  Serial.begin(9600);
  pinMode(BUILTIN_LED, OUTPUT);
  ticker.attach(0.6, tick);

  if (!bme.begin()) {
    Serial.println(F("Could not find a valid BME280 sensor, check wiring!"));
    while (1) delay(10);
  }

  bme_temp->printSensorDetails();
  bme_pressure->printSensorDetails();
  bme_humidity->printSensorDetails();

  WiFiManager wifiManager;
  //reset settings - for testing
  //wifiManager.resetSettings();

  wifiManager.setDebugOutput(false);        // Disable if you want to disable debug
  wifiManager.setAPCallback(configModeCallback);

  if (!wifiManager.autoConnect()) {
    Serial.println("failed to connect and hit timeout");
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(1000);
  }
  Serial.println("connected...yeey :)");
  ticker.detach();

  // Configures static IP address
  if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS))
  {
    Serial.println("STA Failed to configure");
  }

  //Serial.print("Connecting to : "); Serial.println(ssid);
  //WiFi.begin(ssid, password);
  //while (WiFi.status() != WL_CONNECTED) {
  //  delay(500);
  //  Serial.print(".");
  //}
  Serial.println("");
  Serial.println("WiFi connected..!");
  Serial.print("Got IP: ");  Serial.println(WiFi.localIP());
  Serial.print("MAC Address: ");  Serial.println(WiFi.macAddress());

  server.on("/", handle_OnConnect);
  server.on("/metrics", handleMetrics);
  server.on("/temperature", TemperatureJSON);
  server.on("/humidity", HumidityJSON);
  server.on("/pressure", PressureJSON);
  server.on("/ledstatus", Ledstatus);
  server.on("/ledtoggle", Ledtoggle);
  server.onNotFound(handle_NotFound);

  server.begin();
  Serial.println("HTTP server started");
  String MACADD = WiFi.macAddress();
  MACADD.replace(":", "_");

}

// LOOP ///////////////////////////////////////////////////////

void loop() {
  server.handleClient();
  float raw_h = dht.readHumidity();
  float raw_t = dht.readTemperature();
  float raw_f = dht.readTemperature(true);

  sensors_event_t temp_event, pressure_event, humidity_event;
  bme_temp->getEvent(&temp_event);
  bme_pressure->getEvent(&pressure_event);
  bme_humidity->getEvent(&humidity_event);

  digitalWrite(LED_BUILTIN, Mode );
  delay(950);
  digitalWrite(LED_BUILTIN, 1);
  delay(10);
  Serial.print("T: ");
  Serial.print(temp_event.temperature);
  Serial.print("C, ");
  Serial.print(", H: ");
  Serial.print(humidity_event.relative_humidity);
  Serial.print(" %, P: ");
  Serial.print(pressure_event.pressure);
  Serial.println(" hPa");
  return;
}

// HANDLERS & Function Declerations ////////////////////////////

void handleMetrics() {
  sensors_event_t temp_event, pressure_event, humidity_event;
  bme_temp->getEvent(&temp_event);
  bme_pressure->getEvent(&pressure_event);
  bme_humidity->getEvent(&humidity_event);

  String MACADD = WiFi.macAddress();
  MACADD.replace(":", "_");
  String metrics = "esp_";
  metrics += MACADD ;
  metrics += "_temperature{handler=\"/tempeture\"} ";
  metrics += temp_event.temperature ;
  metrics += "\n";
  metrics += "esp_";
  metrics += MACADD ;
  metrics += "_humidity{handler=\"/humidity\"} ";
  metrics += humidity_event.relative_humidity ;
  metrics += "\n";
  metrics += "esp_";
  metrics += MACADD ;
  metrics += "_pressure{handler=\"/pressure\"} ";
  metrics += pressure_event.pressure ;
  server.send(200, "text/plain", metrics);
}

void TemperatureJSON() {
  sensors_event_t temp_event, pressure_event, humidity_event;
  bme_temp->getEvent(&temp_event);
  bme_pressure->getEvent(&pressure_event);
  bme_humidity->getEvent(&humidity_event);

  String metrics = "{ \"Temperature\" : \"";
  metrics += temp_event.temperature ;
  metrics += "\" }";
  server.send(200, "text/plain", metrics);
}

void PressureJSON() {
  sensors_event_t temp_event, pressure_event, humidity_event;
  bme_temp->getEvent(&temp_event);
  bme_pressure->getEvent(&pressure_event);
  bme_humidity->getEvent(&humidity_event);

  String metrics = "{ \"Pressure\" : \"";
  metrics += pressure_event.pressure ;
  metrics += "\" }";
  server.send(200, "text/plain", metrics);
}

void HumidityJSON() {
  sensors_event_t temp_event, pressure_event, humidity_event;
  bme_temp->getEvent(&temp_event);
  bme_pressure->getEvent(&pressure_event);
  bme_humidity->getEvent(&humidity_event);

  String metrics = "{ \"Humidity\" : \"";
  metrics += humidity_event.relative_humidity ;
  metrics += "\" }";
  server.send(200, "text/plain", metrics);
}

void Ledtoggle() {
  if (!buttonState)
  {
    buttonState = true;
    Mode = !Mode;
    delay(1000);
  }
  else buttonState = false;
  String metrics = "";
  metrics += "{ OK }";
  server.send(200, "text/plain", metrics);
}

void Ledstatus() {
  String metrics = "";
  metrics += "{ \"LED_state\" : \"";
  if ( !Mode ) {
    metrics += "Enabled" ;
  }
  if ( Mode ) {
    metrics += "Disabled" ;
  }
  metrics += "\" }";
  server.send(200, "text/plain", metrics);
}

void handle_OnConnect() {
  sensors_event_t temp_event, pressure_event, humidity_event;
  bme_temp->getEvent(&temp_event);
  bme_pressure->getEvent(&pressure_event);
  bme_humidity->getEvent(&humidity_event);
  server.send(200, "text/html", SendHTML(temp_event.temperature, humidity_event.relative_humidity, pressure_event.pressure));
}

void handle_NotFound() {
  server.send(404, "text/plain", "Not found");
}

String SendHTML(float Temperaturestat, float Humiditystat, float Preassurestat) {
  String MACADD = WiFi.macAddress();
  MACADD.replace(":", "_");
  String ptr = "<!DOCTYPE html> <html>\n";
  ptr += "<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n";
  ptr += "<link href=\"https://fonts.googleapis.com/css?family=Open+Sans:300,400,600\" rel=\"stylesheet\">\n";
  ptr += "<link rel=\"stylesheet\" href=\"https://cdnjs.cloudflare.com/ajax/libs/font-awesome/4.7.0/css/font-awesome.min.css\">";
  ptr += "<title>ESP8266 Sensor</title>\n";
  ptr += "  <meta http-equiv=\"refresh\" content=\"5\">";
  ptr += "<style>html { font-family: 'Open Sans', sans-serif; display: block; margin: 0px auto; text-align: center;color: #333333;}\n";
  ptr += "body{margin-top: 50px;}\n";
  ptr += "h1 {margin: 50px auto 30px;}\n";
  ptr += ".side-by-side{display: inline-block;vertical-align: middle;position: relative;}\n";

  ptr += ".humidity-icon{background-color: #3498db;width: 30px;height: 30px;border-radius: 50%;line-height: 36px;}\n";
  ptr += ".humidity-text{font-weight: 600;padding-left: 15px;font-size: 19px;width: 160px;text-align: left;}\n";
  ptr += ".humidity{font-weight: 300;font-size: 60px;color: #3498db;}\n";

  ptr += ".temperature-icon{background-color: #f39c12;width: 30px;height: 30px;border-radius: 50%;line-height: 40px;}\n";
  ptr += ".temperature-text{font-weight: 600;padding-left: 15px;font-size: 19px;width: 160px;text-align: left;}\n";
  ptr += ".temperature{font-weight: 300;font-size: 60px;color: #f39c12;}\n";

  ptr += ".pressure-icon{background-color: #f39c12;width: 30px;height: 30px;border-radius: 50%;line-height: 40px;}\n";
  ptr += ".pressure-text{font-weight: 600;padding-left: 15px;font-size: 19px;width: 160px;text-align: left;}\n";
  ptr += ".pressure{font-weight: 300;font-size: 60px;color: #f39c12;}\n";

  ptr += ".led-icon{background-color: #f39c12;width: 30px;height: 30px;border-radius: 50%;line-height: 40px;}\n";
  ptr += ".led-text{font-weight: 600;padding-left: 15px;font-size: 19px;width: 160px;text-align: left;}\n";
  ptr += ".led{font-weight: 300;font-size: 60px;color: #f39c12;}\n";

  ptr += ".superscript{font-size: 17px;font-weight: 600;position: absolute;right: -20px;top: 15px;}\n";

  ptr += ".data{padding: 10px;}\n";
  ptr += "</style>\n";
  ptr += "</head>\n";
  ptr += "<body>\n";
  ptr += "<div id=\"webpage\">\n";
  ptr += "<h1>ESP8266 Sensor</h1>\n";
  ptr += "<div class=\"data\">\n";
  ptr += "<div class=\"side-by-side temperature-icon\">\n";
  ptr += "<svg version=\"1.1\" id=\"Layer_1\" xmlns=\"http://www.w3.org/2000/svg\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" x=\"0px\" y=\"0px\"\n";
  ptr += "width=\"9.915px\" height=\"22px\" viewBox=\"0 0 9.915 22\" enable-background=\"new 0 0 9.915 22\" xml:space=\"preserve\">\n";
  ptr += "<path fill=\"#FFFFFF\" d=\"M3.498,0.53c0.377-0.331,0.877-0.501,1.374-0.527C5.697-0.04,6.522,0.421,6.924,1.142\n";
  ptr += "c0.237,0.399,0.315,0.871,0.311,1.33C7.229,5.856,7.245,9.24,7.227,12.625c1.019,0.539,1.855,1.424,2.301,2.491\n";
  ptr += "c0.491,1.163,0.518,2.514,0.062,3.693c-0.414,1.102-1.24,2.038-2.276,2.594c-1.056,0.583-2.331,0.743-3.501,0.463\n";
  ptr += "c-1.417-0.323-2.659-1.314-3.3-2.617C0.014,18.26-0.115,17.104,0.1,16.022c0.296-1.443,1.274-2.717,2.58-3.394\n";
  ptr += "c0.013-3.44,0-6.881,0.007-10.322C2.674,1.634,2.974,0.955,3.498,0.53z\"/>\n";
  ptr += "</svg>\n";
  ptr += "</div>\n";
  ptr += "<div class=\"side-by-side temperature-text\">Temperature</div>\n";
  ptr += "<div class=\"side-by-side temperature\">";
  ptr += (int)Temperaturestat;
  ptr += "<span class=\"superscript\"> C</span></div>\n";
  ptr += "</div>\n";
  ptr += "<div class=\"data\">\n";
  ptr += "<div class=\"side-by-side humidity-icon\">\n";
  ptr += "<svg version=\"1.1\" id=\"Layer_2\" xmlns=\"http://www.w3.org/2000/svg\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" x=\"0px\" y=\"0px\"\n\"; width=\"12px\" height=\"17.955px\" viewBox=\"0 0 13 17.955\" enable-background=\"new 0 0 13 17.955\" xml:space=\"preserve\">\n";
  ptr += "<path fill=\"#FFFFFF\" d=\"M1.819,6.217C3.139,4.064,6.5,0,6.5,0s3.363,4.064,4.681,6.217c1.793,2.926,2.133,5.05,1.571,7.057\n";
  ptr += "c-0.438,1.574-2.264,4.681-6.252,4.681c-3.988,0-5.813-3.107-6.252-4.681C-0.313,11.267,0.026,9.143,1.819,6.217\"></path>\n";
  ptr += "</svg>\n";
  ptr += "</div>\n";
  ptr += "<div class=\"side-by-side humidity-text\">Humidity</div>\n";
  ptr += "<div class=\"side-by-side humidity\">";
  ptr += (int)Humiditystat;
  ptr += "<span class=\"superscript\"></span></div>\n";
  ptr += "    <sup class=\"units\">%</sup> " ;
  ptr += "  </p> " ;

  ptr += "<div class=\"data\">\n";
  ptr += "<div class=\"side-by-side humidity-icon\">\n";
  ptr += "<i class=\"fas fa-fist-raised\"\"></i>";
  ptr += "</div>\n";
  ptr += "<div class=\"side-by-side humidity-text\">Pressure</div>\n";
  ptr += "<div class=\"side-by-side humidity\">";
  ptr += (int)Preassurestat;
  ptr += "<span class=\"superscript\"></span></div>\n";
  ptr += "    <sup class=\"units\">%</sup> " ;
  ptr += "  </p> " ;

  if ( !Mode ) {
    ptr += "<div class=\"data\">\n";
    ptr += "<div class=\"side-by-side humidity-icon\">\n";
    ptr += "<i class=\"far fa-lightbulb\"\"></i>";
    ptr += "</div>\n";
    ptr += "<div class=\"side-by-side humidity-text\">LED State</div>\n";
    ptr += "<div class=\"side-by-side humidity\">";
    ptr += "On";
    ptr += "<span class=\"superscript\"></span></div>\n";
    ptr += "    <sup class=\"units\"></sup> " ;
  }
  if ( Mode ) {
    ptr += "<div class=\"data\">\n";
    ptr += "<div class=\"side-by-side humidity-icon\">\n";
    ptr += "<i class=\"far fa-lightbulb\"\"></i>";
    ptr += "</div>\n";
    ptr += "<div class=\"side-by-side humidity-text\">LED State</div>\n";
    ptr += "<div class=\"side-by-side humidity\">";
    ptr += "Off";
    ptr += "<span class=\"superscript\"></span></div>\n";
    ptr += "    <sup class=\"units\"></sup> " ;
  }
  ptr += "<hr>";
  ptr += "<p style=\"font-family:\'Lucida Console\', monospace\"> " ;
  ptr += "<h3  style=\"font-family:\'Lucida Console\', monospace\">Other Info</h3> " ;
  ptr += "<h5  style=\"font-family:\'Lucida Console\', monospace\">MAC Address: </h5> " ;
  ptr += WiFi.macAddress();
  //ptr += "<h5  style=\"font-family:\'Lucida Console\', monospace\">IP: </h5>  " ;
  //ptr += String(WiFi.localIP());
  //ptr += "<h5  style=\"font-family:\'Lucida Console\', monospace\">SSID: </h5> " ;
  //ptr += "<h5  style=\"font-family:\'Lucida Console\', monospace\">BSSID: </h5>  " ;
  //ptr += (hex)WiFi.BSSID();
  ptr += "<h5  style=\"font-family:\'Lucida Console\', monospace\">RSSI: </h5>  " ;
  ptr += WiFi.RSSI();
  //ptr += "<h5  style=\"font-family:\'Lucida Console\', monospace\">EncryptionType: </h5> " ;
  //ptr +=  WiFi.encryptionType();
  ptr += "</p>" ;


  ptr += "<h3>HTTP API routes</h3>";
  ptr += "<h5>/</h5>";
  ptr += "<a href=\"./temperature\"><h5>/temperature</h5></a>";
  ptr += "<a href=\"./humidity\"><h5>/humidity</h5></a>";
  ptr += "<a href=\"./pressure\"><h5>/pressure</h5></a>";
  ptr += "<a href=\"./metrics\"><h5>/metrics</h5></a>";
  ptr += "<a href=\"./ledtoggle\"><h5>/ledtoggle</h5></a>";
  ptr += "<a href=\"./ledstatus\"><h5>/ledstatus</h5></a>";

  ptr += "<hr>";
  ptr += "FW Version: ";
  ptr += "1.4.10" ;

  ptr += "</body> " ;
  ptr += "</html>  ";
  return ptr;
}


void tick()
{
  //toggle state
  int state = digitalRead(BUILTIN_LED);  // get the current state of GPIO1 pin
  digitalWrite(BUILTIN_LED, !state);     // set pin to the opposite state
}

//gets called when WiFiManager enters configuration mode
void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  //if you used auto generated SSID, print it
  Serial.println(myWiFiManager->getConfigPortalSSID());
  //entered config mode, make led toggle faster
  ticker.attach(0.2, tick);
}
