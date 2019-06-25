//
// Serial to Ethernet bridge
//
// Protocol:
// <start><size of data 16bit binary><data>
//
#include <w5500.h>

#define BUFF_SIZE (1500)
#define LED 13

Wiznet5500 w5500(10);
const byte source_mac[] = { 0x98, 0x76, 0xB6, 0x10, 0x9e, 0xbe };
uint8_t send_buffer[BUFF_SIZE];
uint8_t recv_buffer[BUFF_SIZE];

byte start_pattern[] = {'S', 'E', 'N', 'D'};

void setup() {
    Serial.begin(115200);
    while(!Serial);

    pinMode(LED, OUTPUT);
    digitalWrite(LED, LOW);

    w5500.begin(source_mac);
}

void sendStateMachine() {
    static enum {INIT, WAIT_FOR_START, READ_SIZE, SEND_DATA} senderState = INIT;
    char c;
    static int statei = 0;
    static int size = 0;

    if (senderState == INIT) {
        statei = 0;
        size = 0;
        senderState = WAIT_FOR_START;
    } else if (senderState == WAIT_FOR_START) {
        if (Serial.available()) {
            c = Serial.read();
            if (c == start_pattern[statei]) {
                statei ++;
            } else {
                statei = 0;
            }

            if (statei == sizeof(start_pattern)) {
                senderState = READ_SIZE;
                statei = 0;
                digitalWrite(LED, HIGH);
            }
        }
    } else if (senderState == READ_SIZE) {
        if (Serial.available()) {
            c = Serial.read();
            statei ++;
            size = (size << 8) + c;

            if (statei == 2) {
                senderState = SEND_DATA;
                statei = 0;
            }
        }
    } else if (senderState == SEND_DATA) {
        if (statei < size) {
            if (Serial.available()) {
                c = Serial.read();
                send_buffer[statei++] = c;
            }
        } else {
            w5500.sendFrame(send_buffer, size);
            digitalWrite(LED, LOW);
            senderState = INIT;
        }
    }
}

void recvStateMachine() {
    uint16_t len = w5500.readFrame(recv_buffer, sizeof(recv_buffer));
    if (len > 0) {
        Serial.print("RECV"); Serial.write((uint8_t*)(&len), 2);
        Serial.write(recv_buffer, len);
    }
}

void loop() {
    sendStateMachine();
    recvStateMachine();
}