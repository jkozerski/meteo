#include <DHT.h>  // Temp and humidity
#include <Wire.h> // Pressure
#include <Adafruit_BMP085.h> // Pressure
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
|In:  50.5°C  100% RH|
|Out:-20.5°C  100% RH|
|DP:-70°C  1013.9 hPa|
|17:34:59  31.12.2017|
+--------------------+

+--------------------+
|In:  Err °C  Err% RH|
|Out: Err °C  Err% RH|
|DP:N/A°C     Err hPa|
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
#define DHT_OUT_PIN  4
#define DHT_OUT_TYPE DHT22
DHT dht_out(DHT_OUT_PIN, DHT_OUT_TYPE);


// Pressure (BMP180 used)
Adafruit_BMP085 bmp;


// LCD Display
LiquidCrystal_I2C lcd(0x27, 20, 4);  // set the LCD address to 0x27 for a 20 chars and 4 line display
static bool LCD_backlight;


// Real-time clock
DS3231 rtc;


// Time format
// 01234567890123456789
// 00:00:00  31.12.2017
char time_format[21] = "17:34:59  31.12.2017";
bool print_time_string(unsigned row, bool full_update = false);


// Error value
const int32_t err_val = 200000;


// Send meteo data via RS not more often than: [ms]
const unsigned long long rs_wait_time = 20L * 1000L; // 20 seconds
unsigned long old_time = 0;


// Show dew point or clock icon or none? (No place for both)
bool show_dew_point = true;

////////////////////////////////////////////////////////////////////////////////
//                DEW POINT SUPPORT

/*
 * Dew point calculated using equation:
 *
 * DP = (H/100)^(1/8) * (112 + (0.9 * T)) + (0.1 * T) - 112
 *
 * Where:
 * DP - Dew point [°C]
 * H  - relative humidity [%]
 * T  - temperature [°C]
 *
 * Should be good estimation for -30°C <= T <= 70°C
 */

/*
 * Return a dew point for temperatures in range -30°C up to +70°C.
 * Humidity should be from 0 up to 100%.
 * Functions returns esimated dew point in °C
 * Returns 100 as an error - Temperature beyond the scale.
 */
int8_t get_dew_point (float temp, float humid)
{
    if (temp < -31.0 || temp > 70)
        return 100; // treat it as an error.

    double tmp = (sqrt(sqrt(sqrt( (double)humid/100.0 ))) * (112.0 + (0.9 * (double)temp)) + (0.1 * temp) - 112.0);
    return (int8_t) (tmp + 0.5); // + 0.5 for round up
}

////////////////////////////////////////////////////////////////////////////////
//                CLOCK ICON SUPPORT
byte c0[8] = {
  B00000,
  B00100,
  B00100,
  B00100,
  B00000,
  B00000,
  B00000,
};

byte c1[8] = {
  B00000,
  B00101,
  B00110,
  B00100,
  B00000,
  B00000,
  B00000,
};

byte c2[8] = {
  B00000,
  B00100,
  B00100,
  B00111,
  B00000,
  B00000,
  B00000,
};

byte c3[8] = {
  B00000,
  B00100,
  B00100,
  B00100,
  B00010,
  B00001,
  B00000,
};

byte c4[8] = {
  B00000,
  B00100,
  B00100,
  B00100,
  B00100,
  B00100,
  B00000,
};

byte c5[8] = {
  B00000,
  B00100,
  B00100,
  B00100,
  B01000,
  B10000,
  B00000,
};

byte c6[8] = {
  B00000,
  B00100,
  B00100,
  B11100,
  B00000,
  B00000,
  B00000,
};

byte c7[8] = {
  B00000,
  B10100,
  B01100,
  B00100,
  B00000,
  B00000,
  B00000,
};

void print_clock_setup()
{
    lcd.createChar(0, c0);
    lcd.createChar(1, c1);
    lcd.createChar(2, c2);
    lcd.createChar(3, c3);
    lcd.createChar(4, c4);
    lcd.createChar(5, c5);
    lcd.createChar(6, c6);
    lcd.createChar(7, c7);
}

void print_clock_char(byte c)
{
    lcd.setCursor(1, 2);
    lcd.write(c);
}

void print_clock()
{
    static byte iter;
    iter = (iter + 1) % 8;
    print_clock_char(iter);
}


////////////////////////////////////////////////////////////////////////////////
//                TIME SETUP

/* implements hysteresis */
bool was_button_pressed (unsigned pin)
{
    const int default_state = HIGH; /*assume that button is pull-up*/

    if (digitalRead(pin) != default_state) {
        delay(20); /*ms*/
        if (digitalRead(pin) != default_state) {
            do {} while (digitalRead(pin) == !default_state); // wait for button releases
            delay(20);
            return true;
        }
    }
    return false;
}

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
                delay(50);
                return true;
            }
    }

    return false;
}

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

    int val;
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
            switch (pos) {
                case 1: val = rtc.getMinute();    lcd.setCursor(3, line);  break;
                case 2: val = rtc.getSecond();    lcd.setCursor(6, line);  break;
                case 3: val = rtc.getDate();      lcd.setCursor(10, line); break;
                case 4: val = rtc.getMonth(tmp1); lcd.setCursor(13, line); break;
                case 5: val = rtc.getYear();      lcd.setCursor(18, line); break;
                default: break;
            }
            if (pos > 5) { //exit setup function
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
                if (val < 10)
                    lcd.print(0);
                lcd.print(val);
                lcd.setCursor(0, line);
                break;
            case 1:
                if (val < 0)  val = 59;
                if (val > 59) val = 0;
                rtc.setMinute(val);
                lcd.setCursor(3, line);
                if (val < 10)
                    lcd.print(0);
                lcd.print(val);
                lcd.setCursor(3, line);
                break;
            case 2:
                if (val < 0)  val = 59;
                if (val > 59) val = 0;
                rtc.setSecond(val);
                lcd.setCursor(6, line);
                if (val < 10)
                    lcd.print(0);
                lcd.print(val);
                lcd.setCursor(6, line);
                break;
            case 3:
                if (val < 1)  val = 31; // this allows to set 31.02 as a date
                if (val > 31) val = 1;
                rtc.setDate(val);
                lcd.setCursor(10, line);
                if (val < 10)
                    lcd.print(0);
                lcd.print(val);
                lcd.setCursor(10, line);
                break;
            case 4:
                if (val < 1)  val = 12;
                if (val > 12) val = 1;
                rtc.setDate(val);
                lcd.setCursor(13, line);
                if (val < 10)
                    lcd.print(0);
                lcd.print(val);
                lcd.setCursor(13, line);
                break;
            case 5:
                if (val < 0)  val = 99;
                if (val > 99) val = 0;
                rtc.setYear(val);
                lcd.setCursor(18, line);
                if (val < 10)
                    lcd.print(0);
                lcd.print(val);
                lcd.setCursor(18, line);
                break;
            default: break;
        }
        if (pos > 5) break; // exit loop;
    }

    lcd.noCursor();
    lcd.noBlink();

    // Full time update
    print_time_string(3 /* 4th row */, true);
}


////////////////////////////////////////////////////////////////////////////////
//                PRINT TIME

/*
 * Prints time-date string in global char time_format[]
 * Does an update/printing only when needed and return true.
 * Return false otherway.
 */
bool print_time_string(unsigned row, bool full_update)
{
    /*
     * 01234567890123456789
     * 17:34:59  31.12.2017
     */

    static byte last_second;
    if (last_second == rtc.getSecond()) {
        // No update needed
        return false;
    }
    last_second = rtc.getSecond();

    //print_clock(); // Optional

    byte ret;
    bool tmp1, tmp2;

    // second
    ret = last_second;
    time_format[6] = ret / 10 + ('0');
    time_format[7] = ret % 10 + ('0');

    if (ret != 0 && !full_update)
       goto end;

    // minute
    ret = rtc.getMinute();
    time_format[3] = ret / 10 + ('0');
    time_format[4] = ret % 10 + ('0');

    if (ret != 0 && !full_update)
       goto end;

    // hour
    ret = rtc.getHour(tmp1, tmp2);
    time_format[0] = ret / 10 + ('0');
    time_format[1] = ret % 10 + ('0');

    if (ret != 0 && !full_update)
       goto end;

    // Update all date string - very little optimization gain

    // day
    ret = rtc.getDate();
    time_format[10] = ret / 10 + ('0');
    time_format[11] = ret % 10 + ('0');

    // month
    ret = rtc.getMonth(tmp1);
    time_format[13] = ret / 10 + ('0');
    time_format[14] = ret % 10 + ('0');

    // year - only last two digits
    ret = rtc.getYear();

    time_format[18] = ret / 10 + ('0');
    time_format[19] = ret % 10 + ('0');

end:
    lcd.setCursor(0, row);
    lcd.print(time_format);

    // DHT22 sensor can refresh data every 1-2 seconds,
    // so do the update every two second (odd second).
    return last_second % 2;
}

////////////////////////////////////////////////////////////////////////////////
//                DISPLAY TEMPLATE

void draw_template()
{
    lcd.setCursor(0, 0);
    lcd.print("In:      ");
    lcd.print((char)223); // '°'
    lcd.print("C     ");
    lcd.print((char)37); // '%'
    lcd.print(" RH");

    lcd.setCursor(0, 1);
    lcd.print("Out:     ");
    lcd.print((char)223); // '°'
    lcd.print("C     ");
    lcd.print((char)37); // '%'
    lcd.print(" RH");


    lcd.setCursor(0, 2);
    lcd.print("                 hPa");
    if (show_dew_point) {
      lcd.setCursor(0, 2);
        lcd.print("DP:   ");
        lcd.print((char)223); // '°'
        lcd.print("C");
    }

    lcd.setCursor(0, 3);
    lcd.print("                    ");
}

////////////////////////////////////////////////////////////////////////////////
//                GET AND PRINT METEO DATA

void set_error (int cur, int line, bool empty_chars = false)
{
    lcd.setCursor(cur, line);
    if (empty_chars) {
        lcd.print(" Err ");
    }
    else {
        lcd.print("Err");
    }
}

void fill_data(float temp_in, float temp_out, float humid_in, float humid_out, int32_t pressure)
{
    char buff[8];

/*
 01234567890123456789
+--------------------+
|In:  11.1°C   90% RH|
|Out:-20.5°C  100% RH|
|DP:-70°C  1013.9 hPa|
|17:34:59  31.12.2017|
+--------------------+

+--------------------+
|In:  Err °C  Err% RH|
|Out: Err °C  Err% RH|
|DP:N/A°C     Err hPa|
|17:34:59  31.12.2017|
+--------------------+
 */

    // Temp inside
    if (temp_in >= err_val) {
        set_error(4, 0, true);
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
        set_error(4, 1, true);
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

    // Dew point
    if (show_dew_point) {
        lcd.setCursor(3, 2);
        int8_t ret = get_dew_point(temp_out, humid_out);
        if (ret != 100) {
            dtostrf(get_dew_point(temp_out, humid_out), 3, 0, buff);
        }
        else {
            buff[0] = 'N';
            buff[1] = (char)47; // '/'
            buff[2] = 'A';
            buff[3] = '\0';
        }
        lcd.print(buff);
    }
}

/*
 * Returns true if everything is OK.
 * Returns false if read of any value fails.
 */
bool read_meteo_data(float &temp_in,  float &humid_in,
                     float &temp_out, float &humid_out,
                     int32_t &pressure)
{
    bool ret = true;

    temp_in   = dht_in.readTemperature();
    humid_in  = dht_in.readHumidity();
    temp_out  = dht_out.readTemperature();
    humid_out = dht_out.readHumidity();
    pressure  = bmp.readPressure();


    // ##### Temperature inside #####
    // Check value
    if (isnan(temp_in)) {
        temp_in = err_val; //ERR
        ret = false;
    }


    // ##### Temperature outside #####
    // Check value
    if (isnan(temp_out)) {
        temp_out = err_val; //ERR
        ret = false;
    }


    // ##### Humidity inside #####
    // Check value
    if(isnan(humid_in)) {
        humid_in = err_val; //ERR
        ret = false;
    }


    // ##### Humidity outside #####
    // Check value
    if(isnan(humid_out)) {
        humid_out = err_val; //ERR
        ret = false;
    }


    // ##### Pressure #####
    // Nothing to do here

    return ret;
}


////////////////////////////////////////////////////////////////////////////////
//                BACKLIGHT BUTTONS

void backlight_buttons()
{
    const int default_state = HIGH; /*assume that button is pull-up*/

    static byte off_time;
    static bool should_off;

    // This may cause that we miss off_time and the backlight will stay on.
    // This may happend beause of some "lag" from sensors, or user will use
    // DOWN+UP+OK buttons to set time;
    // Anyway it's not a big issue. Backlight will be off eventually.
    // User can always turn it off manually by pushing DOWN button.
    if (should_off && off_time == rtc.getSecond()) {
        lcd.noBacklight();
        should_off = false;
        LCD_backlight = false;
    }

    // BUTTON_UP
    if (digitalRead(BUTTON_DOWN) == default_state &&
        digitalRead(BUTTON_OK) == default_state &&
        was_button_pressed(BUTTON_UP)) {
        if (LCD_backlight) {
            // Do nothing
            return;
        }
        lcd.backlight();
        LCD_backlight = true;
        off_time = (rtc.getSecond() + 10) % 60;
        should_off = true;
        return;
    }

    // BUTTON_DOWN
    if (digitalRead(BUTTON_UP) == default_state &&
        digitalRead(BUTTON_OK) == default_state &&
        was_button_pressed(BUTTON_DOWN)) {
        if (LCD_backlight) {
            lcd.noBacklight();
            LCD_backlight = false;
            should_off = false;
        }
        else {
            lcd.backlight();
            LCD_backlight = true;
            should_off = false;
        }
        return;
    }
}

////////////////////////////////////////////////////////////////////////////////
//                PRINT METEO DATA VIA RS
void print_meteo_to_rs(float temp_in, float humid_in, float temp_out, float humid_out, int32_t pressure)
{
    Serial.print("(");
    Serial.print(temp_in, 1);
    Serial.print(";");
    Serial.print(humid_in, 0);
    Serial.print(";");
    Serial.print(temp_out, 1);
    Serial.print(";");
    Serial.print(humid_out, 0);
    Serial.print(";");
    Serial.print(pressure, 1);
    Serial.print(")");
}

////////////////////////////////////////////////////////////////////////////////
//                ARDUINO SETUP

void setup ()
{
    // RS232
    Serial.begin(115200);

    // LEDs
    pinMode(DEBUG_LED, OUTPUT);
    digitalWrite(DEBUG_LED, LOW);

    // Buttons
    pinMode(BUTTON_OK, INPUT_PULLUP);
    pinMode(BUTTON_UP, INPUT_PULLUP);
    pinMode(BUTTON_DOWN, INPUT_PULLUP);

    // Configure RTC
    rtc.enableOscillator(false /* ON */,
                        false /* disabled if batery-only powered */,
                        0 /* freq: 1Hz */);
    rtc.enable32kHz(false);
    rtc.setClockMode(false); /* set to 24-hour mode */

    // LCD
    lcd.init();
    print_clock_setup(); // Can be disabled if print_clock() isn't used

    lcd.backlight();
    LCD_backlight = true;
    //lcd.noBacklight();
    lcd.noCursor();
    lcd.noBlink();
    lcd.setCursor(5, 0);
    lcd.print("Witamy!");
    lcd.setCursor(1, 1);
    lcd.print("Stacja pogodowa");
    lcd.setCursor(5, 2);
    lcd.print("wersja 0.5.0");
    lcd.setCursor(12, 3);
    lcd.print("Wi-Fi");

    // Sleep for setup dht sensors - just in case. This also gives you time to read te welcome screen.
    delay(4000);

    // Temperature and humidity sensor
    dht_in.begin();
    dht_out.begin();

    // Pressure sensor
    if (!bmp.begin(3)) { // init pressure sensor with high precission param - 3
        digitalWrite(DEBUG_LED, HIGH); // if error turn debug LED on
    }

    // Draw template - Labels and units.
    draw_template();

    // Full time update
    print_time_string(3 /* 4th row */, true);
}

////////////////////////////////////////////////////////////////////////////////
//                MAIN LOOP

void loop ()
{
    if (enter_time_setup())
        time_setup(3 /* 4th row */);

    // Support backlight buttons
    backlight_buttons();

    // Update the displayed time
    if (print_time_string(3 /* 4th row */)) {
        // Update meteo data
        digitalWrite(DEBUG_LED, HIGH);
        float temp_in, humid_in, temp_out, humid_out;
        int32_t pressure;
        bool read_status;

        read_status = read_meteo_data(temp_in, humid_in, temp_out, humid_out, pressure);
        fill_data(temp_in, temp_out, humid_in, humid_out, pressure);
        read_status = true; // fill_data puts special values in data if reading fails

        // Send meteo data via RS to ESP8266
        if ((millis() > old_time + rs_wait_time) && read_status /*Send data to ESP8266 only if read_meteo_data succed*/) {
            old_time = millis();
            print_meteo_to_rs(temp_in, humid_in, temp_out, humid_out, pressure);
        }

        digitalWrite(DEBUG_LED, LOW);
    }
}

