/*
 * ESP8266 (Adafruit HUZZAH) Mosquitto MQTT Publish Example
 * Thomas Varnish (https://github.com/tvarnish), (https://www.instructables.com/member/Tango172)
 * Made as part of my MQTT Instructable - "How to use MQTT with the Raspberry Pi and ESP8266"
 * https://www.instructables.com/id/How-to-Use-MQTT-With-the-Raspberry-Pi-and-ESP8266/
 *
 * Additional Boards Manager URL:
 * http://arduino.esp8266.com/stable/package_esp8266com_index.json
 */
#include <ESP8266WiFi.h> // Enables the ESP8266 to connect to the local network (via WiFi)
#include <PubSubClient.h> // Allows us to connect to, and publish to the MQTT broker


// WiFi
// Make sure to update this for your own WiFi network!
const char* ssid = ""; //FIXME
const char* wifi_password = ""; //FIXME

// MQTT
// Make sure to update this for your own MQTT Broker!
const char* mqtt_server = ""; //FIXME
const char* mqtt_topic = "meteo";
const char* mqtt_username = "meteo";
const char* mqtt_password = ""; //FIXME
// The client id identifies the ESP8266 device. Think of it a bit like a hostname (Or just a name, like Greg).
const char* clientID = "Meteo_LCD";


// Variables for main loop:
#define BUFF_LEN 64
char message[BUFF_LEN];
int buff_iter;
int buff_status;

// Initialise the Pushbutton Bouncer object

// Initialise the WiFi and MQTT Client objects
WiFiClient wifiClient;
PubSubClient client(mqtt_server, 1883, wifiClient); // 1883 is the listener port for the Broker

void setup()
{
    buff_iter = 0;
    buff_status = 0;

    // Begin Serial on 115200
    // Remember to choose the correct Baudrate on the Serial monitor!
    // This is just for debugging purposes
    Serial.begin(115200);

    Serial.print("Connecting to ");
    Serial.println(ssid);

    // Connect to the WiFi
    WiFi.begin(ssid, wifi_password);

    // Wait until the connection has been confirmed before continuing
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
    }

    // Debugging - Output the IP Address of the ESP8266
    Serial.println("WiFi connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    // Connect to MQTT Broker
    // client.connect returns a boolean value to let us know if the connection was successful.
    // Make sure you are using the correct MQTT Username and Password
    if (client.connect(clientID, mqtt_username, mqtt_password)) {
        Serial.println("Connected to MQTT Broker!");
    }
    else {
        Serial.println("Connection to MQTT Broker failed...");
    }
}

void loop()
{
    while(Serial.available()) {

        if (buff_iter > BUFF_LEN - 1) {
            buff_iter = 0;
            buff_status = 0;
        }
    
        message[buff_iter] = Serial.read();

        if (buff_status == 0) {// waiting for '(' - begin of the message
            if (message[buff_iter] == '(') {
                buff_status = 1;
            }
            continue;
        }
        if (buff_status == 1) {
            if ((message[buff_iter] >= '0' && message[buff_iter] <= '9') || 
                 message[buff_iter] == '-' || message[buff_iter] == ' '  ||
                 message[buff_iter] == '.' || message[buff_iter] == ';')  { // any legal char
                 buff_iter++;
                 continue;
            }
            else if (message[buff_iter] == ')') { // end of message
                message[buff_iter] == '\0';
                buff_iter = 0;
                buff_status = 0;
                Serial.println(message);

                if (client.publish(mqtt_topic, message)) {
                    // OK
                }
                // If the message failed to send, we will try again, as the connection may have broken.
                else {
                    // ERROR
                    client.connect(clientID, mqtt_username, mqtt_password);
                    delay(10); // ensures that client.publish doesn't clash with the client.connect
                    client.publish(mqtt_topic, message);
                }
                continue;
            }
            else { // any illegal char
                buff_iter = 0;
                buff_status = 0;
            }
        }
    }
} 
