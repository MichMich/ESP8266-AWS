/**
 * This is an example sketch to connect your ESP8266 to the AWS IoT servers.
 * Check http://michaelteeuw.nl for more information.
 *
 * Don't forget to add your certificate and key to the data directory
 * and upload your spiffs (data) folder using the following terminal command:
 *
 * platformio run --target uploadfs
 */

#include <Arduino.h>
#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include "FS.h"

// Set your Wifi Network & Password
// And define your AWS endpoint which you can find
// in the AWS IoT core console.
const char* ssid = "WifiNetwork";
const char* password = "WifiPassword";
const char* AWSEndpoint = "a1ktbwyheeinn0-ats.iot.eu-west-1.amazonaws.com";

/**
 * Add your ssl certificate and ssl key to the data folder. (You can remove the example files.)
 * After adding the files, upload them to your ESP8266 with the following terminal command:
 *
 * platformio run --target uploadfs
 */
const char* certFile = "/edf80b7abd-certificate.pem.crt";
const char* keyFile = "/edf80b7abd-private.pem.key";

// The following two variables are used for the example outTopic messages.
unsigned int counter = 0;
unsigned long lastPublished = 0;

/**
 * Callback to handle the incoming MQTT messages.
 */
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (unsigned int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }

  Serial.println();
}

/**
 * Function to (re)connect to the MQTT server.
 */
void connectMqtt() {
  // As long as there is no connection, try to connect.
  while (!client.connected()) {
    Serial.print("Connecting to MQTT server ...");

    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);

    // Attempt to connect
    delay(500);

    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      client.subscribe("inTopic");
      client.publish("outTopic", "Connected!");
    } else {
      Serial.print(" Failed, rc=");
      Serial.print(client.state());
      Serial.println(". Try again in 3 seconds ...");
      delay(3000);
    }
  }
}

// Initialize espClient and (mqtt) client instances.
// Note that we use `WiFiClientSecure` in stead of `WiFiClient`

WiFiClientSecure espClient;
PubSubClient client(AWSEndpoint, 8883, callback, espClient);

/**
 * Setup function.
 */
void setup() {
  Serial.begin(115200);

  // Connect to Wifi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi Connected!");

  // Mount file system.
  if (!SPIFFS.begin()) Serial.println("Failed to mount file system");

  // Allows for 'insecure' TLS connections. Of course this is totally insecure,
  // but seems the only way to connect to IoT. So be cautious with your data.
  espClient.setInsecure();

  // Read the SSL certificate from the spiffs filesystem and load it.
  File cert = SPIFFS.open(certFile, "r");
  if (!cert) Serial.println("Failed to open certificate file: " + String(certFile));
  if(espClient.loadCertificate(cert)) Serial.println("Certificate loaded!");

  // Read the SSL key from the spiffs filesystem and load it.
  File key = SPIFFS.open(keyFile, "r");
  if (!key) Serial.println("Failed to open private key file: " + String(keyFile));
  if(espClient.loadPrivateKey(key)) Serial.println("Private key loaded!");
}

/**
 * Main run loop
 */
void loop() {
  // Make sure MQTT is connected and run the `loop` method to check for new data.
  if (!client.connected()) connectMqtt();
  client.loop();

  // Publish a message every second.
  if (millis() > lastPublished + 1000) {
    String message = "Hello world! - " + String(counter++);

    client.publish("outTopic", message.c_str());
    Serial.println("Message published [outTopic] " + message);
    lastPublished = millis();
  }
}
