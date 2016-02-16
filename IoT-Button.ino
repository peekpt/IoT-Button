/***
 *      ___    _____   ___      _   _
 *     |_ _|__|_   _| | _ )_  _| |_| |_ ___ _ _
 *      | |/ _ \| |   | _ \ || |  _|  _/ _ \ ' \
 *     |___\___/|_|   |___/\_,_|\__|\__\___/_||_|
 *
 */

#include <Ticker.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <WiFiClient.h>
#include <EEPROM.h>
#include <ESP8266WebServer.h>

Ticker ledOn;
Ticker ledTicker;
Ticker timeout;
Ticker timeoutLong;

// OUTPUT
int ledPin = 2;

// AP
const IPAddress apIP(192, 168, 1, 1);
const char* apSSID = "BUTTON";
DNSServer dnsServer;
ESP8266WebServer webServer(80);
String ssidList;

// STATION
String ssid;
String password;
int status = WL_IDLE_STATUS;
int retries = 3; // connection retries
int retriesDelay = 10; // time in seconds between retries
IPAddress ip;

// FLAGS
boolean canSleep = true;
boolean isConfig = true;
boolean dnsLoop = false;
boolean failed = false;
boolean timeoutEnabled = true;

// COUNTERS
unsigned int buttonClicks = 0;
int blinks = 0;

// IFTTT
const char* iftttUrl = "maker.ifttt.com";
String iftttKey;
String iftttEvent;



void setup() {

  Serial.begin(115200);

  EEPROM.begin(512);

  delay(20);
  Serial.println("\n\n\n");
  Serial.println("  ___    _____   ___      _   _            ");
  Serial.println(" |_ _|__|_   _| | _ )_  _| |_| |_ ___ _ _  ");
  Serial.println("  | |/ _ \\| |   | _ \\ || |  _|  _/ _ \\ ' \\ ");
  Serial.println(" |___\\___/|_|   |___/\\_,_|\\__|\\__\\___/_||_|");



  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, HIGH);

  isConfig = !readConfig(); // if eeprom 1st byte is 0 then setup mode

  if (isConfig) {
    // enter setup mode blink 200 times (thread safe)
    blinks = 200;
    blinkLed();
    scanAPs();
  }

  else  {
    // connect AP and to ifttt server and send the payload
    if (connectToAP()) {
      // connection to AP success
      blinks = 1;
      blinkLed();
      if (sendEvent()) {
        // event sent
        blinks = 5;
        blinkLed();


      } else {
        failed = true;
        blinks = 2;
        blinkLed();
      }
    } else {
      // connection to AP failed
      failed = true;
      blinks = 2;
      blinkLed();
    }

  }
  delay(200);
  apMode();
  delay(200);
  startWebServer();


  //  timeouts: 1 minute to connect to the webserver, 10 minutes to config until sleep.

  timeout.once(60, []() {
    sleepNow();
  });

  timeoutLong.once(600, []() {
    sleepNow();
  });

}// end setup

void sleepNow() {
  pinMode(ledPin, INPUT);
  Serial.println("\n\n***Sleep Mode\n\n");
  delay(500);
  ESP.deepSleep(100, WAKE_RF_DISABLED);
  delay(1000);
}

void loop() {

  if (dnsLoop) {
    dnsServer.processNextRequest();
  }

  webServer.handleClient();

}

void saveConfig () {
  Serial.println("\n***Saving Configuration***");
  clearConfig ();
  Serial.print("Saving AP SSID...");
  for (int i = 0; i < ssid.length(); ++i) {
    EEPROM.write(i, ssid[i]);
  }
  Serial.println("\t\t[OK]");

  Serial.print("Saving AP PASS...");
  for (int i = 0; i < password.length(); ++i) {
    EEPROM.write(32 + i, password[i]);
  }
  Serial.println("\t\t[OK]");

  Serial.print("Saving IFTTT EVENT...");
  for (int i = 0; i < iftttEvent.length(); ++i) {
    EEPROM.write(96 + i, iftttEvent[i]);
  }
  Serial.println("\t\t[OK]");

  Serial.print("Saving IFTTT KEY...");
  for (int i = 0; i < iftttKey.length(); ++i) {
    EEPROM.write(128 + i, iftttKey[i]);
  }
  Serial.println("\t\t[OK]");

  EEPROM.commit();
}

boolean readConfig() {
  // reset vars
  ssid = "";
  password = "";
  iftttKey = "";
  iftttEvent = "";

  Serial.println("\n***Loading Configuration***");

  if (EEPROM.read(0) != 0) {
    // read ssid 32bytes
    for (int i = 0; i < 32; ++i) {
      ssid += char(EEPROM.read(i));
    }
    Serial.print("SSID: ");
    Serial.println(ssid);

    //read password 64bytes
    for (int i = 32; i < 96; ++i) {
      password += char(EEPROM.read(i));

    }
    Serial.print("Password: ");
    Serial.println(password);

    //read event 32bytes
    for (int i = 96; i < 128; ++i) {
      iftttEvent += char(EEPROM.read(i));
    }
    Serial.print("Event: ");
    Serial.println(iftttEvent);

    //read ifttt key 32bytes
    for (int i = 128; i < 160; ++i) {
      iftttKey += char(EEPROM.read(i));
    }
    Serial.print("IFTTT Key: ");
    Serial.println(iftttKey);

    return true;
  }

  else {
    Serial.println("Saved Configuration not found.");
    return false;
  }
  return false;
}

void clearConfig () {
  Serial.print("Clear Configuration...");



  for (int i = 0; i < 162; ++i) {
    EEPROM.write(i, 0);
  }
  EEPROM.commit();
  Serial.println("\t\t[OK]");

}

boolean connectToAP() {
  boolean isConnected = false;
  Serial.print("\n***Attemping to connect to Access Point: ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  int tries = 1;
  while (status != WL_CONNECTED && tries <= retries) {

    Serial.print("#");
    Serial.print(tries);
    Serial.print(" requesting...");
    status = WiFi.begin(ssid.c_str(), password.c_str());
    delay(retriesDelay * 1000);
    if (status != WL_CONNECTED) {
      Serial.println("\t\t[FAIL]");
      isConnected = false;
    } else {
      Serial.println("\t\t[OK]");
      isConnected = true;
      ip = WiFi.localIP();
    }
    tries += 1;
  }
  if (isConnected) {
    String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
    Serial.println ("***Connected\t\t");
    Serial.print("IP >> ");
    Serial.println(ipStr);
  } else {
    Serial.println ("***Failed to Connect\n");
  }
  return isConnected;
}

void scanAPs() {
  Serial.print("\n***Scanning for Wifi Networks nearby...");
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  int n = WiFi.scanNetworks();
  delay(100);
  Serial.println("\t[Done]");
  Serial.println("");
  for (int i = 0; i < n; ++i) {
    Serial.println(WiFi.SSID(i));
    ssidList += "<option value=\"";
    ssidList += WiFi.SSID(i);
    ssidList += "\">";
    ssidList += WiFi.SSID(i);
    ssidList += "</option>";
  }
}

void apMode() {
  WiFi.disconnect();
  delay(100);
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP(apSSID);

  Serial.print("\n***Starting Access Point at \"");
  Serial.print(apSSID);
  Serial.println("\"");
  Serial.println("\n***Starting DNS server...");
  dnsServer.start(53, "*", apIP);

}

void startWebServer() {
  dnsLoop = true;
  if (isConfig) {

    // Configuration mode

    Serial.println("\n***Starting Web Server...");

    // Settings Page
    webServer.on("/settings", []() {
      Serial.println("\n***Starting Config...");
      if (timeoutEnabled) {
        timeout.detach();
        Serial.println("\n***Disable Sleep Mode for 10 minutes...");
        timeoutEnabled = false;
      }
      String s = "<h2>Wi-Fi Settings</h2><p>Please select the SSID of the network you wish to connect to and then enter the password and submit.</p>";
      s += "<form method=\"get\" action=\"config\"><label>SSID: </label><select name=\"ssid\">";
      s += ssidList;
      s += "</select><br><br>Password: <input name=\"pass\" length=64 type=\"password\"><br><br>Ifttt event: <input name=\"event\" length=32 type=\"text\"><br><br>Ifttt key: <input name=\"key\" length=32 type=\"text\"><br><br><input type=\"submit\"></form>";
      webServer.send(200, "text/html", makePage("Configuration Settings", "Configuration", s));
    });

    webServer.on("/config", []() {
      if (timeoutEnabled) {
        timeout.detach();
        Serial.println("\n***Disable Sleep Mode for 10 minutes...");
        timeoutEnabled = false;
      }
      ssid = urlDecode(webServer.arg("ssid"));
      Serial.print("SSID: ");
      Serial.println(ssid);
      password = urlDecode(webServer.arg("pass"));
      Serial.print("Password: ");
      Serial.println(password);
      iftttEvent = urlDecode(webServer.arg("event"));
      Serial.print("Ifttt event: ");
      Serial.println(iftttEvent);
      iftttKey = urlDecode(webServer.arg("key"));
      Serial.print("Ifttt key: ");
      Serial.println(iftttKey);
      saveConfig();
      String s = "<h1>IoT Button Configuration completed.</h1><p>The button will be connected automatically to \"";
      s += ssid;
      s += "\" after the restart.";
      webServer.send(200, "text/html", makePage("Configuration", "Configuration", s));
      ESP.restart();
    });
    // Show the configuration page if no path is specified
    webServer.onNotFound([]() {
      timeout.detach();
      Serial.println("\n***Disable Sleep Mode for 10 minutes...");
      String s = "<h1>Configuration</h1><p><a href=\"/settings\">Configure</a></p>";
      webServer.send(200, "text/html", makePage("Access Point mode", "Settings", s));
    });


  } else {

    // Info Mode

    Serial.println("\n***Starting Info Mode, connect AP in 1 minute.");

    webServer.on("/", []() {
      if (timeoutEnabled) {
        timeout.detach();
        Serial.println("\n***Disable Sleep Mode for 10 minutes...");
        timeoutEnabled = false;
      }

      String payload;
      payload =   "<h1>STATUS</h1>";
      if (isConfig)
        payload +=  (failed) ? "<p>***FAILED TO SEND EVENT<***/p>" : "<p>LAST EVENT WAS SENT!</p>";
      payload +=  "<h3>NETWORK DETAILS</h3>";
      payload +=  "<p>Connected to: " + String(apSSID) + "</p>";
      payload +=  "<h3>BUTTON DETAILS</h3>";
      payload +=  "<p>IFTTT event: " + String(iftttEvent.c_str()) + "</p>";
      payload +=  "<p>Button Presses: " + String(buttonClicks) + "</p>";
      payload +=  "<h3>OPTIONS</h3>";
      payload +=  "<p><a href=\"/reset\">Clear all Configuration</a></p>";
      webServer.send(200, "text/html", makePage("Button Status", "Internet of Things", payload));
    });

    webServer.on("/reset", []() {
      clearConfig();
      Serial.println("\n\n***Configuration Cleared");
      String payload = "<h1>The configuration was cleared.</h1><p>Please click the button to reset device.</p>";
      webServer.send(200, "text/html", makePage("Clear Settings", "Clear Settings", payload));
    });

  }

  webServer.begin();

}

String urlDecode(String input) {
  String s = input;
  s.replace("%20", " ");
  s.replace("+", " ");
  s.replace("%21", "!");
  s.replace("%22", "\"");
  s.replace("%23", "#");
  s.replace("%24", "$");
  s.replace("%25", "%");
  s.replace("%26", "&");
  s.replace("%27", "\'");
  s.replace("%28", "(");
  s.replace("%29", ")");
  s.replace("%30", "*");
  s.replace("%31", "+");
  s.replace("%2C", ",");
  s.replace("%2E", ".");
  s.replace("%2F", "/");
  s.replace("%2C", ",");
  s.replace("%3A", ":");
  s.replace("%3A", ";");
  s.replace("%3C", "<");
  s.replace("%3D", "=");
  s.replace("%3E", ">");
  s.replace("%3F", "?");
  s.replace("%40", "@");
  s.replace("%5B", "[");
  s.replace("%5C", "\\");
  s.replace("%5D", "]");
  s.replace("%5E", "^");
  s.replace("%5F", "-");
  s.replace("%60", "`");
  return s;
}

String makePage(String title, String headerTitle, String contents) {
  String s = "<!DOCTYPE html><html><head>";
  s += "<meta name=\"viewport\" content=\"width=device-width,user-scalable=0\">";
  s += "<style>";
  // Simple Reset CSS
  s += "*,*:before,*:after{-webkit-box-sizing:border-box;-moz-box-sizing:border-box;box-sizing:border-box}html{font-size:100%;-ms-text-size-adjust:100%;-webkit-text-size-adjust:100%}html,button,input,select,textarea{font-family:sans-serif}article,aside,details,figcaption,figure,footer,header,hgroup,main,nav,section,summary{display:block}body,form,fieldset,legend,input,select,textarea,button{margin:0}audio,canvas,progress,video{display:inline-block;vertical-align:baseline}audio:not([controls]){display:none;height:0}[hidden],template{display:none}img{border:0}svg:not(:root){overflow:hidden}body{font-family:sans-serif;font-size:16px;font-size:1rem;line-height:22px;line-height:1.375rem;color:#585858;font-weight:400;background:#fff}p{margin:0 0 1em 0}a{color:#cd5c5c;background:transparent;text-decoration:underline}a:active,a:hover{outline:0;text-decoration:none}strong{font-weight:700}em{font-style:italic}";
  // Basic CSS Styles
  s += "body{font-family:sans-serif;font-size:16px;font-size:1rem;line-height:22px;line-height:1.375rem;color:#585858;font-weight:400;background:#fff}p{margin:0 0 1em 0}a{color:#cd5c5c;background:transparent;text-decoration:underline}a:active,a:hover{outline:0;text-decoration:none}strong{font-weight:700}em{font-style:italic}h1{font-size:32px;font-size:2rem;line-height:38px;line-height:2.375rem;margin-top:0.7em;margin-bottom:0.5em;color:#343434;font-weight:400}fieldset,legend{border:0;margin:0;padding:0}legend{font-size:18px;font-size:1.125rem;line-height:24px;line-height:1.5rem;font-weight:700}label,button,input,optgroup,select,textarea{color:inherit;font:inherit;margin:0}input{line-height:normal}.input{width:100%}input[type='text'],input[type='email'],input[type='tel'],input[type='date']{height:36px;padding:0 0.4em}input[type='checkbox'],input[type='radio']{box-sizing:border-box;padding:0}";
  // Custom CSS
  s += "header{width:100%;background-color: #2c3e50;top: 0;min-height:60px;margin-bottom:21px;font-size:15px;color: #fff}.content-body{padding:0 1em 0 1em}header p{font-size: 1.25rem;float: left;position: relative;z-index: 1000;line-height: normal; margin: 15px 0 0 10px}";
  s += "</style>";
  s += "<title>";
  s += title;
  s += "</title></head><body>";
  s += "<header><p>" + headerTitle + "</p></header>";
  s += "<div class=\"content-body\">";
  s += contents;
  s += "</div>";
  s += "</body></html>";
  return s;
}

boolean sendEvent() {
  buttonClicks++;
  EEPROMWriteInt(160, buttonClicks);

  WiFiClient client;
  IPAddress ip = WiFi.localIP();
  String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
  String value_1 = String(buttonClicks);
  String value_2 = String(ssid.c_str());
  String value_3 = "";
  // Build JSON data string
  String data = "";
  data = data + "\n" + "{\"value1\":\"" + value_1 + "\",\"value2\":\"" + value_2 + "\",\"value3\":\"" + value_3 + "\"}";

  String url = "/trigger/" + String(iftttEvent.c_str()) + "/with/key/" + String(iftttKey.c_str());
  if (client.connect(iftttUrl, 80)) {
    client.println("POST " + url + " HTTP/1.1");
    Serial.println("POST " + url + " HTTP/1.1");
    client.println("Host: " + String(iftttUrl));
    Serial.println("Host: " + String(iftttUrl));
    client.println("User-Agent: Arduino/1.0");
    Serial.println("User-Agent: Arduino/1.0");
    client.print("Accept: *");
    Serial.print("Accept: *");
    client.print("/");
    Serial.print("/");
    client.println("*");
    Serial.println("*");
    client.print("Content-Length: ");
    Serial.print("Content-Length: ");
    client.println(data.length());
    Serial.println(data.length());
    client.println("Content-Type: application/json");
    Serial.println("Content-Type: application/json");
    client.println("Connection: close");
    Serial.println("Connection: close");
    client.println();
    Serial.println();
    client.println(data);
    Serial.println(data);
    Serial.println("Event sent!");
    return true;
  } else {
    return false;
  }
}

void led() {
  digitalWrite(ledPin, LOW);
  ledOn.once_ms(200, []() {
    digitalWrite(ledPin, HIGH);
  });
}

void blinkLed() {
  if (blinks == 0) {
    digitalWrite(ledPin, HIGH);
    ledTicker.detach();
    return;
  }
  led();
  blinks--;
  ledTicker.once_ms(500, []() {
    blinkLed();
  });

}


//This function will write a 2 byte integer to the eeprom at the specified address and address + 1
void EEPROMWriteInt(int p_address, int p_value)
{
  byte lowByte = ((p_value >> 0) & 0xFF);
  byte highByte = ((p_value >> 8) & 0xFF);

  EEPROM.write(p_address, lowByte);
  EEPROM.write(p_address + 1, highByte);
  EEPROM.commit();
}

//This function will read a 2 byte integer from the eeprom at the specified address and address + 1
unsigned int EEPROMReadInt(int p_address)
{
  byte lowByte = EEPROM.read(p_address);
  byte highByte = EEPROM.read(p_address + 1);

  return ((lowByte << 0) & 0xFF) + ((highByte << 8) & 0xFF00);
}

