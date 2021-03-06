// Uncomment define DEBUG line to build with debug logs (on serial)
// This line MUST be before meteo_common.h include to make any effect
#define DEBUG

#include <DHT.h>  // Temp and humidity
#include <meteo_common.h> // Common function for meteo


/*
Used Arduino ports:

| Port |       Purpose     | Desc.                         |
+------+-------------------+-------------------------------+
|   3  |      temp out IND | PWM - temp out indicator      |
|   5  |     humid out IND | PWM - humid out indicator     |
|   6  |  low temp out LED |                               |
|   7  | high temp out LED |                               |
|   8  |            DHT22  | Temp & Humid sensor           |
+------+-------------------+-------------------------------+
| *GND |           5 x GND | 2xLED, 2xIND, DHT22.          |
+------+-------------------+-------------------------------+
| *+5V |  1 x power supply | DHT22.                        |
+------+-------------------+-------------------------------+
 (*) Ground and +5V can be taken directly from power supply
*/


// LEDs
#define LOW_TEMP_OUT_LED   6
#define HIGH_TEMP_OUT_LED  7


// INDICATORS (need PWM)
#define IND_TEMP_OUT   3
#define IND_HUMID_OUT  5


// Temp + Humidity
#define DHT_PIN  8
#define DHT_TYPE DHT22
DHT dht(DHT_PIN, DHT_TYPE);


// Calibration of indicators - values on which the pointer reach maximum on scale.
// Notice: zero value should be (manually) calibrated to point 0 on scale.
const int calib_max_temp  = 248;
const int calib_max_humid = 241;

void setup ()
{
    #ifdef DEBUG
    // Open serial port for debug
    Serial.begin(9600);
    LOGLN("Setup begin");
    #endif

    // LEDs
    pinMode(DEBUG_LED,         OUTPUT);
    pinMode(LOW_TEMP_OUT_LED,  OUTPUT);
    pinMode(HIGH_TEMP_OUT_LED, OUTPUT);

    // Indicators
    pinMode(IND_TEMP_OUT,  OUTPUT);
    pinMode(IND_HUMID_OUT, OUTPUT);

    //diagnostic setup
    digitalWrite(LOW_TEMP_OUT_LED,  HIGH);
    digitalWrite(HIGH_TEMP_OUT_LED, HIGH);
    int tab[]       = {IND_TEMP_OUT, IND_HUMID_OUT, -1};
    int tab_calib[] = {calib_max_temp, calib_max_humid, -1};
    diagnostic_setup(tab, tab_calib);
    digitalWrite(LOW_TEMP_OUT_LED,  LOW);
    digitalWrite(HIGH_TEMP_OUT_LED, LOW);

    // initalize temperature and humidity sensor
    dht.begin();

    // turn off debug LED
    digitalWrite(DEBUG_LED, LOW);

    LOGLN("Setup end");
}


void loop ()
{
    LOGLN("Loop");
    error_blink(1);

    float temp_out  = dht.readTemperature();
    float humid_out = dht.readHumidity();
    long val;

    // ##### Temperature #####
    // Check value
    if (isnan(temp_out)) {
        // blink LED on error
        LOGLN("temp_out is NaN");
        error_blink(3);
        temp_out = 0.0;
    }
    
    // Check if temp value is in range, and fix it if needed
    if (temp_out < temp_out_min)
        temp_out = temp_out_min;
    else if (temp_out > temp_out_max)
        temp_out = temp_out_max;

    // Check proper range of indicator, map the value, and set value on indicator
    // temp out
    LOG("temp_out: "); LOGLN(temp_out);
    if (temp_out < temp_out_1) {
        LOGLN("temp_out range 1");
        digitalWrite(LOW_TEMP_OUT_LED, HIGH);
        digitalWrite(HIGH_TEMP_OUT_LED, LOW);
        val = ind_map10_cal(temp_out, temp_out_min, temp_out_1, calib_max_temp);
    }
    else if (temp_out < temp_out_2){
        LOGLN("temp_out range 2");
        digitalWrite(LOW_TEMP_OUT_LED, LOW);
        digitalWrite(HIGH_TEMP_OUT_LED, LOW);
        val = ind_map10_cal(temp_out, temp_out_1, temp_out_2, calib_max_temp);
    }
    else { // temp_out >= temp_out_2
        LOGLN("temp_out range 3");
        digitalWrite(LOW_TEMP_OUT_LED, LOW);
        digitalWrite(HIGH_TEMP_OUT_LED, HIGH);
        val = ind_map10_cal(temp_out, temp_out_2, temp_out_max, calib_max_temp);
    }
    analogWrite(IND_TEMP_OUT, val);


    // ##### Humidity #####
    // Check value
    if(isnan(humid_out)) {
        // blink LED on error
        LOGLN("humid_out is NaN");
        error_blink(4);
        humid_out = humid_min;
    }
    // Humidity is always in range
    // Map the value, and set value on indicator
    LOG("humid_out: "); LOGLN(humid_out);
    val = ind_map10_cal(humid_out, humid_min, humid_max, calib_max_humid);
    analogWrite(IND_HUMID_OUT, val);


    delay(delay_time);
}

