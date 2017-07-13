// Uncomment define DEBUG line to build with debug logs (on serial)
// This line MUST be before log.h include to make any effect
//#define DEBUG

#include <DHT.h>  // Temp and humidity
#include <Wire.h> // Pressure
#include <Adafruit_BMP085.h> // Pressure
#include <log.h> // Serial log support
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
#define DEBUG_LED    13

// Buttons
#define BUTTON_OK     3
#define BUTTON_UP     5
#define BUTTON_DOWN   6

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
const int32_t delay_time = 100; // [ms]
const int32_t meteo_delay = 2000; // [ms] Update meteo data every 2s
int32_t delay_counter = 0;

// Error value
const int32_t err_val = 200000;

////////////////////////////////////////////////////////////////////////////////

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

////////////////////////////////////////////////////////////////////////////////

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

////////////////////////////////////////////////////////////////////////////////

void set_error (int cur, int line)
{
    lcd.setCursor(cur, line);
    lcd.print("Err");
}

////////////////////////////////////////////////////////////////////////////////

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

////////////////////////////////////////////////////////////////////////////////

/* implements hysteresis */
bool was_button_pressed (unsigned pin)
{
    const int default_state = HIGH; /*assume that button is pull-up*/

    if (digitalRead(pin) != default_state) {
        delay(50); /*ms*/
        if (digitalRead(pin) != default_state) {
            do {} while (digitalRead(pin) == !default_state); // wait for button releases
            return true;
        }
    }
    return false;
}

////////////////////////////////////////////////////////////////////////////////

/* check if should enter the time_setup finction */
bool enter_time_setup()
{
    const int default_state = HIGH; /*assume that button is pull-up*/

    if (digitalRead(BUTTON_UP)   != default_state &&
        digitalRead(BUTTON_DOWN) != default_state &&
        digitalRead(BUTTON_OK)   != default_state) {
        delay(50); /*ms*/
        if (digitalRead(BUTTON_UP)   != default_state &&
            digitalRead(BUTTON_DOWN) != default_state &&
            digitalRead(BUTTON_OK)   != default_state) {
                do {} while (digitalRead(BUTTON_UP)   != default_state ||
                             digitalRead(BUTTON_DOWN) != default_state ||
                             digitalRead(BUTTON_OK)   != default_state);
                return true;
            }
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////

void time_setup(unsigned line)
{
    int pos = 0;
    /* possition:
     * 0 - hour
     * 1 - minute
     * 2 - second
     * 3 - day
     * 4 - month
     * 5 - year
     * 6 - exit;
    */

    byte val;
    int change = 0;
    bool tmp1, tmp2;

    lcd.cursor();
    lcd.blink();

    val = rtc.getHour(tmp1, tmp2);
    lcd.setCursor(0, line);
    while (true) {

        change = 0;
        if (was_button_pressed(BUTTON_UP))
            change = 1;
        if (was_button_pressed(BUTTON_DOWN))
            change = -1;
        if (was_button_pressed(BUTTON_OK)) {
            pos++;
            switch (val) {
                case 1: val = rtc.getMinute();    lcd.setCursor(3, line);  break;
                case 2: val = rtc.getSecond();    lcd.setCursor(6, line);  break;
                case 3: val = rtc.getDate();      lcd.setCursor(10, line); break;
                case 4: val = rtc.getMonth(tmp1); lcd.setCursor(13, line); break;
                case 5: val = rtc.getYear();      lcd.setCursor(18, line); break;
                default: break;
            }
            if (val > 5) { //exit setup function
              break;
            }
            continue;
        }

        if (change == 0)
            continue; // Nothig to do;

        val += change;
        switch (pos) {
            case 0:
                if (val < 0)  val = 23;
                if (val > 23) val = 0;
                rtc.setHour(val);
                lcd.setCursor(0, line);
                lcd.print(val);
                break;
            case 1:
                if (val < 0)  val = 59;
                if (val > 59) val = 0;
                rtc.setMinute(val);
                lcd.setCursor(3, line);
                lcd.print(val);
                break;
            case 2:
                if (val < 0)  val = 59;
                if (val > 59) val = 0;
                rtc.setSecond(val);
                lcd.setCursor(6, line);
                lcd.print(val);
                break;
            case 3:
                if (val < 0)  val = 31; // this allows to set 31.02 as a date
                if (val > 31) val = 0;
                rtc.setDate(val);
                lcd.setCursor(10, line);
                lcd.print(val);
                break;
            case 4:
                if (val < 0)  val = 12;
                if (val > 12) val = 0;
                rtc.setDate(val);
                lcd.setCursor(16, line);
                lcd.print(val);
                break;
            case 5:
                if (val < 0)  val = 99;
                if (val > 99) val = 0;
                rtc.setYear(val);
                lcd.setCursor(18, line);
                lcd.print(val);
                break;
            default: break;
        }
        if (pos > 5) break; // exit loop;
    }

    lcd.noCursor();
    lcd.noBlink();
}

void read_meteo_data(float &temp_in,  float &humid_in,
                     float &temp_out, float &humid_out,
                     int32_t &pressure)
{
    temp_in   = dht_in.readTemperature();
    humid_in  = dht_in.readHumidity();
    temp_out  = dht_out.readTemperature();
    humid_out = dht_out.readHumidity();
    pressure  = bmp.readPressure();


    // ##### Temperature inside #####
    // Check value
    if (isnan(temp_in)) {
        // blink LED on error
        LOGLN("temp_in is NaN");
        error_blink(1);
        temp_in = err_val; //ERR
    }
    else {
        LOG("Temp in: "); LOGLN(temp_in);
    }


    // ##### Temperature outside #####
    // Check value
    if (isnan(temp_out)) {
        // blink LED on error
        LOGLN("temp_out is NaN");
        error_blink(1);
        temp_out = err_val; //ERR
    }
    else {
        LOG("Temp out: "); LOGLN(temp_out);
    }


    // ##### Humidity inside #####
    // Check value
    if(isnan(humid_in)) {
        // blink LED on error
        LOGLN("humid_in is NaN");
        error_blink(1);
        humid_in = err_val; //ERR
    }
    else {
        LOG("Humid in: "); LOGLN(humid_in);
    }


    // ##### Humidity outside #####
    // Check value
    if(isnan(humid_out)) {
        // blink LED on error
        LOGLN("humid_out is NaN");
        error_blink(1);
        humid_out = err_val; //ERR
    }
    else {
        LOG("Humid out: "); LOGLN(humid_out);
    }


    // ##### Pressure #####
    // Nothing to do here
    LOG("Pressuren: "); LOGLN(pressure);
}

////////////////////////////////////////////////////////////////////////////////

void setup ()
{
    LogInit(9600);

    LOGLN("Setup begin");

    // LEDs
    pinMode(DEBUG_LED, OUTPUT);
    digitalWrite(DEBUG_LED, LOW);

    // Buttons
    pinMode(BUTTON_OK, INPUT_PULLUP);
    pinMode(BUTTON_UP, INPUT_PULLUP);
    pinMode(BUTTON_DOWN, INPUT_PULLUP);

    // Configure RTC
    rtc.enableOscillator(true /* ON */,
                        false /* disabled if batery-only powered */,
                        0 /* freq: 1Hz */);
    rtc.enable32kHz(false);
    rtc.setClockMode(false); /* set to 24-hour mode */

    // LCD
    lcd.init();

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

    // Temperature and humidity sensor
    dht_in.begin();
    dht_out.begin();

    // Pressure sensor
    if (!bmp.begin(3)) { // init pressure sensor with high precission param - 3
        LOGLN("Cannot initalize BMP (pressure) sensor");
        error_blink(5);
        digitalWrite(DEBUG_LED, HIGH); // if error turn debug LED on
        delay(5000);
        digitalWrite(DEBUG_LED, LOW);
    }

    LOGLN("Setup end");
}

////////////////////////////////////////////////////////////////////////////////

void loop ()
{
    LOGLN("Loop");
//    error_blink(1);

    if (enter_time_setup())
        time_setup(3 /* 4th row */);

    // Update meteo data after every 'meteo_delay' time
    if (delay_counter == 0) {
        float temp_in, humid_in, temp_out, humid_out;
        int32_t pressure;

        read_meteo_data(temp_in, humid_in, temp_out, humid_out, pressure);
        draw_template();
        fill_data(temp_in, temp_out, humid_in, humid_out, pressure);
    }

    // update delay_counter
    delay_counter = (delay_counter + 1) % (meteo_delay / delay_time);


    // Update the displayed time
    print_time_string(3 /* 4th row */);


    LOG("delay "); LOGLN(delay_time);
    delay(delay_time);
}

