// Uncomment define DEBUG line to build with debug logs (on serial)
// This line MUST be before meteo_common.h include to make any effect
//#define DEBUG

#include <DHT.h>  // Temp and humidity
#include <Wire.h> // Pressure
#include <Adafruit_BMP085.h> // Pressure
#include <meteo_common.h> // Common function for meteo
#include <LiquidCrystal_I2C.h> // LCD display
#include <DS3231.h>

/*
Used Arduino ports:

| Port |       Purpose     | Desc.                            |
+------+-------------------+----------------------------------+
|   2  |            DHT22  | Temp & Humid sensor (inside)     |
|   4  |            DHT22  | Temp & Humid sensor (outside)    |
|   3  |           Button  | OK button                        |
|   5  |           Button  | Up button                        |
|   6  |           Button  | Down button                      |
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
|Pressure:    Err hPa|
|Pressure: 1013.9 hPa|
|17:34:59  31.12.2017|
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


// Real-time clock
DS3231 rtc;

// Delay
const int32_t sleep_time = 1000; // 1000ms

// Error value
const int32_t err_val = 200000;


/* returns time-date string via param from rtc.
 * rtc should be at least 21 bytes long.
 */
void print_time_string(unsigned row)
{
    bool tmp1 = false;
    bool tmp2 = false;

    char str[21];
    byte ret;

    /*
     * 01234567890123456789
     * 17:34:59  31.12.2017
     */

    // hour
    ret = rtc.getHour(tmp1, tmp2);
    str[0] = ret / 10;
    str[1] = ret % 10;

    str[2] = ':';

    // minute
    ret = rtc.getMinute();
    str[3] = ret / 10;
    str[4] = ret % 10;

    str[5] = ':';

    // second
    ret = rtc.getSecond();
    str[6] = ret / 10;
    str[7] = ret % 10;

    str[8] = ' ';
    str[9] = ' ';

    // day
    ret = rtc.getDate();
    str[10] = ret / 10;
    str[11] = ret % 10;

    str[12] = '.';

    // month
    ret = rtc.getMonth(tmp1);
    str[13] = ret / 10;
    str[14] = ret % 10;

    str[15] = '.';

    // year - only last two digits
    ret = rtc.getYear();
    str[16] = '2';
    str[17] = '0';
    str[18] = ret / 10;
    str[19] = ret % 10;

    // end of string
    str[20] = '\0';

    lcd.setCursor(0, row);
    lcd.print(str);
}

void setup ()
{
    #ifdef DEBUG
    // Open serial port for debug
    Serial.begin(9600);
    LOGLN("Setup begin");
    #endif

    // Configure RTC
    rtc.enableOscillator(true /* ON */,
                        false /* disabled if batery-only powered */,
                        0 /* freq: 1Hz */);
    rtc.enable32kHz(false);
    rtc.setClockMode(false); /* set to 24-hour mode */

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
    lcd.print("Pressure:    Err hPa");
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
    char buff[8];

/*
 01234567890123456789
+--------------------+
|In:  Err °C  Err% RH|
|In: -20.5°C  100% RH|
|Out:-20.5°C  100% RH|
|Pressure:    Err hPa|
|Pressure: 1013.9 hPa|
|17:34:59  31.12.2017|
+--------------------+
 */

    // Temp inside
    if (temp_in >= err_val) {
        set_error(5, 0);
    }
    else {
        lcd.setCursor(4, 0);
        dtostrf(temp_in, 5, 1, buff);
        lcd.print(buff);
    }

    // Humid inside
    if (humid_in >= err_val) {
        set_error(13, 0);
    }
    else {
        lcd.setCursor(13, 0);
        dtostrf(humid_in, 3, 0, buff);
        lcd.print(buff);
    }

    // Temp outside
    if (temp_out >= err_val) {
        set_error(5, 1);
    }
    else {
        lcd.setCursor(4, 1);
        dtostrf(temp_out, 5, 1, buff);
        lcd.print(buff);
    }

    // Humid outside
    if (humid_out >= err_val) {
        set_error(13, 1);
    }
    else {
        lcd.setCursor(13, 1);
        dtostrf(humid_out, 3, 0, buff);
        lcd.print(buff);
    }

    // Pressure
    lcd.setCursor(10, 2);
    dtostrf(pressure/100.0, 6, 1, buff);
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

    
    // ##### Temperature outside #####
    // Check value
    if (isnan(temp_out)) {
        // blink LED on error
        LOGLN("temp_out is NaN");
        error_blink(1);
        temp_out = err_val; //ERR
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
    print_time_string(3 /* 4th row */);

    
    LOG("delay "); LOGLN(sleep_time);
    delay(sleep_time);
}

