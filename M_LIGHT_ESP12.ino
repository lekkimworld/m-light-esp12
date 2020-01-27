#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <ESP8266mDNS.h>

#define DELAY_TURNOFF_AP 300000L        // delay after restart before turning off access point, in milliseconds
#define PIN_RED 14
#define PIN_GREEN 4
#define PIN_BLUE 12
#define PWM_MAX 1024
#define CONFIG_VERSION 1

#define VERSION_NUMBER "20200126T1642"
#define VERSION_LASTCHANGE "Added yield"

ESP8266WebServer server(80);

// define struct to hold wifi configuration
struct { 
  char ssid[20] = "";
  char password[20] = "";
  bool keep_ap_on = false;
} wifi_data;
struct {
  uint8_t version = CONFIG_VERSION;
  unsigned int r = 0;
  unsigned int g = 0;
  unsigned int b = 0;
} configuration;


// *** WEB SERVER
void webHeader(char* buffer, bool back, char* title) {
  strcpy(buffer, "<!DOCTYPE html><html><head><meta name=\"viewport\" content=\"initial-scale=1.0\"><title>M LIGHT</title><link rel=\"stylesheet\" href=\"./styles.css\"></head><body>");
  if (back) strcat(buffer, "<div class=\"position\"><a href=\"./\">Back</a></div>");
  strcat(buffer, "<div class=\"position title\">");
  strcat(buffer, title);
  strcat(buffer, "</div>");
}

void initWebserver() {
  server.on("/", HTTP_GET, webHandle_GetRoot);
  server.on("/rgb", HTTP_POST, webHandle_PostRGBForm);
  server.on("/wificonfig.html", HTTP_GET, webHandle_GetWifiConfig);
  server.on("/wifi", HTTP_POST, webHandle_PostWifiForm);
  server.on("/styles.css", HTTP_GET, webHandle_GetStyles);
  server.onNotFound(webHandle_NotFound);  
  
}

void webHandle_GetRoot() {
  char integer_string[32];
  char response[1024];
  webHeader(response, false, "Lysops&aelig;tning");
  strcat(response, "<div class=\"position menuitem height30\"><a href=\"./wificonfig.html\">Wi-Fi Config.</a></div>");
  strcat(response, "<div class=\"position menuitem\">");
  strcat(response, "<p>");
  
  sprintf(integer_string, "%d", configuration.r);
  strcat(response, "Current red: "); strcat(response, integer_string); strcat(response, "<br/>");
  sprintf(integer_string, "%d", configuration.g);
  strcat(response, "Current green: "); strcat(response, integer_string); strcat(response, "<br/>");
  sprintf(integer_string, "%d", configuration.b);
  strcat(response, "Current blue: "); strcat(response, integer_string); strcat(response, "<br/>");
  strcat(response, "</p>");
  strcat(response, "<form method=\"post\" action=\"/rgb\">");
  strcat(response, "<table border=\"0\">");
  strcat(response, "<tr><td align=\"left\">Red</td><td><input type=\"text\" name=\"r\" autocomplete=\"off\"></input></td></tr>");
  strcat(response, "<tr><td align=\"left\">Green</td><td><input type=\"text\" name=\"g\" autocomplete=\"off\"></input></td></tr>");
  strcat(response, "<tr><td align=\"left\">Blue</td><td><input type=\"text\" name=\"b\" autocomplete=\"off\"></input></td></tr>");
  strcat(response, "<tr><td colspan=\"2\" align=\"right\"><input type=\"submit\"></input></td></tr>");
  strcat(response, "</table></form>");
  strcat(response, "</div>");
  strcat(response, "<div class=\"position footer right\">");
  strcat(response, VERSION_NUMBER);
  strcat(response, "<br/>");
  strcat(response, VERSION_LASTCHANGE);
  strcat(response, "</div>");
  strcat(response, "</body></html>");
  server.send(200, "text/html", response);
}

void webHandle_GetWifiConfig() {
  char response[1024];
  webHeader(response, true, "Wi-Fi Config.");
  strcat(response, "<div class=\"position menuitem\">");
  strcat(response, "<p>");
  strcat(response, "Current SSID: "); strcat(response, wifi_data.ssid); strcat(response, "<br/>");
  strcat(response, "Current Password: "); strncat(response, wifi_data.password, 4); strcat(response, "****<br/>");
  strcat(response, "Keep AP on: "); strcat(response, wifi_data.keep_ap_on ? "Yes" : "No"); strcat(response, "<br/>");
  strcat(response, "Status: "); strcat(response, WiFi.status() == WL_CONNECTED ? "Connected" : "NOT connected");
  strcat(response, "</p>");
  strcat(response, "<form method=\"post\" action=\"/wifi\">");
  strcat(response, "<table border=\"0\">");
  strcat(response, "<tr><td align=\"left\">SSID</td><td><input type=\"text\" name=\"ssid\" autocomplete=\"off\"></input></td></tr>");
  strcat(response, "<tr><td align=\"left\">Password</td><td><input type=\"text\" name=\"password\" autocomplete=\"off\"></input></td></tr>");
  strcat(response, "<tr><td align=\"left\">Keep AP on</td><td><input type=\"checkbox\" name=\"keep_ap_on\" value=\"1\"></input></td></tr>");
  strcat(response, "<tr><td colspan=\"2\" align=\"right\"><input type=\"submit\"></input></td></tr>");
  strcat(response, "</table></form>");
  strcat(response, "</div></body></html>");
  server.send(200, "text/html", response);
}

void webHandle_PostWifiForm() {
  if (!server.hasArg("ssid") || !server.hasArg("password") || server.arg("ssid") == NULL || server.arg("password") == NULL) {
    server.send(417, "text/plain", "417: Invalid Request");
    return;
  }

  // save to eeprom
  server.arg("ssid").toCharArray(wifi_data.ssid, 20);
  server.arg("password").toCharArray(wifi_data.password, 20);
  wifi_data.keep_ap_on = (server.arg("keep_ap_on") && server.arg("keep_ap_on").charAt(0) == '1');
  EEPROM.put(0, wifi_data);
  EEPROM.commit();

  // send response
  char response[400];
  webRestarting(response);
  server.send(200, "text/html", response);
  delay(200);

  // restart esp
  ESP.restart();
}

void webHandle_PostRGBForm() {
  if (!server.hasArg("r") || !server.hasArg("g") || !server.hasArg("b") || server.arg("r") == NULL || server.arg("g") == NULL || server.arg("b") == NULL) {
    server.send(417, "text/plain", "417: Invalid Request");
    return;
  }

  // save to eeprom
  unsigned i;
  char data[32];
  
  server.arg("r").toCharArray(data, 32);
  sscanf(data, "%u", &i);
  Serial.print("Read red value: "); Serial.print(data); Serial.print(", as integer: "); Serial.println(i);
  configuration.r = i;
  
  server.arg("g").toCharArray(data, 32);
  sscanf(data, "%u", &i);
  Serial.print("Read green value: "); Serial.print(data); Serial.print(", as integer: "); Serial.println(i);
  configuration.g = i;
  
  server.arg("b").toCharArray(data, 32);
  sscanf(data, "%u", &i);
  Serial.print("Read blue value: "); Serial.print(data); Serial.print(", as integer: "); Serial.println(i);
  configuration.b = i;
  
  EEPROM.put(sizeof wifi_data, configuration);
  EEPROM.commit();
  delay(200);

  // update colors
  updateLEDStrip();
  delay(200);
  
  // send response
  server.sendHeader("Location", String("/"), true);
  server.send(301, "text/plain", "Redirecting...");
}

void webHandle_GetStyles() {
  char response[500];
  strcpy(response, "* {font-size: 14pt;}");
  strcat(response, "a {font-weight: bold;}");
  strcat(response, "table {margin-left:auto;margin-right:auto;}");
  strcat(response, ".position {width: 60%; margin-bottom: 10px; position: relative; margin-left: auto; margin-right: auto;}");
  strcat(response, ".title {text-align: center; font-weight: bold; font-size: 20pt;}");
  strcat(response, ".right {text-align: right;}");
  strcat(response, ".footer {font-size: 10pt; font-style: italic;}");
  strcat(response, ".menuitem {text-align: center; background-color: #efefef; cursor: pointer; border: 1px solid black;}");
  strcat(response, ".height30 {height: 30px;}");
  server.send(200, "text/css", response);
}

void webHandle_NotFound(){
  server.send(404, "text/plain", "404: Not found");
}

// *** NETWORKING
void initNetworking() {
  // read wifi config from eeprom
  EEPROM.get(0, wifi_data);
  Serial.println("Read data from EEPROM - ssid: " + String(wifi_data.ssid) + ", password: ****");
  
  // start AP
  WiFi.softAP("MLIGHT", "");
  Serial.print("Started AP on IP: ");
  Serial.println(WiFi.softAPIP());
  
  // start web server
  initWebserver();
  server.begin();
  Serial.println("Started web server on port 80");

  // attempt to start wifi if we have config
  WiFi.hostname("MLIGHT");
  WiFi.begin(wifi_data.ssid, wifi_data.password);
  Serial.print("Establishing WiFi connection");
  while (WiFi.status() != WL_CONNECTED) {
    setColorRgb(PWM_MAX, 0, 0);
    delay(200);
    setColorRgb(0, PWM_MAX, 0);
    delay(200);
    server.handleClient();
    Serial.print(".");
    delay(200);
  }
  Serial.print("\n");
  Serial.print("WiFi connection established - IP address: ");
  Serial.println(WiFi.localIP());

  // init mdns
  if (!MDNS.begin("mlight")) {
    Serial.println("Error setting up MDNS responder!");
  }
}

void webRestarting(char* buffer) {
  webHeader(buffer, false, "Restarting");
  strcat(buffer, "</body></html>");
}

bool isConnectedToNetwork() {
  if (WiFi.status() != WL_CONNECTED) {
    initNetworking();
  }
  return true;
}



void setup() {
  // init serial
  Serial.begin(115200);
  Serial.print("Version: ");
  Serial.println(VERSION_NUMBER);
  Serial.print("Last change: ");
  Serial.println(VERSION_LASTCHANGE);
  
  // init pins
  pinMode(PIN_RED, OUTPUT);
  pinMode(PIN_GREEN, OUTPUT);
  pinMode(PIN_BLUE, OUTPUT);

  // init EEPROM
  EEPROM.begin(sizeof wifi_data + sizeof configuration);

  // init networking
  initNetworking();

  // read config
  EEPROM.get(sizeof wifi_data, configuration);
  if (configuration.version != CONFIG_VERSION) {
    Serial.println("Setting standard configuration");
    configuration.version = CONFIG_VERSION;
    configuration.r = PWM_MAX;
    configuration.g = PWM_MAX;
    configuration.b = PWM_MAX;
    EEPROM.put(sizeof wifi_data, configuration);
    EEPROM.commit();
  }
  Serial.print("Read EERPOM configuration - red: "); Serial.println(configuration.r);
  Serial.print("Read EERPOM configuration - green: "); Serial.println(configuration.g);
  Serial.print("Read EERPOM configuration - blue: "); Serial.println(configuration.b);

  // set light
  updateLEDStrip();
}

void loop() {
  // disable AP 
  if ((WiFi.softAPIP() && millis() > DELAY_TURNOFF_AP) && wifi_data.keep_ap_on == false) {
    // diable AP
    Serial.println("Disabling AP...");
    WiFi.softAPdisconnect(false);
    WiFi.enableAP(false);
  }
  
  // handle incoming request to web server
  server.handleClient();
  yield();
}

void updateLEDStrip() {
  setColorRgb(configuration.r, configuration.g, configuration.b);
}

void setColorRgb(unsigned int red, unsigned int green, unsigned int blue) {
  unsigned int clamped_red = constrain(red, 0, PWM_MAX);
  unsigned int clamped_green = constrain(green, 0, PWM_MAX);
  unsigned int clamped_blue = constrain(blue, 0, PWM_MAX);
  Serial.print("Setting LED Strip - red: "); Serial.println(clamped_red);
  Serial.print("Setting LED Strip - green: "); Serial.println(clamped_green);
  Serial.print("Setting LED Strip - blue: "); Serial.println(clamped_blue);
  analogWrite(PIN_RED, clamped_red);
  analogWrite(PIN_GREEN, clamped_green);
  analogWrite(PIN_BLUE, clamped_blue);
 }
