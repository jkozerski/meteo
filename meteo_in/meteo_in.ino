// Uncomment define DEBUG line to build with debug logs (on serial)
// This line MUST be before meteo_common.h include to make any effect
//#define DEBUG

#include <DHT.h>  // Temp and humidity
#include <Wire.h> // Pressure
#include <Adafruit_BMP085.h> // Pressure
#include <meteo_common.h> // Common function for meteo


/*
Used Arduino ports:

| Port |       Purpose     | Desc.                         |
+------+-------------------+-------------------------------+
|   2  |            DHT22  | Temp & Humid sensor           |
|   3  |       temp in IND | PWN - temp in indicator       |
|   4  |  low pressure LED |                               |
|   5  |      humid in IND | PWN - humid in indicator      |
|   6  |      pressure IND | PWM - air pressure indicator  |
|   7  | high pressure LED |                               |
+------+-------------------+-------------------------------+
|  A5  |      BMP180 (SCL) | Air pressure sensor (SCL)     |
|  A4  |      BMP180 (SDA) | Air pressure sensor (SDA)     |
+------+-------------------+-------------------------------+
| *GND |           7 x GND | 2xLED, 3xIND, DHT22, BMP180   |
+------+-------------------+-------------------------------+
| *+5V |  2 x power supply | DHT22, BMP180                 |
+------+-------------------+-------------------------------+
 (*) Ground and +5V can be taken directly from power supply
*/


// LEDs
#define DEBUG_LED          13
#define LOW_PRESSURE_LED   4
#define HIGH_PRESSURE_LED  7


// INDICATORS (need PWM)
#define IND_TEMP_IN    3
#define IND_HUMID_IN   5
#define IND_PRESSURE   6


// Temp + Humidity
#define DHT_PIN  2
#define DHT_TYPE DHT22
DHT dht(DHT_PIN, DHT_TYPE);


// Pressure (BMP180 used)
Adafruit_BMP085 bmp;


// Calibration of indicators - values on which the pointer reach maximum on scale.
// Notice: zero value should be (manually) calibrated to point 0 on scale.
const int calib_max_temp  = 255;
const int calib_max_humid = 255;
const int calib_max_press = 255;


void setup ()
{
    #ifdef DEBUG
    // Open serial port for debug
    Serial.begin(9600);
    LOGLN("Setup begin");
    #endif

    // LEDs
    pinMode(DEBUG_LED,         OUTPUT);
    pinMode(LOW_PRESSURE_LED,  OUTPUT);
    pinMode(HIGH_PRESSURE_LED, OUTPUT);

    // Indicators
    pinMode(IND_TEMP_IN,   OUTPUT);
    pinMode(IND_HUMID_IN,  OUTPUT);
    pinMode(IND_PRESSURE,  OUTPUT);

    //diagnostic setup
    digitalWrite(LOW_PRESSURE_LED,  HIGH);
    digitalWrite(HIGH_PRESSURE_LED, HIGH);
    int tab[] = {IND_TEMP_IN, IND_HUMID_IN, IND_PRESSURE, -1};
    diagnostic_setup(tab);
    digitalWrite(LOW_PRESSURE_LED,  LOW);
    digitalWrite(HIGH_PRESSURE_LED, LOW);

    // turn off debug LED
    digitalWrite(DEBUG_LED, LOW);

    // initalize temperature and humidity sensor
    dht.begin();  

    if (!bmp.begin(3)) { // init pressure sensor with high precission param - 3
        LOGLN("Cannot initalize BMP (pressure) sensor");
        error_blink(5);
        digitalWrite(DEBUG_LED, HIGH); // if error turn debug LED on
        delay(delay_time);
    }

    LOGLN("Setup end");
}

void loop ()
{
    LOGLN("Loop");
    error_blink(1);
    float temp_in  = dht.readTemperature();
    float humid_in = dht.readHumidity();
    int32_t pressure = bmp.readPressure();
    long val; 

    // ##### Temperature #####
    // Check value
    if (isnan(temp_in)) {
        // blink LED on error
        LOGLN("temp_in is NaN");
        error_blink(3);
        temp_in = temp_in_min;
    }
    
    // Check if temp value is in range, and fix it if needed
    if (temp_in < temp_in_min)
        temp_in = temp_in_min;
    else if (temp_in > temp_in_max)
        temp_in = temp_in_max;

    // Map the value, and set value on indicator
    LOG("temp_in: "); LOGLN(temp_in);
    val = ind_map10_cal(temp_in, temp_in_min, temp_in_max, calib_max_temp);
    analogWrite(IND_TEMP_IN, val);


    // ##### Humidity #####
    // Check value
    if(isnan(humid_in)) {
        // blink LED on error
        LOGLN("humid_in is NaN");
        error_blink(3);
        humid_in = humid_min;
    }
    // Humidity is always in range
    // Map the value, and set value on indicator
    LOG("humid_in: "); LOGLN(humid_in);
    val = ind_map10_cal(humid_in, humid_min, humid_max, calib_max_humid); 
    analogWrite(IND_HUMID_IN, val);


    // ##### Pressure #####
    // Check if pressure value is in range, and fix it if needed
    if (pressure < pressure_min)
        pressure = pressure_min;
    else if (pressure > pressure_max)
        pressure = pressure_max;

    // Check range, map the value, set value on indicator, and set LEDs
    LOG("pressure: "); LOGLN(pressure);
    if (pressure < pressure_1) {
        LOGLN("pressure range 1");
        digitalWrite(LOW_PRESSURE_LED,  HIGH);
        digitalWrite(HIGH_PRESSURE_LED, LOW);
        val = ind_map_cal(pressure, pressure_min, pressure_1, calib_max_press);
    }
    else if (pressure < pressure_2) {
        LOGLN("pressure range 2");
        digitalWrite(LOW_PRESSURE_LED,  LOW);
        digitalWrite(HIGH_PRESSURE_LED, LOW);
        val = ind_map_cal(pressure, pressure_1, pressure_2, calib_max_press);
    }
    else { // pressure >= pressure_2
        LOGLN("pressure range 3");
        digitalWrite(LOW_PRESSURE_LED,  LOW);
        digitalWrite(HIGH_PRESSURE_LED, HIGH);
        val = ind_map_cal(pressure, pressure_2, pressure_max, calib_max_press);
    }
    analogWrite(IND_PRESSURE, val);


    LOG("delay "); LOGLN(delay_time);
    delay(delay_time);
}

