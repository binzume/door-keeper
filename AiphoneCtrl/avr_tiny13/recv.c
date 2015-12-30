#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#define ROOM 0x00
#define USE_COMPARATOR 1 // using analog comparator

#define TX_DDR DDRB
#define TX_PORT PORTB
#define TX_PIN 0x10

#define OUT_PORT PORTB
#define OUT_DDR DDRB
#define OUT_PIN_A 0x01
#define OUT_PIN_B 0x02
#define OUT_PIN_MASK (OUT_PIN_A | OUT_PIN_B)

#define TCNT TCNT0
#define OCRA OCR0A

#define T0 (F_CPU/51800)
#define T1 (F_CPU/69900)
#define TLIMIT (T0 * 5 / 4)
#define TMIN (T1 * 2 / 3)
#if TLIMIT > 250 // 8bit TCNT
#  error "Timer counter OVF." TLIMIT
#endif

#if USE_COMPARATOR
#  define CAPTURE_VECT ANA_COMP_vect
#else
#  define CAPTURE_VECT INT0_vect
#endif



volatile uint8_t out = 0;
volatile uint8_t out_count = 0;
uint8_t b;
ISR(CAPTURE_VECT) {
    uint16_t t = TCNT;
    if (t < TMIN) return;
    TCNT = 0;
    b = (b << 1) & 0x07;
    if (t > (T0 + T1) / 2)  b |= 1;  // 350 =  17.5us = (15+20)/2us @20Mhz
    if (b < 3 || b == 4) { // 0,1,2,4 or 3,5,6,7
        TX_PORT |= TX_PIN;
    } else {
        TX_PORT &= ~TX_PIN;
    }
}

ISR(TIM0_COMPA_vect) {
    if (out) {
        OUT_PORT ^= OUT_PIN_MASK;
        out_count++;
    } else {
        // timeout
        b = 0;
        TX_PORT |= TX_PIN;
    }
}

void sendbit(uint8_t b) {
    out_count = 0;
    uint8_t n;
    if (b & 1) {
        OCRA = (T1 / 2);
        TX_PORT |= TX_PIN;
        n = (F_CPU / 1200 / (T1/2));
    } else {
        OCRA = (T0 / 2);
        TX_PORT &= ~TX_PIN;
        n = (F_CPU / 1200 / (T0/2));
    }
    if (TCNT >= OCRA) {
        TCNT = OCRA - 1;
    }
    while(out_count < n);
}

void sendb(uint8_t d) {
    sendbit(1);
    sendbit(0); // st

    uint8_t p = 0;
    for (uint8_t i = 0; i<8; i++) {
        if (d & 1) {
            p^=1;
        }
        sendbit(d);
        d >>= 1;
    }

    sendbit(p);
    sendbit(1);
}

void send_message(uint8_t d[], uint8_t len) {
    OUT_PORT |= OUT_PIN_B;
    OUT_PORT &= ~OUT_PIN_A;
    OUT_DDR |= OUT_PIN_MASK;

#if USE_COMPARATOR
    ACSR = 0;
#else
    EIMSK = 0x00;
#endif

    out = 1;
    TCNT = 0;
    uint8_t sum = 0;

    sendbit(1);
    for (uint8_t i = 0; i<len; i++) {
        sendb(d[i]);
        sum += d[i];
    }
    sendb(0x100-sum);

    OUT_PORT &= ~OUT_PIN_MASK;
    OUT_DDR &= ~OUT_PIN_MASK;
    out = 0;
    OCRA = TLIMIT;

#if USE_COMPARATOR
    ACSR |= (1 << ACIE) | (1 << ACIS1);
#else
    EIMSK = 0x01;
#endif
}


int main(void) {
    OSCCAL = 0x61;
    TX_DDR = TX_PIN; // TxD
    PORTB = 0x0C; // INPUT

#if USE_COMPARATOR
    // Comparator
    DIDR0 |= (1 << AIN1D) | (1 << AIN0D);
    ACSR |= (1 << ACIE) | (1 << ACIS1); // falling edge.
#else
    // INT0
    PORTD |= 0x04; // pull up
    EICRA = 0x02; // INT0 falling edge.
    EIMSK = 0x01; // INT0 enable.
#endif


    // timer1
    TIMSK0 = 1 << OCIE0A;
    OCRA = TLIMIT;
    TCNT = 0;
    TCCR0A = (1 << WGM01); // TOP=OCRxA
    TCCR0B = (1 << CS00);

    sei();

    for (;;) {
        _delay_ms(20);
        //  for debug...
        if ((PINB & 0x04) == 0) {
            //uint8_t cmd[] = {0x40, ROOM, 0x68}; // ping
            //uint8_t cmd[] = {0xC0, ROOM, 0x1c}; // off
            uint8_t cmd[] = {0x40, ROOM, 0x05, 0xc0}; // call
            send_message(cmd, 4);
            _delay_ms(200);
            _delay_ms(200);
        }
        if ((PINB & 0x08) == 0) {
            uint8_t cmd[] = {0xC0, ROOM, 0x45, 0x8F}; //  start
            send_message(cmd, 4);
            _delay_ms(180); // Send befor 1C(reject?).
            uint8_t cmd2[] = {0xC0, ROOM, 0x45, 0x8C}; //  open
            send_message(cmd2, 4);
            _delay_ms(200);
            _delay_ms(200);
        }
    }
}
