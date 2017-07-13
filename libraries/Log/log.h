#ifndef __LOG_H__
#define __LOG_H__

#ifdef DEBUG
#define LOG(...)   do {Serial.print(__VA_ARGS__);}   while(0)
#define LOGLN(...) do {Serial.println(__VA_ARGS__);} while(0)
#else
#define LOG(...)   do {} while(0)
#define LOGLN(...) do {} while(0)
#endif

#ifndef DEBUG_LED
#define DEBUG_LED 13
#endif


void LogInit(uint32_t speed)
{
    #ifdef DEBUG
    // Open serial port for debug
    Serial.begin(speed);
    #endif
}


// ##### BLINK #####
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


#endif /* __LOG_H__ */
