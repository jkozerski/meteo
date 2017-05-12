// Uncomment define DEBUG line to build with debug logs (on serial)
// This line MUST be before meteo_common.h include to make any effect
#define DEBUG

#include <VirtualWire.h> // 433MHz receiver. This library will disable PWM on pins 9 and 10.
#include <meteo_common.h> // Common function for meteo


/*
Used Arduino ports:

| Port |       Purpose     | Desc.                         |
+------+-------------------+-------------------------------+
|   3  |      temp out IND | PWM - temp out indicator      |
|   5  |     humid out IND | PWM - humid out indicator     |
|   6  |  low temp out LED |                               |
|   7  | high temp out LED |                               |
|   8  |   433MHz receiver |                               |
+------+-------------------+-------------------------------+
| *GND |           5 x GND | 2xLED, 2xIND, 433MHz rec.     |
+------+-------------------+-------------------------------+
| *+5V |  1 x power supply | 433MHz rec.,                  |
+------+-------------------+-------------------------------+
 (*) Ground and +5V can be taken directly from power supply
*/


// LEDs
#define LOW_TEMP_OUT_LED   6
#define HIGH_TEMP_OUT_LED  7


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
int temp_out_last_count = 0;
int humid_out_last_count = 0;
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
    vw_setup(1000);
    vw_rx_start();

    // turn off debug LED
    digitalWrite(DEBUG_LED, LOW);

    temp_out_last_val  = temp_out_1;
    humid_out_last_val = humid_min;
    temp_out_last_count = 0;
    humid_out_last_count = 0;

    LOGLN("Setup end");
}


void loop ()
{
    LOGLN("Loop");

    float temp_out  = 0;
    float humid_out = 0;

    int ret = receive_data1(&temp_out, &humid_out);
    if (ret) {
        LOGLN("Receiving datasucceed");
        temp_out_last_val  = temp_out;
        temp_out_last_count = 0;
        humid_out_last_val = humid_out;
    }
    else {
        if (temp_out_last_count > last_count_max) { // remote connection lost set vales to 0
            LOGLN("Reset remote data");
            temp_out = 0.0;
            humid_out = 0.0;
        }
        else { // restore last received value, increment counter
            LOGLN("Restore remote data");
            temp_out = temp_out_last_val;
            humid_out = humid_out_last_val;
            temp_out_last_count++;
        }
    }

/*
    int ret = receive_data(&temp_out, &humid_out);

    if (ret & 1) { // temperature
        LOGLN("Receiving temp succeed");
        temp_out_last_val  = temp_out;
        temp_out_last_count = 0;
    }
    else {
        if (temp_out_last_count > last_count_max) { // remote connection lost set vales to 0
            LOGLN("Reset remote temp data");
            temp_out = 0.0;
        }
        else {
            temp_out  = temp_out_last_val;
            temp_out_last_count++;
        }
    }

    if (ret & 2) { // humidity
        LOGLN("Receiving humid succeed");
        humid_out_last_val = humid_out;
        humid_out_last_count = 0;
    }
    else {
        if (humid_out_last_count > last_count_max) { // remote connection lost set vales to 0
            LOGLN("Reset remote humid data");
            humid_out = 0.0;
        }
        else { // restore last received value, increment counter
            humid_out = humid_out_last_val;
            humid_out_last_count++;
        }
    }
*/

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


//    LOG("delay "); LOGLN(delay_time);
    delay(delay_time);
}


/*
On success: returns 1, and sets temp_out and humid_out params to received values
On error:   returns 0, and doesn't touch params.
*/

/*
   returns 1, and sets temp
   returns 2, and sets humid
   returns 3, and sets both
   returns 0, and doesn't touch params
*/
//int receive_data (float *temp_out, float *humid_out)
//{
//    LOGLN("receive_data begin");
//    uint8_t buff[VW_MAX_MESSAGE_LEN];
//    uint8_t bufflen = VW_MAX_MESSAGE_LEN;
//    uint8_t len = 0;
//    int _val = 0;     // temporary variable
//    int i = 0;        // iterator
//    float temp = 0;   // temperature
//    float humid = 0;  // humidity
//    int ret = 0;
//
//
//    String message;
//
//    while(vw_have_message()) { // check if there is any message to received
//
//        bufflen = VW_MAX_MESSAGE_LEN;
//        message = "";
//        if (vw_get_message(buff, &bufflen)) { // if message received
//            LOG("Message received ");
//            for (len = 0; len < bufflen; len++) {
//                message += char(buff[len]);
//	    }
//        }
//        else {
//            LOGLN("Message seems to be broken");
//            goto out;
//        }
//
//    /*
//    Message format is:
//    T[temp_val*10 + 300]
//    and
//    H[humid_val]
//    ie.:
//    T900
//    H700
//    T12
//    H100
//    T0
//    H21
//    T153
//    H15
//    T500
//    H80
//    */
//
//        i = 0;
//        // temperature
//        if (message[i] == 'T') { // found "T"
//            i++;
//            if (i >= len) break;
//
//            while (message[i] >= '0' && message[i] <= '9') {
//                _val *= 10;
//                _val += message[i] - '0';
//                i++;
//                if (i >= len) break;
//            }
//
//            *temp_out = (_val - 300.0) / 10.0;
//            temp = 1;
//            LOG("parsed temp_out: ");  LOGLN(*temp_out);
//        }
//        if (i >= len) break;
//
//        // humidity
//        if (message[i] == 'H') {
//            i++;
//            if (i >= len) goto out;
//
//            _val = 0;
//            while (message[i] >= '0' && message[i] <= '9') {
//                _val *= 10;
//                _val += message[i] - '0';
//                i++;
//                if (i >= len) break;
//            }
//
//            *humid_out = _val;
//            humid = 2;
//            LOG("parsed humid_out: "); LOGLN(*humid_out);
//        }
//    }
//
//out:
//
//    LOG("receive_data end: "); LOGLN(temp + humid);
//    return temp + humid;
//}


/*
   returns 1, and sets temp
   returns 2, and sets humid
   returns 3, and sets both (temp and humid)
   returns 0, and doesn't touch params
*/
int receive_data (float *temp_out, float *humid_out)
{
    LOGLN("receive_data begin");
    uint8_t buff[VW_MAX_MESSAGE_LEN];
    uint8_t bufflen = VW_MAX_MESSAGE_LEN;
    uint8_t len = 0;
    int _val = 0; // temporary variable
    int ret = 0;

    
    String message;

    while(vw_have_message()) { // check if there is any message to received

        bufflen = VW_MAX_MESSAGE_LEN;
        message = "";
        if (vw_get_message(buff, &bufflen)) { // if message received
            for (len = 0; len < bufflen; len++) {
                message += char(buff[len]);
	    }
        }
        else {
            LOGLN("Message seems to be broken");
            continue;
        }

        if (len != 2) {
            LOG("Wrong message length : "); LOGLN(len);
            continue;
        }

        LOG("Message received : "); LOG(int(message[0])); LOGLN(int(message[1]));

        if (message[1] & 0x80) { // humid
            //message[1] &= 0x7F;
            _val = message[0];

            *humid_out = _val;
            ret |= 2; // humidity
            LOG("parsed humid_out: "); LOGLN(*humid_out);
        }
        else { // temp
            _val = message[1];
            _val = _val << 8;
            _val |= message[0];

            *temp_out = (_val + temp_out_min) / 10.0;
            ret |= 1; // temperature
            LOG("parsed temp_out: ");  LOGLN(*temp_out);
        }
    }

    return ret;
}



int receive_data1 (float *temp_out, float *humid_out)
{
    LOGLN("receive_data begin");
    uint8_t buff[VW_MAX_MESSAGE_LEN];
    uint8_t bufflen = VW_MAX_MESSAGE_LEN;
    uint8_t len = 0;
    int ret = 0;


    String message;

    while(vw_have_message()) { // check if there is any message to received

        bufflen = VW_MAX_MESSAGE_LEN;
        message = "";
        if (vw_get_message(buff, &bufflen)) { // if message received
            for (len = 0; len < bufflen; len++) {
                message += char(buff[len]);
	    }
        }
        else {
            LOGLN("Message seems to be broken");
            continue;
        }

        if (len < 2) {
            LOG("Wrong message length : "); LOGLN(len);
            continue;
        }

        LOG("Message received : "); LOG(uint8_t(message[0]));
        LOG(", ");                LOGLN(uint8_t(message[1]));

        *temp_out = (uint8_t(message[0]) + temp_out_min) / 2.5;
        *humid_out = (uint8_t(message[0]));
        LOG("parsed humid_out: "); LOGLN(*humid_out);
        LOG("parsed temp_out: ");  LOGLN(*temp_out);

        ret = 1;
    }

    return ret;
}

