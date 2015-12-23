#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

uint8_t b;
ISR(INT0_vect) {
    uint16_t t = TCNT1;
    if (t < 200) return;
    TCNT1 = 0;
    if (t < 500) {
        b = (b << 1) % 0x07;
        if (t > 350)  b |= 1;  // 350 =  17.5us = (15+20)/2us @20Mhz
        if (b < 3 || b == 4) { // 0,1,2,4 or 3,5,6,7
            PORTD |= 0x02;
        } else {
            PORTD &= ~0x02;
        }
    }
}

ISR(TIMER1_COMPA_vect) {
    // timeout
    b = 0;
    PORTD |= 0x02;
    PORTB ^= 0x02;
}

int main(void) {
    DDRD = 0x02; // TxD
    PORTD = 0x04;
    DDRB = 0x03;
    PORTB = 0x00;

    // INT0
    EICRA = 0x02; // INT0 falling edge.
    EIMSK = 0x01; // INT0 enable.

    // timer1
    TIMSK1 = 1 << OCIE1A;
    OCR1A = 800;
    TCNT1 = 0;
    TCCR1B = 1 << CS10;

    sei();

    for (;;) {
        _delay_ms(250);
        PORTB ^= 0x02;
    }
}
