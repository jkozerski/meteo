// Uncomment define DEBUG line to build with debug logs (on serial)
// This line MUST be before meteo_common.h include to make any effect
//#define DEBUG

#include <DHT.h>  // Temp and humidity
#include <Wire.h> // Pressure
#include <Adafruit_BMP085.h> // Pressure
#include <meteo_common.h> // Common function for meteo
#include <LiquidCrystal_I2C.h> // LCD display

/*
Used Arduino ports:

| Port |       Purpose     | Desc.                            |
+------+-------------------+----------------------------------+
|   2  |            DHT22  | Temp & Humid sensor (inside)     |
|   4  |            DHT22  | Temp & Humid sensor (outside)    |
+------+-------------------+----------------------------------+
|  A5  |      BMP180 (SCL) | Air pressure sensor (SCL)        |
|  A5  |     HD44780 (SCL) | LCD Display (SCL)                |
|  A5  |      DS3231 (SCL) | RTC Clock (SCL)                  |  
+------+-------------------+----------------------------------+
|  A4  |      BMP180 (SDA) | Air pressure sensor (SDA)        |
|  A4  |     HD44780 (SDA) | LCD Display (SDA)                |
|  A4  |      DS3231 (SDA) | RTC Clock (SDA)                  |
+------+-------------------+----------------------------------+
| *GND |           5 x GND | 2xDHT22, BMP180, HD44780, DS3231 |
+------+-------------------+----------------------------------+
| *+5V |  5 x power supply | 2xDHT22, BMP180, HD44780, DS3231 |
+------+-------------------+----------------------------------+
 (*) Ground and +5V can be taken directly from power supply
*/

/*
Display configuration

 01234567890123456789
+--------------------+
|In:  Err °C  Err% RH|
|In: -20.5°C  100% RH|
|Out:-20.5°C  100% RH|
|          1013,9 hPa|
|12:34:56  12.12.2017|
+--------------------+
 */


// LEDs
#define DEBUG_LED          13

// Temp + Humidity inside
#define DHT_IN_PIN  2
#define DHT_IN_TYPE DHT22
DHT dht_in(DHT_IN_PIN, DHT_IN_TYPE);

// Temp + Humidity outside
#define DHT_OUT_PIN  2
#define DHT_OUT_TYPE DHT22
DHT dht_out(DHT_OUT_PIN, DHT_OUT_TYPE);


// Pressure (BMP180 used)
Adafruit_BMP085 bmp;


// LCD Display
LiquidCrystal_I2C lcd(0x27, 20, 4);  // set the LCD address to 0x27 for a 20 chars and 4 line display


// TODO: Add RTC


// Error value
const int32_t err_val = 200000;

void setup ()
{
    #ifdef DEBUG
    // Open serial port for debug
    Serial.begin(9600);
    LOGLN("Setup begin");
    #endif

    // LCD
    lcd.init();

    // LEDs
    pinMode(DEBUG_LED, OUTPUT);
    digitalWrite(DEBUG_LED, LOW);

    lcd.backlight();
    //lcd.noBacklight();
    //lcd.noCursor();
    //lcd.noBlink()
    lcd.setCursor(5, 0);
    lcd.print("Witamy!");
    lcd.setCursor(1, 1);
    lcd.print("Stacja pogodowa");
    lcd.setCursor(7, 2);
    lcd.print("wersja D_0.1");

    // Sleep for setup dht sensors
    delay(2000);

    // initalize temperature and humidity sensor
    dht_in.begin();
    dht_out.begin();

    if (!bmp.begin(3)) { // init pressure sensor with high precission param - 3
        LOGLN("Cannot initalize BMP (pressure) sensor");
        error_blink(5);
        digitalWrite(DEBUG_LED, HIGH); // if error turn debug LED on
        delay(5000);
        digitalWrite(DEBUG_LED, LOW);
    }

    LOGLN("Setup end");
}

void draw_template()
{
    lcd.setCursor(0, 0);
    lcd.print("In:      °C     % RH");
    lcd.setCursor(0, 1);
    lcd.print("Out:     °C     % RH");
    lcd.setCursor(0, 2);
    lcd.print("                 hPa");
    lcd.setCursor(0, 3);
    lcd.print("                    ");
}

void set_error (int cur, int line)
{
    lcd.setCursor(cur, line);
    lcd.print("Err");
}

void fill_data(float temp_in, float temp_out, float humid_in, float humid_out, int32_t pressure)
{
    int cur;
    char buff[6];

    if (temp_in >= err_val) {
        set_error(5, 0);
    }
    else {
        cur = 6;
        if (temp_in < 0)
            cur--;
        if (temp_in <= -10.0 || temp_in >= 10.0)
            cur--;
        lcd.setCursor(cur, 0);
        //lcd.print("%.1f", temp_in);
        dtostrf(temp_in, 3, 1, buff);
        lcd.print(buff);
    }


    if (humid_in >= err_val) {
        set_error(13, 0);
    }
    else {
        cur = 15;
        if (humid_in >= 10.0)
            cur--;
        if (humid_in >= 100.0)
            cur--;
        lcd.setCursor(cur, 0);
        //lcd.print("%.0f", humid_in);
        dtostrf(humid_in, 1, 0, buff);
        lcd.print(buff);
    }


    if (temp_out >= err_val) {
        set_error(5, 1);
    }
    else {
        cur = 6;
        if (temp_out < 0)
            cur--;
        if (temp_out <= -10.0 || temp_out >= 10.0)
            cur--;
        lcd.setCursor(cur, 1);
        //lcd.print("%.1f", temp_out);
        dtostrf(temp_out, 3, 1, buff);
        lcd.print(buff);
    }

    if (humid_out >= err_val) {
        set_error(13, 1);
    }
    else {
        cur = 15;
        if (humid_out >= 10.0)
            cur--;
        if (humid_out >= 100.0)
            cur--;
        lcd.setCursor(cur, 1);
        //lcd.print("%.0f", humid_out);
        dtostrf(humid_out, 1, 0, buff);
        lcd.print(buff);
    }

    cur = 11;
    if (pressure >= 100000)
        cur--;
    lcd.setCursor(cur, 2);
    //lcd.print("%.1f", (float) pressure/100.0);
    dtostrf(pressure/100.0, 5, 1, buff);
    lcd.print(buff);
}

void loop ()
{
    LOGLN("Loop");
//    error_blink(1);
    float temp_in   = dht_in.readTemperature();
    float humid_in  = dht_in.readHumidity();
    float temp_out  = dht_out.readTemperature();
    float humid_out = dht_out.readHumidity();
    int32_t pressure = bmp.readPressure();


    // ##### Temperature inside #####
    // Check value
    if (isnan(temp_in)) {
        // blink LED on error
        LOGLN("temp_in is NaN");
        error_blink(1);
        temp_in = err_val; //ERR
    }
    else { // Temp in OK
      
    }

    
    // ##### Temperature outside #####
    // Check value
    if (isnan(temp_out)) {
        // blink LED on error
        LOGLN("temp_out is NaN");
        error_blink(1);
        temp_out = err_val; //ERR
    }
    else { // Temp out OK
      
    }


    // ##### Humidity inside #####
    // Check value
    if(isnan(humid_in)) {
        // blink LED on error
        LOGLN("humid_in is NaN");
        error_blink(1);
        humid_in = err_val; //ERR
    }


    // ##### Humidity outside #####
    // Check value
    if(isnan(humid_out)) {
        // blink LED on error
        LOGLN("humid_out is NaN");
        error_blink(1);
        humid_out = err_val; //ERR
    }


    // ##### Pressure #####
    // Nothing to do here


    draw_template();
    fill_data(temp_in, temp_out, humid_in, humid_out, pressure);

    
    LOG("delay "); LOGLN(delay_time);
    delay(delay_time);
}

