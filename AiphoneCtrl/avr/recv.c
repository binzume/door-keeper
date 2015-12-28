#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#define T0 386
#define T1 286
#define TLIMIT 500

uint8_t b;
ISR(INT0_vect) {
    uint16_t t = TCNT1;
    if (t < 200) return;
    TCNT1 = 0;
    if (t < TLIMIT) {
        b = (b << 1) & 0x07;
        if (t > (T0 + T1) / 2)  b |= 1;  // 350 =  17.5us = (15+20)/2us @20Mhz
        if (b < 3 || b == 4) { // 0,1,2,4 or 3,5,6,7
            PORTD |= 0x02;
        } else {
            PORTD &= ~0x02;
        }
    }
}


volatile uint8_t out = 0;

ISR(TIMER1_COMPA_vect) {
    TCNT1 = 24;
    // timeout
    b = 0;

    if (out) {
        PORTD ^= 0xC0;
    } else {
        PORTD |= 0x02;
        PORTB ^= 0x02;
    }
}

void sendbit(uint8_t b) {
    if (b & 1) {
        OCR1A = T1 / 2;
        PORTD |= 0x02;
    } else {
        OCR1A = T0 / 2;
        PORTD &= ~0x02;
    }
    if (TCNT1 >= OCR1A) {
        TCNT1 = OCR1A - 1;
    }

    // Wait 833ms - TIMER1_COMPA_vect.... FIXME
    for (uint8_t j = 0; j < 5; j++) {
        _delay_us(61*2);
    }
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

void send_bytes(uint8_t d[], uint8_t len) {
    PORTD |= 0x80;
    PORTD &= ~0x40;
    DDRD |= 0xC0;
    PORTC = 0x20;
    EIMSK = 0x00;

    out = 1;
    TCNT1 = 0;

    sendbit(1);
    for (uint8_t i = 0; i<len; i++) {
        sendb(d[i]);
    }

    PORTD &= ~0xC0;
    DDRD &= ~0xC0;
    out = 0;
    OCR1A = TLIMIT;
    EIMSK = 0x01;
}


int main(void) {
    DDRD = 0x02; // TxD
    PORTD = 0x04;
    DDRB = 0x03;
    PORTB = 0x00;
    PORTC = 0x20;

    // INT0
    EICRA = 0x02; // INT0 falling edge.
    EIMSK = 0x01; // INT0 enable.

    // timer1
    TIMSK1 = 1 << OCIE1A;
    OCR1A = TLIMIT;
    TCNT1 = 0;
    TCCR1B = 1 << CS10;

    sei();

    for (;;) {
        _delay_ms(20);
        if ((PINC & 0x20) == 0) {
            //  for debug.
            //char cmd[] = {0x40, 0xXX, 0x68, 0xXX};
            //char cmd[] = {0x40, 0xXX, 0x05, 0xc0, 0xXX};
            //send_bytes(cmd, 5);
            _delay_ms(200);
        }
    }
}
