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
|In: -20.5°C   100%RH|
|Out:-20.5°C   100%RH|
|DP:-70°C  1013.9 hPa|
|DP: 70°C     Err hPa|
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


// Real-time clock
DS3231 rtc;


// Time format
// 01234567890123456789
// 00:00:00  31.12.2017
char time_format[21] = "17:34:59  31.12.2017";
void print_time_string(unsigned row, bool full_update = false);


// Error value
const int32_t err_val = 200000;

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
int8_t dew_point[41][21] = {
           /* 0.5%  5%   10%  15%  20%  25%  30%  35%  40%  45%  50%  55%  60%  65%  70%  75%  80%  85%  90%  95% 100%*/
/*  50°C */  {-26,   1,  11,  17,  21,  25,  28,  31,  33,  35,  37,  39,  40,  42,  43,  44,  46,  47,  48,  49,  50},
/*  48°C */  {-27,   0,  10,  16,  21,  24,  27,  30,  32,  34,  36,  38,  39,  41,  42,  43,  45,  46,  47,  48,  48},
/*  46°C */  {-28,   0,   9,  15,  20,  23,  26,  29,  31,  33,  35,  37,  38,  40,  41,  43,  44,  45,  46,  46,  46},
/*  44°C */  {-29,  -3,   6,  12,  16,  20,  23,  25,  28,  30,  31,  33,  35,  36,  37,  39,  40,  41,  42,  43,  44},
/*  42°C */  {-31,  -5,   5,  10,  15,  18,  21,  24,  26,  28,  30,  31,  33,  34,  35,  37,  38,  39,  40,  41,  42},
/*  40°C */  {-32,  -6,   3,   9,  13,  16,  19,  22,  24,  26,  28,  29,  31,  32,  34,  35,  36,  37,  38,  39,  40},
/*  38°C */  {-33,  -8,   1,   7,  11,  15,  18,  20,  22,  24,  26,  27,  29,  30,  32,  33,  34,  35,  36,  37,  38},
/*  36°C */  {-34,  -9,   0,   6,  10,  13,  16,  18,  20,  22,  24,  26,  27,  28,  30,  31,  32,  33,  34,  35,  36},
/*  34°C */  {-35, -11,  -2,   4,   8,  11,  14,  16,  19,  20,  22,  24,  25,  27,  28,  29,  30,  31,  32,  33,  34},
/*  32°C */  {-36, -12,  -3,   2,   6,  10,  12,  15,  17,  19,  20,  22,  23,  25,  26,  27,  28,  29,  30,  31,  32},
/*  30°C */  {-37, -13,  -5,   1,   5,   8,  11,  13,  15,  17,  18,  20,  21,  23,  24,  25,  26,  27,  28,  29,  30},
/*  28°C */  {-38, -15,  -6,  -1,   3,   6,   9,  11,  13,  15,  17,  18,  20,  21,  22,  23,  24,  25,  26,  27,  28},
/*  26°C */  {-40, -16,  -8,  -3,   1,   4,   7,   9,  11,  13,  15,  16,  18,  19,  20,  21,  22,  23,  24,  25,  26},
/*  24°C */  {-41, -18,  -9,  -4,   0,   3,   5,   8,  10,  11,  13,  14,  16,  17,  18,  19,  20,  21,  22,  23,  24},
/*  22°C */  {-42, -19, -11,  -6,  -2,   1,   4,   6,   8,   9,  11,  13,  14,  15,  16,  17,  18,  19,  20,  21,  22},
/*  20°C */  {-43, -21, -13,  -7,  -4,  -1,   2,   4,   6,   8,   9,  11,  12,  13,  14,  15,  16,  17,  18,  19,  20},
/*  18°C */  {-44, -22, -14,  -9,  -5,  -2,   0,   2,   4,   6,   7,   9,  10,  11,  12,  13,  14,  15,  16,  17,  18},
/*  16°C */  {-45, -23, -16, -11,  -7,  -4,  -2,   0,   2,   4,   6,   7,   8,   9,  10,  12,  13,  13,  14,  15,  16},
/*  14°C */  {-46, -25, -17, -12,  -9,  -6,  -3,  -1,   1,   2,   4,   5,   6,   7,   9,  10,  11,  11,  12,  13,  14},
/*  12°C */  {-47, -26, -19, -14, -10,  -8,  -5,  -3,  -1,   0,   2,   3,   4,   6,   7,   8,   9,  10,  10,  11,  12},
/*  10°C */  {-49, -28, -20, -16, -12,  -9,  -7,  -5,  -3,  -1,   0,   1,   3,   4,   5,   6,   7,   8,   8,   9,  10},
/*   8°C */  {-50, -29, -22, -17, -14, -11,  -9,  -7,  -5,  -3,  -2,  -1,   1,   2,   3,   4,   5,   6,   6,   7,   8},
/*   6°C */  {-51, -31, -23, -19, -15, -13, -10,  -8,  -7,  -5,  -4,  -2,  -1,   0,   1,   2,   3,   4,   4,   5,   6},
/*   4°C */  {-52, -32, -25, -20, -17, -14, -12, -10,  -9,  -7,  -6,  -4,  -3,  -2,  -1,   0,   1,   2,   2,   3,   4},
/*   2°C */  {-53, -34, -26, -22, -19, -16, -14, -12, -10,  -9,  -7,  -6,  -5,  -4,  -3,  -2,  -1,   0,   1,   1,   2},
/*   0°C */  {-54, -35, -28, -24, -20, -18, -16, -14, -12, -11,  -9,  -8,  -7,  -6,  -5,  -4,  -3,  -2,  -1,  -1,   0},
/*  -2°C */  {-55, -36, -30, -25, -22, -20, -17, -16, -14, -12, -11, -10,  -9,  -8,  -7,  -6,  -5,  -4,  -3,  -3,  -2},
/*  -4°C */  {-57, -38, -31, -27, -24, -21, -19, -17, -16, -14, -13, -12, -11, -10,  -9,  -8,  -7,  -6,  -5,  -5,  -4},
/*  -6°C */  {-58, -39, -33, -29, -25, -23, -21, -19, -18, -16, -15, -14, -13, -12, -11, -10,  -9,  -8,  -7,  -7,  -6},
/*  -8°C */  {-59, -41, -34, -30, -27, -25, -23, -21, -19, -18, -17, -16, -14, -13, -13, -12, -11, -10,  -9,  -9,  -8},
/* -10°C */  {-60, -42, -36, -32, -29, -26, -24, -23, -21, -20, -19, -17, -16, -15, -14, -14, -13, -12, -11, -11, -10},
/* -12°C */  {-61, -44, -37, -33, -30, -28, -26, -24, -23, -22, -20, -19, -18, -17, -16, -16, -15, -14, -13, -13, -12},
/* -14°C */  {-62, -45, -39, -35, -32, -30, -28, -26, -25, -23, -22, -21, -20, -19, -18, -18, -17, -16, -15, -15, -14},
/* -16°C */  {-63, -46, -40, -37, -34, -32, -30, -28, -27, -25, -24, -23, -22, -21, -20, -19, -19, -18, -17, -17, -16},
/* -18°C */  {-64, -48, -42, -38, -35, -33, -31, -30, -28, -27, -26, -25, -24, -23, -22, -21, -21, -20, -19, -19, -18},
/* -20°C */  {-66, -49, -44, -40, -37, -35, -33, -32, -30, -29, -28, -27, -26, -25, -24, -23, -23, -22, -21, -21, -20},
/* -22°C */  {-67, -51, -45, -41, -39, -37, -35, -33, -32, -31, -30, -29, -28, -27, -26, -25, -25, -24, -23, -23, -22},
/* -24°C */  {-68, -52, -47, -43, -40, -38, -37, -35, -34, -33, -32, -31, -30, -29, -28, -27, -26, -26, -25, -25, -24},
/* -26°C */  {-69, -54, -48, -45, -42, -40, -38, -37, -36, -34, -33, -32, -31, -31, -30, -29, -28, -28, -27, -27, -26},
/* -28°C */  {-70, -55, -50, -46, -44, -42, -40, -39, -37, -36, -35, -34, -33, -33, -32, -31, -30, -30, -29, -29, -28},
/* -30°C */  {-71, -57, -51, -48, -45, -44, -42, -40, -39, -38, -37, -36, -35, -34, -34, -33, -32, -32, -31, -31, -30}};


/*
 * Return a dew point for temperatures in range -30°C up to +50°C.
 * Humidity should be from 0 up to 100%.
 * Functions returns esimated dew point in °C
 * Returns 100 as an error - Temperature beyond the scale.
 */
int8_t get_dew_point (float temp, float humid)
{
    if (temp < -31.0 || temp > 50)
        return 100; // treat it as an error.

    int temp_idx = 40-(temp + 31.0)/2;
    if (temp_idx < 0) temp_idx = 0;
    if (temp_idx > 40) temp_idx = 40;

    int humid_idx = (humid+2.5)/5;
    if (humid_idx < 0) humid_idx = 0;
    if (humid_idx > 20) humid_idx = 20;

    return dew_point[temp_idx][humid_idx];
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
    static byte last_second;
    if (last_second == rtc.getSecond()) {
        // No update needed
        return;
    }
    last_second = rtc.getSecond();

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
 * Does an update/printing only when needed
 */
void print_time_string(unsigned row, bool full_update)
{
    /*
     * 01234567890123456789
     * 17:34:59  31.12.2017
     */

    static byte last_second;
    if (last_second == rtc.getSecond()) {
        // No update needed
        LOGLN("RTC No update needed");
        return;
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
    LOGLN(time_format);
    lcd.setCursor(0, row);
    lcd.print(time_format);
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
|In:  Err °C  Err% RH|
|Out:-20.5°C  100% RH|
|DP:-70°C  1013.9 hPa|
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
        humid_out = err_val; //ERR
    }
    else {
        LOG("Humid out: "); LOGLN(humid_out);
    }


    // ##### Pressure #####
    // Nothing to do here
    LOG("Pressure: "); LOGLN(pressure);
}


////////////////////////////////////////////////////////////////////////////////
//                BACKLIGHT BUTTONS

void backlight_buttons()
{
    static byte off_time;
    static bool should_off;

    // This may cause that we miss off_time and the backlight will stay on.
    // This may happend beause of some "lag" from sensors, or user witl use
    // DOWN+UP+OK buttons to set time;
    // Anyway it's not a big issue. Backlight will be off eventually.
    // User can always turn it off manually by pushing DOWN button.
    if (should_off && off_time == rtc.getSecond()) {
        lcd.noBacklight();
        should_off = false;
    }

    if (was_button_pressed(BUTTON_UP)) {
        lcd.backlight();
	return;
    }
    if (was_button_pressed(BUTTON_DOWN)) {
        lcd.noBacklight();
	return;
    }
    else if (was_button_pressed(BUTTON_OK)) {
        lcd.backlight();
        off_time = (rtc.getSecond() + 4) % 60;
        should_off = true;
    }
}


////////////////////////////////////////////////////////////////////////////////
//                ARDUINO SETUP

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
    rtc.enableOscillator(false /* ON */,
                        false /* disabled if batery-only powered */,
                        0 /* freq: 1Hz */);
    rtc.enable32kHz(false);
    rtc.setClockMode(false); /* set to 24-hour mode */

    // LCD
    lcd.init();
    print_clock_setup(); // Can be disabled if print_clock() isn't used

    lcd.backlight();
    //lcd.noBacklight();
    //lcd.noCursor();
    //lcd.noBlink()
    lcd.setCursor(5, 0);
    lcd.print("Witamy!");
    lcd.setCursor(1, 1);
    lcd.print("Stacja pogodowa");
    lcd.setCursor(7, 2);
    lcd.print("wersja D_0.2");

    // Sleep for setup dht sensors - just in case. This also gives you time to read te welcome screen.
    delay(4000);

    // Temperature and humidity sensor
    dht_in.begin();
    dht_out.begin();

    // Pressure sensor
    if (!bmp.begin(3)) { // init pressure sensor with high precission param - 3
        LOGLN("Cannot initalize BMP (pressure) sensor");
        digitalWrite(DEBUG_LED, HIGH); // if error turn debug LED on
    }

    // Draw template - Labels and units.
    draw_template();

    // Full time update
    print_time_string(3 /* 4th row */, true);
    LOGLN("Setup end");
}

////////////////////////////////////////////////////////////////////////////////
//                MAIN LOOP

void loop ()
{
    static unsigned long lastTime;
    unsigned long timeNow = millis();
    LOGLN("Loop");

    if (enter_time_setup())
        time_setup(3 /* 4th row */);

    // Support backlight buttons
    backlight_buttons();

    // Update meteo data
    if (timeNow - lastTime > 1000) {
        lastTime = timeNow;
        digitalWrite(DEBUG_LED, HIGH);
        float temp_in, humid_in, temp_out, humid_out;
        int32_t pressure;

        read_meteo_data(temp_in, humid_in, temp_out, humid_out, pressure);
        fill_data(temp_in, temp_out, humid_in, humid_out, pressure);
        digitalWrite(DEBUG_LED, LOW);
    }

    // Update the displayed time
    print_time_string(3 /* 4th row */);
}

