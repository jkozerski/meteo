// Uncomment define DEBUG line to build with debug logs (on serial)
// This line MUST be before meteo_common.h include to make any effect
//#define DEBUG

#include <DHT.h>  // Temp and humidity
#include <VirtualWire.h> // 433MHz tramsmiter
#include <meteo_common.h> // Common function for meteo


/*
Used Arduino ports:

| Port |       Purpose     | Desc.                         |
+------+-------------------+-------------------------------+
|   2  |            DHT22  | Temp & Humid sensor           |
|   4  |  433MHz tramsmit. | 433MHz tramsmiter             |
+------+-------------------+-------------------------------+
| *GND |           2 x GND | DHT22, 433MHz trans.          |
+------+-------------------+-------------------------------+
| *+5V |  2 x power supply | DHT22, 433MHz trans.          |
+------+-------------------+-------------------------------+
 (*) Ground and +5V can be taken directly from power supply
*/


// Temp + Humidity
#define DHT_PIN  2
#define DHT_TYPE DHT22
DHT dht(DHT_PIN, DHT_TYPE);


// 433MHz Transmiter
#define TRANSMITER_PIN 4 // was 12
void send_data (float temp_out, float humid_out);

 
void setup ()
{
    #ifdef DEBUG
    // Open serial port for debug
    Serial.begin(9600);
    LOGLN("Setup begin");
    #endif

    pinMode(DEBUG_LED, OUTPUT);
    digitalWrite(DEBUG_LED, LOW); // turn off debug LED

    // Wait 2s - just in case, DHT22 may need this time after power off
    delay(2000);

    // setup transmiter
    vw_set_tx_pin(TRANSMITER_PIN);
    vw_setup(2000);

    dht.begin();  // initalize temperature and humidity sensor
    LOGLN("Setup end");
}

void loop ()
{
    LOGLN("Loop");

    float temp_out  = dht.readTemperature();
    float humid_out = dht.readHumidity();
 
    // Check values
    if (isnan(temp_out) || isnan(humid_out))
    {
        // blink LED on error
        error_blink(3);
        LOGLN("temp_out or humid_out is NaN");
    }
    else {
        digitalWrite(DEBUG_LED, HIGH);
        send_data (temp_out, humid_out);
        digitalWrite(DEBUG_LED, LOW);
    }

    delay(delay_time);
}


void send_data (float temp_out, float humid_out)
{
    LOGLN("send_data begin");

    const int prime = 251;
    int checksum = (((int(temp_out * 10.0)) + 300) + int(humid_out)) % prime;
    char chk = (char)checksum;

    /*
    Message format is:
    T[temp_val*10]H[humid_val*10];[checksum];
    ie.:
    No checksum  |  With checksum
    -------------+----------------
    T-200H700;   |  T-200H700;'\170'  ->  T-200H700;¬
    T-12H1000;   |  T-12H1000;'\137'  ->  T-12H1000;ë
    T0H210;      |  T0H210;'\70'      ->  T0H210;F
    T153H550;    |  T153H550;'\6'     ->  (no pritable checksum)
    T500H800;    |  T500H800;'\127'   ->  (no pritable checksum)

    checksum = (temp_val * 10 + humid_val) % 251
    checksum is one byte long
    */

    String temp(int (temp_out * 10.0));
    String humid(int (humid_out * 10.0));

    //String toSend("T" +  temp + "H" + humid + ";");
    String toSend("T" +  temp + "H" + humid + ";" + chk);

                                                         //12345678901234
    char msg[14]; // 13 char is long enough to any values "T-300H1000;C"
    toSend.toCharArray(msg, toSend.length() + 1);

    vw_send((uint8_t *)msg, strlen(msg)); // send
    vw_wait_tx();

    LOGLN("send_data end");
}

