#ifndef __METEO_COMMON_H__
#define __METEO_COMMON_H__

#ifdef DEBUG
#define LOG(...)   do {Serial.print(__VA_ARGS__);}   while(0)
#define LOGLN(...) do {Serial.println(__VA_ARGS__);} while(0)
#else
#define LOG(...)   do {} while(0)
#define LOGLN(...) do {} while(0)
#endif

#define DEBUG_LED 13

/*
Indicator ranges:
Temperature [*C] in:
    10 : 40
Temperature [*C] out:
    -30 : 0 / 0 : 30 / 30 : 60
Pressure [hPa]:
    960 : 990 / 990 : 1020 / 1020 : 1050
Humidity (relative) [%] in/out:
    0 : 100
*/
const long temp_in_min  = 10; // [*C]
const long temp_in_max  = 40; // [*C]

const long temp_out_min = -30; // [*C]
const long temp_out_1   =   0; // [*C]
const long temp_out_2   =  30; // [*C]
const long temp_out_max =  60; // [*C]

const long humid_min    =   0; // [%]
const long humid_max    = 100; // [%]

const long pressure_min =  96000; // [Pa]
const long pressure_1   =  99000; // [Pa]
const long pressure_2   = 102000; // [Pa]
const long pressure_max = 105000; // [Pa]

/* Map values to indicators */
long ind_map (long val, long val_min, long val_max)
{
    return map(val, val_min, val_max, 0, 255);
};

/* Map values to indicators - multiply value *10.0 for better accuracy */
long ind_map10(float val, float val_min, float val_max)
{
    return map(val * 10.0, val_min * 10.0, val_max * 10.0, 0, 255);
};

/* Map values to indicators - with calibration */
long ind_map_cal(long val, long val_min, long val_max, int cal_max)
{
    if (cal_max > 255)
        cal_max = 255;
    return map(val, val_min, val_max, 0, cal_max);
};

/* Map values to indicators - multiply value *10.0 for better accuracy - with calibration */
long ind_map10_cal(float val, float val_min, float val_max, int cal_max)
{
    if (cal_max > 255)
        cal_max = 255;
    return map(val * 10.0, val_min * 10.0, val_max * 10.0, 0, cal_max);
};


/* sleep delay */
const int delay_time = 5000; // [ms] Read values every 5s.

/* Blinks c times with debug LED.
   Used to notify about errors */
void error_blink (int c = 3)
{
    int i = 0;
    const int dt = 100; // delay time [ms]

    for(;i < c; i++) {
    digitalWrite(DEBUG_LED, HIGH);
    delay(dt);
    digitalWrite(DEBUG_LED, LOW);
    delay(dt);
    }
}

/*
Function just wave PWM duty cycle from 0 -> 100% on provided pins, to check
if indicators are working properly. This should take ~4s.
Then is sets indicator on possitions: 2/3, 1/2, 1/3 and stops on each posstion
for a while - at the time you can check if indicator's scales are properly calibrated.
At the end it sets indicator to possitions 0 and returns.
This function should take about 12 seconds.

ports[] : Table of PWM ports to run diagnostic setup.
          The last element in the table should be -1, e.g.:
          int posts[] = {3, 5, 6, -1}
          int posts[] = {10, -1}
*/
void diagnostic_setup(int ports[], int calib[])
{
    int i;
    const int dt  = 4000/255; // delay time = 4[s] / 255[steps]
    const int dt2 = 2000;     // delay time = 2[s]

    int j;
    for (i = 0; i < 256; i++) {
        j = 0;
        while (ports[j] > -1) {
            if (calib[j] >= i)
                analogWrite(ports[j], i);
            j++;
        }
        delay(dt);
    }
    delay(dt2);

    j = 0;
    while (ports[j] > -1) {
        analogWrite(ports[j], calib[j] * 2/3);
        j++;
    }
    delay(dt2);

    j = 0;
    while (ports[j] > -1) {
        analogWrite(ports[j], calib[j] * 1/2);
        j++;
    }
    delay(dt2);

    j = 0;
    while (ports[j] > -1) {
        analogWrite(ports[j], calib[j] * 1/3);
        j++;
    }
    delay(dt2);

    j = 0;
    while (ports[j] > -1) {
        analogWrite(ports[j], 0);
        j++;
    }
    delay(dt2);
}

#endif /* __METEO_COMMON_H__ */
