// Uncomment define DEBUG line to build with debug logs (on serial)
// This line MUST be before meteo_common.h include to make any effect
//#define DEBUG

#include <VirtualWire.h> // 433MHz receiver. This library will disable PWM on pins 9 and 10.
#include <meteo_common.h> // Common function for meteo


/*
Used Arduino ports:

| Port |       Purpose     | Desc.                         |
+------+-------------------+-------------------------------+
|   3  |      temp out IND | PWM - temp out indicator      |
|   4  |  low temp out LED |                               |
|   5  |     humid out IND | PWM - humid out indicator     |
|   6  | high temp out LED |                               |
|   8  |   433MHz receiver |                               |
+------+-------------------+-------------------------------+
| *GND |           5 x GND | 2xLED, 2xIND, 433MHz rec.     |
+------+-------------------+-------------------------------+
| *+5V |  1 x power supply | 433MHz rec.,                  |
+------+-------------------+-------------------------------+
 (*) Ground and +5V can be taken directly from power supply
*/


// LEDs
#define LOW_TEMP_OUT_LED   4
#define HIGH_TEMP_OUT_LED  6


// INDICATORS (need PWM)
#define IND_TEMP_OUT   3
#define IND_HUMID_OUT  5


// 433MHz Receiver
#define RECEIVER_PIN 8 // was 12
int receive_data (float *temp_out, float *humid_out);


// These values will keep last values from remote sensor
// In every loop if remote connetion is lost (no or corrupted data is received)
// out_last_count will be incremented, and the last correctly received value
// will be used. If out_last_count will be bigger than last_count_max, the we
// assume that connection with remote is lost for good, and set *_last_val to 0 -
// - which should means 'sorry no data from remote sensor'.
float temp_out_last_val = 0;
float humid_out_last_val = 0;
int out_last_count = 0;
const int last_count_max = 10;


// Calibration of indicators - values on which the pointer reach maximum on scale.
// Notice: zero value should be (manually) calibrated to point 0 on scale.
const int calib_max_temp  = 255;
const int calib_max_humid = 255;

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

    // receiver setup
    vw_set_rx_pin(RECEIVER_PIN);
    vw_setup(2000);	 
    vw_rx_start();

    // turn off debug LED
    digitalWrite(DEBUG_LED, LOW);

    temp_out_last_val  = temp_out_1;
    humid_out_last_val = humid_min;
    out_last_count = 0;

    LOGLN("Setup end");
}


void loop ()
{
    LOGLN("Loop");

    float temp_out  = 0;
    float humid_out = 0;
    if (!receive_data(&temp_out, &humid_out)) {
        // Error while receiving data
        LOGLN("Error while receiving data");
        error_blink(3);

        if (out_last_count > last_count_max) { // remote connection lost set values to 0
            LOGLN("Connection lost, reset remote data");
            temp_out = 0.0;
            humid_out = 0.0;
        }
        else { // restore last received values, increment counter
            LOGLN("Using last received data");
            temp_out  = temp_out_last_val;
            humid_out = humid_out_last_val;
            out_last_count++;
        }
    }
    else { // Receiving data succeed
        LOGLN("Receiving data succeed");
        temp_out_last_val  = temp_out;
        humid_out_last_val = humid_out;
        out_last_count = 0;
    }

 
    long val;
    // Check if temp and pressure values are in ranges, and fix them if needed
    // Humidity is always in range
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


    // humidity out
    LOG("humid_out: "); LOGLN(humid_out);
    val = ind_map10_cal(humid_out, humid_min, humid_max, calib_max_humid);
    analogWrite(IND_HUMID_OUT, val);


    LOG("delay "); LOGLN(delay_time);
    delay(delay_time);
}


/*
On success: returns 1, and sets temp_out and humid_out params to received values
On error:   returns 0, and doesn't touch params.
*/
int receive_data (float *temp_out, float *humid_out)
{
    LOGLN("receive_data begin");
    uint8_t buff[VW_MAX_MESSAGE_LEN];
    uint8_t bufflen = VW_MAX_MESSAGE_LEN;
    uint8_t len = 0;
    int _val = 0;     // temporary variable
    int i = 0;        // iterator
    float temp = 0;   // temperature
    int negative = 0; // set if temperature is negative
    float humid = 0;  // humidity
    int checksum = 0;      // simple checksum - should take only one byte
    const int prime = 251; // for calculating checksum
    
    String message;

    if(!vw_have_message()) { // check if there is any message to received
        // no message to received
        LOGLN("No message to received");
        goto err;
    }
 
    if (vw_get_message(buff, &bufflen)) // if message received
    {
        for (len = 0; len < bufflen; len++) {
            message += char(buff[len]);
	}
    }
    else {
        LOGLN("Message seems to be broken");
        goto err;
    }

    LOGLN(message);

    /*
    Message format is:
    T[temp_val*10]H[humid_val*10];[checksum];
    ie.:
    No checksum  |  With checksum
    -------------+----------------
    T-200H700;   |  T-200H700;'\170'  ->  T-200H700;¬
    T-12H1000;   |  T-12H1000;'\137'  ->  T-12H1000;ë
    T0H210;      |  T0H210;'\70'      ->  T0H210;F
    T153H550;    |  T153H550;'\6'     ->  (no pritable checksum)
    T500H800;    |  T500H800;'\127'   ->  (no pritable checksum)

    cc = (temp_val * 10 + humid_val) % 251
    */

    // temperature
    while (message[i] != 'T') { // find "T"
        i++;
        if (i >= len) goto err;
    }

    i++;
    if (i >= len) goto err;

    if (message[i] == '-') { // check if negative
        negative = 1;
        i++;
        if (i >= len) goto err;
    }

    while (message[i] >= '0' && message[i] <= '9') {
        _val *= 10;
        _val += message[i] - '0';
        i++;
        if (i >= len) goto err;
    }
    if (negative)
        _val *= -1;

    temp = _val/10.0;
    checksum = (_val + 300) % prime;

    // humidity
    if (message[i] == 'H')
        i++;
    else
        goto err;

    if (i >= len) goto err;

    _val = 0;
    while (message[i] >= '0' && message[i] <= '9') {
        _val *= 10;
        _val += message[i] - '0';
        i++;
        if (i >= len) goto err;
    }
    if (message[i] == ';') {
        humid = _val / 10.0;
        i++;
        if (i >= len) goto err;
   }
   else
       goto err;

    checksum = (checksum + _val / 10) % prime;

   // check if checksum is correct
/*
   if (message[i] != checksum) {
        goto err;
   }
*/

    *temp_out = temp;
    *humid_out = humid;

    LOG("parsed temp_out: ");  LOGLN(*temp_out);
    LOG("parsed humid_out: "); LOGLN(*humid_out);

    LOGLN("receive_data end (OK)");
    return 1;

err:
    // do not touch function's params
    LOGLN("receive_data end (ERR)");
    return 0;
}

