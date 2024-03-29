/*
This is Official ESP8266 Duco miner with some edits by me(Mystic)
*/


#pragma GCC optimize("-Ofast")
#include <ArduinoJson.h>
#include <bearssl/bearssl.h>
#include <TypeConversion.h>
#include "secret.h"
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <Ticker.h>
#include <ESP8266WebServer.h>

// Uncomment the line below if you wish to use a DHT sensor (Duino IoT beta)
// #define USE_DHT

// Uncomment the line below if you wish to register for IOT updates with an MQTT broker
// #define USE_MQTT

// If you don't know what MQTT means check this link:
// https://www.techtarget.com/iotagenda/definition/MQTT-MQ-Telemetry-Transport

#ifdef USE_DHT
float temp = 0.0;
float hum = 0.0;

// Install "DHT sensor library" if you get an error
#include <DHT.h>
// Change D3 to the pin you've connected your sensor to
#define DHTPIN D3
// Set DHT11 or DHT22 accordingly
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);
#endif

#ifdef USE_MQTT
// Install "PubSubClient" if you get an error
#include <PubSubClient.h>

long lastMsg = 0;

// Change the part in brackets to your MQTT broker address
#define mqtt_server "broker.hivemq.com"
// broker.hivemq.com is for testing purposes, change it to your broker address

// Change this to your MQTT broker port
#define mqtt_port 1883
// If you want to use user and password for your MQTT broker, uncomment the line below
// #define mqtt_use_credentials

// Change the part in brackets to your MQTT broker username
#define mqtt_user "My cool mqtt username"
// Change the part in brackets to your MQTT broker password
#define mqtt_password "My secret mqtt pass"

// Change this if you want to send data to the topic every X milliseconds
#define mqtt_update_time 5000

// Change the part in brackets to your MQTT humidity topic
#define humidity_topic "sensor/humidity"
// Change the part in brackets to your MQTT temperature topic
#define temperature_topic "sensor/temperature"

WiFiClient espClient;
PubSubClient mqttClient(espClient);

void mqttReconnect()
{
  // Loop until we're reconnected
  while (!mqttClient.connected())
  {
    Serial.print("Attempting MQTT connection...");

    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);

    // Attempt to connect
#ifdef mqtt_use_credentials
    if (mqttClient.connect("ESP8266Client", mqtt_user, mqtt_password))
#else
    if (mqttClient.connect(clientId.c_str()))
#endif
    {
      Serial.println("connected");
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
#endif

namespace
{

  // Discord Webhook edit by mystic
  const char *discord = DISCORD_URL;
  const boolean discord_tts = DISCORD_TTS;
  const int interval = 10000;
  int stake;
  float balance;
  String *serverURL = "https://server.duinocoin.com/users/" + String(Username) + "";

  // Change the part in brackets to your WiFi name
  const char *SSID = Wifiname;
  // Change the part in brackets to your WiFi password
  const char *PASSWORD = Password;
  // Change the part in brackets to your Duino-Coin username
  const char *USERNAME = Username;
  // Change the part in brackets if you want to set a custom miner name (use Auto to autogenerate, None for no name)
  const char *RIG_IDENTIFIER = "";
  // Change the part in brackets to your mining key (if you enabled it in the wallet)
  const char *MINER_KEY = "";
  // Change false to true if using 160 MHz clock mode to not get the first share rejected
  const bool USE_HIGHER_DIFF = true;
  // Change true to false if you don't want to host the dashboard page
  const bool WEB_DASHBOARD = false;
  // Change false to true if you want to update hashrate in browser without reloading page
  const bool WEB_HASH_UPDATER = false;
  // Change true to false if you want to disable led blinking(But the LED will work in the beginning until esp connects to the pool)
  const bool LED_BLINKING = false;

  /* Do not change the lines below. These lines are static and dynamic variables
     that will be used by the program for counters and measurements. */
  const char *DEVICE = "NodeMCU";
  const char *POOLPICKER_URL[] = {"https://server.duinocoin.com/getPool"};
  const char *MINER_BANNER = "Official ESP8266 Miner";
  const char *MINER_VER = "3.18";
  unsigned int share_count = 0;
  unsigned int port = 0;
  unsigned int difficulty = 0;
  float hashrate = 0;
  String AutoRigName = "";
  String host = "";
  String node_id = "";

  const char WEBSITE[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
<!--
    Duino-Coin self-hosted dashboard
    MIT licensed
    Duino-Coin official 2019-2022
    https://github.com/revoxhere/duino-coin
    https://duinocoin.com
-->
<head>
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>Duino-Coin @@DEVICE@@ dashboard</title>
    <link rel="stylesheet" href="https://server.duinocoin.com/assets/css/mystyles.css">
    <link rel="shortcut icon" href="https://github.com/revoxhere/duino-coin/blob/master/Resources/duco.png?raw=true">
    <link rel="icon" type="image/png" href="https://github.com/revoxhere/duino-coin/blob/master/Resources/duco.png?raw=true">
</head>
<body>
    <section class="section">
        <div class="container">
            <h1 class="title">
                <img class="icon" src="https://github.com/revoxhere/duino-coin/blob/master/Resources/duco.png?raw=true">
                @@DEVICE@@ <small>(@@ID@@)</small>
            </h1>
            <p class="subtitle">
                Self-hosted, lightweight, official dashboard for your <strong>Duino-Coin</strong> miner
            </p>
        </div>
        <br>
        <div class="container">
            <div class="columns">
                <div class="column">
                    <div class="box">
                        <p class="subtitle">
                            Mining statistics
                        </p>
                        <div class="columns is-multiline">
                            <div class="column" style="min-width:15em">
                                <div class="title is-size-5 mb-0">
                                    <span id="hashratex">@@HASHRATE@@</span>kH/s
                                </div>
                                <div class="heading is-size-5">
                                    Hashrate
                                </div>
                            </div>
                            <div class="column" style="min-width:15em">
                                <div class="title is-size-5 mb-0">
                                    @@DIFF@@
                                </div>
                                <div class="heading is-size-5">
                                    Difficulty
                                </div>
                            </div>
                            <div class="column" style="min-width:15em">
                                <div class="title is-size-5 mb-0">
                                    @@SHARES@@
                                </div>
                                <div class="heading is-size-5">
                                    Shares
                                </div>
                            </div>
                            <div class="column" style="min-width:15em">
                                <div class="title is-size-5 mb-0">
                                    @@NODE@@
                                </div>
                                <div class="heading is-size-5">
                                    Node
                                </div>
                            </div>
                        </div>
                    </div>
                </div>
                <div class="column">
                    <div class="box">
                        <p class="subtitle">
                            Device information
                        </p>
                        <div class="columns is-multiline">
                            <div class="column" style="min-width:15em">
                                <div class="title is-size-5 mb-0">
                                    @@DEVICE@@
                                </div>
                                <div class="heading is-size-5">
                                    Device type
                                </div>
                            </div>
                            <div class="column" style="min-width:15em">
                                <div class="title is-size-5 mb-0">
                                    @@ID@@
                                </div>
                                <div class="heading is-size-5">
                                    Device ID
                                </div>
                            </div>
                            <div class="column" style="min-width:15em">
                                <div class="title is-size-5 mb-0">
                                    @@MEMORY@@
                                </div>
                                <div class="heading is-size-5">
                                    Free memory
                                </div>
                            </div>
                            <div class="column" style="min-width:15em">
                                <div class="title is-size-5 mb-0">
                                    @@VERSION@@
                                </div>
                                <div class="heading is-size-5">
                                    Miner version
                                </div>
                            </div>
)====="
#ifdef USE_DHT
                                 "                            <div class=\"column\" style=\"min-width:15em\">"
                                 "                                <div class=\"title is-size-5 mb-0\">"
                                 "                                    @@TEMP@@ °C"
                                 "                                </div>"
                                 "                                <div class=\"heading is-size-5\">"
                                 "                                    Temperature"
                                 "                                </div>"
                                 "                            </div>"
                                 "                            <div class=\"column\" style=\"min-width:15em\">"
                                 "                                <div class=\"title is-size-5 mb-0\">"
                                 "                                    @@HUM@@ %"
                                 "                                </div>"
                                 "                                <div class=\"heading is-size-5\">"
                                 "                                    Humidity"
                                 "                                </div>"
                                 "                            </div>"
#endif
                                 R"=====(
                        </div>
                    </div>
                </div>
            </div>
            <br>
            <div class="has-text-centered">
                <div class="title is-size-6 mb-0">
                    Hosted on
                    <a href="http://@@IP_ADDR@@">
                        http://<b>@@IP_ADDR@@</b>
                    </a>
                    &bull;
                    <a href="https://duinocoin.com">
                        duinocoin.com
                    </a>
                    &bull;
                    <a href="https://github.com/revoxhere/duino-coin">
                        github.com/revoxhere/duino-coin
                    </a>
                </div>
            </div>
        </div>
        <script>
            setInterval(function(){
                getData();
            }, 3000);
            
            function getData() {
                var xhttp = new XMLHttpRequest();
                xhttp.onreadystatechange = function() {
                    if (this.readyState == 4 && this.status == 200) {
                        document.getElementById("hashratex").innerHTML = this.responseText;
                    }
                };
                xhttp.open("GET", "hashrateread", true);
                xhttp.send();
            }
        </script>
    </section>
</body>
</html>
)=====";

  ESP8266WebServer server(80);

  void hashupdater()
  { // update hashrate every 3 sec in browser without reloading page
    server.send(200, "text/plain", String(hashrate / 1000));
    Serial.println("Update hashrate on page");
  };

  void UpdateHostPort(String input)
  {
    // Thanks @ricaun for the code
    DynamicJsonDocument doc(256);
    deserializeJson(doc, input);
    const char *name = doc["name"];

    host = String((const char *)doc["ip"]);
    port = int(doc["port"]);
    node_id = String(name);

    Serial.println("Poolpicker selected the best mining node: " + node_id);
  }

  String httpGetString(String URL)
  {
    String payload = "";
    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient http;

    if (http.begin(client, URL))
    {
      int httpCode = http.GET();

      if (httpCode == HTTP_CODE_OK)
        payload = http.getString();
      else
        Serial.printf("Error fetching node from poolpicker: %s\n", http.errorToString(httpCode).c_str());

      http.end();
    }
    return payload;
  }

  void UpdatePool()
  {
    String input = "";
    int waitTime = 1;
    int poolIndex = 0;
    int poolSize = sizeof(POOLPICKER_URL) / sizeof(char *);

    while (input == "")
    {
      Serial.println("Fetching mining node from the poolpicker in " + String(waitTime) + "s");
      input = httpGetString(POOLPICKER_URL[poolIndex]);
      poolIndex += 1;

      // Check if pool index needs to roll over
      if (poolIndex >= poolSize)
      {
        poolIndex %= poolSize;
        delay(waitTime * 1000);

        // Increase wait time till a maximum of 32 seconds (addresses: Limit connection requests on failure in ESP boards #1041)
        waitTime *= 2;
        if (waitTime > 32)
          waitTime = 32;
      }
    }

    // Setup pool with new input
    UpdateHostPort(input);
  }

  WiFiClient client;
  String client_buffer = "";
  String chipID = "";
  String START_DIFF = "";

  // Loop WDT... please don't feed me...
  // See lwdtcb() and lwdtFeed() below
  Ticker lwdTimer;
#define LWD_TIMEOUT 60000

  unsigned long lwdCurrentMillis = 0;
  unsigned long lwdTimeOutMillis = LWD_TIMEOUT;

#define END_TOKEN '\n'
#define SEP_TOKEN ','

#define LED_BUILTIN 2

#define BLINK_SHARE_FOUND 1
#define BLINK_SETUP_COMPLETE 2
#define BLINK_CLIENT_CONNECT 3
#define BLINK_RESET_DEVICE 5

  void SetupWifi()
  {
    Serial.println("Connecting to: " + String(SSID));
    WiFi.mode(WIFI_STA); // Setup ESP in client mode
    WiFi.setSleepMode(WIFI_NONE_SLEEP);
    WiFi.begin(SSID, PASSWORD);

    int wait_passes = 0;
    while (WiFi.waitForConnectResult() != WL_CONNECTED)
    {
      delay(500);
      Serial.print(".");
      if (++wait_passes >= 10)
      {
        WiFi.begin(SSID, PASSWORD);
        wait_passes = 0;
      }
    }

    Serial.println("\n\nnSuccessfully connected to WiFi");
    Serial.println("Local IP address: " + WiFi.localIP().toString());
    Serial.println("Rig name: " + String(RIG_IDENTIFIER));
    Serial.println();

    UpdatePool();
  }

  void SetupOTA()
  {
    // Prepare OTA handler
    ArduinoOTA.onStart([]()
                       { Serial.println("Start"); });
    ArduinoOTA.onEnd([]()
                     { Serial.println("\nEnd"); });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total)
                          { Serial.printf("Progress: %u%%\r", (progress / (total / 100))); });
    ArduinoOTA.onError([](ota_error_t error)
                       {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed"); });

    ArduinoOTA.setHostname(RIG_IDENTIFIER); // Give port a name not just address
    ArduinoOTA.begin();
  }

  void blink(uint8_t count, uint8_t pin = LED_BUILTIN)
  {
    if (LED_BLINKING)
    {
      uint8_t state = HIGH;

      for (int x = 0; x < (count << 1); ++x)
      {
        digitalWrite(pin, state ^= HIGH);
        delay(50);
      }
    }
  }

  void RestartESP(String msg)
  {
    Serial.println(msg);
    Serial.println("Restarting ESP...");
    blink(BLINK_RESET_DEVICE);
    ESP.reset();
  }

  // Our new WDT to help prevent freezes
  // code concept taken from https://sigmdel.ca/michel/program/esp8266/arduino/watchdogs2_en.html
  void ICACHE_RAM_ATTR lwdtcb(void)
  {
    if ((millis() - lwdCurrentMillis > LWD_TIMEOUT) || (lwdTimeOutMillis - lwdCurrentMillis != LWD_TIMEOUT))
      RestartESP("Loop WDT Failed!");
  }

  void lwdtFeed(void)
  {
    lwdCurrentMillis = millis();
    lwdTimeOutMillis = lwdCurrentMillis + LWD_TIMEOUT;
  }

  void VerifyWifi()
  {
    while (WiFi.status() != WL_CONNECTED || WiFi.localIP() == IPAddress(0, 0, 0, 0))
      WiFi.reconnect();
  }

  void handleSystemEvents(void)
  {
    VerifyWifi();
    ArduinoOTA.handle();
    yield();
  }

  // https://stackoverflow.com/questions/9072320/split-string-into-string-array
  String getValue(String data, char separator, int index)
  {
    int found = 0;
    int strIndex[] = {0, -1};
    int max_index = data.length() - 1;

    for (int i = 0; i <= max_index && found <= index; i++)
    {
      if (data.charAt(i) == separator || i == max_index)
      {
        found++;
        strIndex[0] = strIndex[1] + 1;
        strIndex[1] = (i == max_index) ? i + 1 : i;
      }
    }
    return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
  }

  void waitForClientData(void)
  {
    client_buffer = "";

    while (client.connected())
    {
      if (client.available())
      {
        client_buffer = client.readStringUntil(END_TOKEN);
        if (client_buffer.length() == 1 && client_buffer[0] == END_TOKEN)
          client_buffer = "???\n"; // NOTE: Should never happen

        break;
      }
      handleSystemEvents();
    }
  }

  void ConnectToServer()
  {
    if (client.connected())
      return;

    Serial.println("\n\nConnecting to the Duino-Coin server...");
    while (!client.connect(host, port))
      ;

    waitForClientData();
    Serial.println("Connected to the server. Server version: " + client_buffer);
    blink(BLINK_CLIENT_CONNECT); // Sucessfull connection with the server
  }

  bool max_micros_elapsed(unsigned long current, unsigned long max_elapsed)
  {
    static unsigned long _start = 0;

    if ((current - _start) > max_elapsed)
    {
      _start = current;
      return true;
    }
    return false;
  }

  void dashboard()
  {
    Serial.println("Handling HTTP client");

    String s = WEBSITE;
    s.replace("@@IP_ADDR@@", WiFi.localIP().toString());

    s.replace("@@HASHRATE@@", String(hashrate / 1000));
    s.replace("@@DIFF@@", String(difficulty / 100));
    s.replace("@@SHARES@@", String(share_count));
    s.replace("@@NODE@@", String(node_id));

    s.replace("@@DEVICE@@", String(DEVICE));
    s.replace("@@ID@@", String(RIG_IDENTIFIER));
    s.replace("@@MEMORY@@", String(ESP.getFreeHeap()));
    s.replace("@@VERSION@@", String(MINER_VER));
#ifdef USE_DHT
    s.replace("@@TEMP@@", String(temp));
    s.replace("@@HUM@@", String(hum));
#endif
    server.send(200, "text/html", s);
  }

} // namespace

void webhook()
{

  if (WiFi.status() == WL_CONNECTED)
  {
    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient https;
    // Sets up POST request to discord webhook.
    https.begin(client, discord);
    https.addHeader("Content-Type", "application/json");

    int httpsCode = https.POST("{\"content\":null,\"embeds\":[{\"title\":\"Duino-Coin Mining Logs\",\"description\":\":coin:**Balance**: "+ String(balance) +" DUCO\\n:euro:**Stake**: "+ String(stake) +" DUCO\",\"url\":\"https://wallet.duinocoin.com/\",\"color\":16729856,\"fields\":[{\"name\":\":satellite_orbital:Details:\",\"value\":\":computer:Status: " + client_buffer + "\\n:jigsaw:Share: " + String(share_count) + "\\n:hash:Hashrate: " + String(hashrate) + " H/s\\n:radioactive:Memory: " + String(ESP.getFreeHeap()/1000) + "%\"}],\"footer\":{\"text\":\"Made by MysticUwU <3\"},\"thumbnail\":{\"url\":\"https://lh3.googleusercontent.com/u4urRlG69w3g5mwwoRK5hot4mUbYCOVl6SbEKfyU5QfCXpw2zjZPkCD3YGd7ItFvxD96AhA7P_M1yIEJ584sLWpC6Aa8t2AbDq9cdA=w600\"}}],\"attachments\":[]}");

    // if the code returned is -1 there has been an error, that's why it checks on -1.
    if (httpsCode > -1)
    {
      Serial.println("Message send succesfull");
    }
    else
    {
      Serial.println("Error sending message");
    }
    https.end();
  }
}

void dataFetcher(){
   std::unique_ptr<BearSSL::WiFiClientSecure>client(new BearSSL::WiFiClientSecure);
    client->setInsecure();
    // Or, if you happy to ignore the SSL certificate, then use the following line instead:
    // client->setInsecure();

    HTTPClient https;

    Serial.print("[HTTPS] begin...\n");
    if (https.begin(*client, serverURL)) {  // HTTPS

      Serial.print("[HTTPS] GET...\n");
      // start connection and send HTTP header
      int httpCode = https.GET();

      // httpCode will be negative on error
      if (httpCode > 0) {
        // HTTP header has been send and Server response header has been handled
        Serial.printf("[HTTPS] GET... code: %d\n", httpCode);

        // file found at server
        if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
          String payload = https.getString();
          Serial.println(payload);
          DynamicJsonDocument doc(5000);
          const char*json = payload.c_str();
          deserializeJson(doc, json);

          JsonObject obj = doc.as<JsonObject>();

          stake = obj["result"]["balance"]["stake_amount"].as<float>();
          balance = obj["result"]["balance"]["balance"].as<float>();


        }
      } else {
        Serial.printf("[HTTPS] GET... failed, error: %s\n", https.errorToString(httpCode).c_str());
      }

      https.end();
    }
    
    }
  

boolean runEvery(unsigned long interval)
{ // Such a usefull function
  static unsigned long previousTime = 0;
  unsigned long currentTime = millis();
  if (currentTime - previousTime >= interval)
  {
    previousTime = currentTime;
    return true;
  }
  return false;
}

void setup()
{

  Serial.begin(500000);
  Serial.println("\nDuino-Coin " + String(MINER_VER));
  pinMode(LED_BUILTIN, OUTPUT);

#ifdef USE_MQTT
  mqttClient.setServer(mqtt_server, mqtt_port);
#endif

#ifdef USE_DHT
  Serial.println("Initializing DHT sensor");
  dht.begin();
  Serial.println("Test reading: " + String(dht.readHumidity()) + "% humidity");
  Serial.println("Test reading: temperature " + String(dht.readTemperature()) + "*C");
#endif

  // Autogenerate ID if required
  chipID = String(ESP.getChipId(), HEX);

  if (strcmp(RIG_IDENTIFIER, "Auto") == 0)
  {
    AutoRigName = "ESP8266-" + chipID;
    AutoRigName.toUpperCase();
    RIG_IDENTIFIER = AutoRigName.c_str();
  }

  SetupWifi();
  SetupOTA();

  lwdtFeed();
  lwdTimer.attach_ms(LWD_TIMEOUT, lwdtcb);
  if (USE_HIGHER_DIFF)
    START_DIFF = "ESP8266H";
  else
    START_DIFF = "ESP8266";

  if (WEB_DASHBOARD)
  {
    if (!MDNS.begin(RIG_IDENTIFIER))
    {
      Serial.println("mDNS unavailable");
    }
    MDNS.addService("http", "tcp", 80);
    Serial.print("Configured mDNS for dashboard on http://" + String(RIG_IDENTIFIER) + ".local (or http://" + WiFi.localIP().toString() + ")");
    server.on("/", dashboard);
    if (WEB_HASH_UPDATER)
      server.on("/hashrateread", hashupdater);
    server.begin();
  }

  blink(BLINK_SETUP_COMPLETE);
}

void loop()
{
  br_sha1_context sha1_ctx, sha1_ctx_base;
  uint8_t hashArray[20];
  String duco_numeric_result_str;

  // 1 minute watchdog
  lwdtFeed();

  // OTA handlers
  VerifyWifi();
  ArduinoOTA.handle();
  if (WEB_DASHBOARD)
    server.handleClient();

  ConnectToServer();
  Serial.println("Asking for a new job for user: " + String(USERNAME));

#ifndef USE_DHT
  client.print("JOB," +
               String(USERNAME) + SEP_TOKEN +
               String(START_DIFF) + SEP_TOKEN +
               String(MINER_KEY) + END_TOKEN);
#endif

#ifdef USE_DHT
  temp = dht.readTemperature();
  hum = dht.readHumidity();

  Serial.println("DHT readings: " + String(temp) + "*C, " + String(hum) + "%");
  client.print("JOB," +
               String(USERNAME) + SEP_TOKEN +
               String(START_DIFF) + SEP_TOKEN +
               String(MINER_KEY) + SEP_TOKEN +
               String(temp) + "@" + String(hum) + END_TOKEN);
#endif

#ifdef USE_MQTT

  if (!mqttClient.connected())
  {
    mqttReconnect();
  }
  mqttClient.loop();
#ifdef USE_DHT
  long now = millis();
  if (now - lastMsg > mqtt_update_time)
  {
    lastMsg = now;
    mqttClient.publish(temperature_topic, String(temp).c_str(), true);
    mqttClient.publish(humidity_topic, String(hum).c_str(), true);
  }
#endif

#endif

  waitForClientData();
  String last_block_hash = getValue(client_buffer, SEP_TOKEN, 0);
  String expected_hash = getValue(client_buffer, SEP_TOKEN, 1);
  difficulty = getValue(client_buffer, SEP_TOKEN, 2).toInt() * 100 + 1;

  int job_len = last_block_hash.length() + expected_hash.length() + String(difficulty).length();
  Serial.println("Received job with size of " + String(job_len) + " bytes");
  expected_hash.toUpperCase();
  br_sha1_init(&sha1_ctx_base);
  br_sha1_update(&sha1_ctx_base, last_block_hash.c_str(), last_block_hash.length());

  float start_time = micros();
  max_micros_elapsed(start_time, 0);

  String result = "";
  digitalWrite(LED_BUILTIN, HIGH);
  for (unsigned int duco_numeric_result = 0; duco_numeric_result < difficulty; duco_numeric_result++)
  {
    // Difficulty loop
    sha1_ctx = sha1_ctx_base;
    duco_numeric_result_str = String(duco_numeric_result);
    br_sha1_update(&sha1_ctx, duco_numeric_result_str.c_str(), duco_numeric_result_str.length());
    br_sha1_out(&sha1_ctx, hashArray);
    result = experimental::TypeConversion::uint8ArrayToHexString(hashArray, 20);
    if (result == expected_hash)
    {
      // If result is found
      unsigned long elapsed_time = micros() - start_time;
      float elapsed_time_s = elapsed_time * .000001f;
      hashrate = duco_numeric_result / elapsed_time_s;
      share_count++;
      blink(BLINK_SHARE_FOUND);
      client.print(String(duco_numeric_result) + "," + String(hashrate) + "," + String(MINER_BANNER) + " " + String(MINER_VER) + "," + String(RIG_IDENTIFIER) + ",DUCOID" + String(chipID) + "\n");

      waitForClientData();
      Serial.println(client_buffer + " share #" + String(share_count) + " (" + String(duco_numeric_result) + ")" + " hashrate: " + String(hashrate / 1000, 2) + " kH/s (" + String(elapsed_time_s) + "s)");
      break;
    }
    if (max_micros_elapsed(micros(), 500000))
    {
      handleSystemEvents();
    }
    else
    {
      delay(0);
    }
  }
  if (runEvery(interval)) // Calls webhook function
  {
    dataFetcher();
    webhook();
  }
}
