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
    vw_setup(1000);

    dht.begin();  // initalize temperature and humidity sensor
    LOGLN("Setup end");
}

void loop ()
{
    LOGLN("Loop");

    float temp_out  = dht.readTemperature();
    float humid_out = dht.readHumidity();
 
//    // Check values
//    if (isnan(temp_out) || isnan(humid_out))
//    {
//        // blink LED on error
//        error_blink(3);
//        LOGLN("temp_out or humid_out is NaN");
//    }
//    else {
//        send_data (temp_out, humid_out);
//    }

    send_data1(-20, 90);
    //delay(delay_time);
}


void send_data (float temp_out, float humid_out)
{
    LOGLN("send_data begin");

    /*
    Message format is:
    T[temp_val*10 + 300]
    (for ranges -30 / 60, this gives values 0 / 900)
    and
    H[humid_val]
    ie.:
    T900
    H700
    T0
    H100
    T10
    H210

    */
    
    if (temp_out < temp_out_min)
        temp_out = temp_out_min;
    else if (temp_out > temp_out_max)
        temp_out = temp_out_max;

    String temp(int ((temp_out - temp_out_min) * 10.0));
    String humid(int (humid_out * 1));

    //String toSend("T" +  temp + "H" + humid + ";");
    String toSendT("T" + temp);
    String toSendH("H" + humid);

    char msgT[4]; // 4 char is long enough to any possible value
    toSendT.toCharArray(msgT, toSendT.length());
    
    char msgH[4]; // 4 char is long enough to any possible value
    toSendH.toCharArray(msgH, toSendH.length());

    digitalWrite(DEBUG_LED, HIGH);
    vw_send((uint8_t *)msgT, strlen(msgT)); // send temp
    vw_wait_tx();
    
    vw_send((uint8_t *)msgH, strlen(msgH)); // send humid
    vw_wait_tx();
    digitalWrite(DEBUG_LED, LOW);

    LOGLN("send_data end");
}

void send_temp (float temp_out)
{
    if (temp_out < temp_out_min)
        temp_out = temp_out_min;
    else if (temp_out > temp_out_max)
        temp_out = temp_out_max;

    char msg[2];
    msg[0] = 0;
    msg[1] = 0;
    
    int temp = ((temp_out - temp_out_min) * 10.0);
    msg[0] = temp & 0xFF;
    msg[1] = (temp >> 8);
    msg[1] &= 0x7F;
    
    digitalWrite(DEBUG_LED, HIGH);
    vw_send((uint8_t *)msg, 2); // send temp
    vw_wait_tx();
    digitalWrite(DEBUG_LED, LOW);
}

void send_humid (float humid_out)
{
    char msg[2];
    msg[0] = 0;
    msg[1] = 0;
    
    int humid = humid_out;
    msg[0] = humid & 0xFF;
    msg[1] = 0x80;
    
    digitalWrite(DEBUG_LED, HIGH);
    vw_send((uint8_t *)msg, 2); // send temp
    vw_wait_tx();
    digitalWrite(DEBUG_LED, LOW);
}

void send_data1 (float temp_out, float humid_out)
{
    if (temp_out < temp_out_min)
        temp_out = temp_out_min;
    else if (temp_out > temp_out_max)
        temp_out = temp_out_max;

    uint8_t msg[2];

    uint8_t temp = ((temp_out - temp_out_min) * 2.5); // 0 - 225
    msg[0] = temp & 0xFF;
    uint8_t humid = humid_out; // 0 - 100
    msg[1] = temp & 0xFF;

    digitalWrite(DEBUG_LED, HIGH);
    vw_send((uint8_t *)msg, 2); // send data
    vw_wait_tx();
    digitalWrite(DEBUG_LED, LOW);
}

