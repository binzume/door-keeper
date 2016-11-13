#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

//#define ROOM 0x00
#ifndef ROOM
#  error "ROOM undeclared. (gcc option: -D ROOM=0xXX)"
#endif
#define USE_COMPARATOR 1 // using analog comparator

// uart
#define TX_DDR DDRB
#define TX_PORT PORTB
#define TX_PIN 0x10
#define RX_PORT PORTB
#define RX_PIN PINB
#define RX_PIN_MSK 0x08
#define RX_INT PCINT3
#define RX_INT_REG PCMSK
#define RX_INT_VECT PCINT0_vect

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
typedef uint16_t tcnt_t; // 8bit TCNT
#if TLIMIT > 250 // 8bit TCNT
#  error "Timer counter OVF." TLIMIT
#endif

#if USE_COMPARATOR
#  define CAPTURE_VECT ANA_COMP_vect
#else
#  define CAPTURE_VECT INT0_vect
#endif

#define MODE_IDLE 0
#define MODE_SEND 1
#define MODE_RECV 2
#define MODE_RX 3
#define MODE_TX 4

volatile uint8_t mode = MODE_IDLE;
volatile tcnt_t last_tcnt = 0;
volatile uint8_t compa_count = 0;
volatile uint8_t bit_count = 0;
volatile uint8_t buf;
volatile uint8_t uart_recv_data = 0;
ISR(CAPTURE_VECT) {
    tcnt_t t = TCNT;
    if (mode == MODE_RX || mode == MODE_TX) return;
    if (t < TMIN) return;
    TCNT = 0;
    OCRA = TLIMIT;

    buf = (buf << 1) & 0x07;
    if (t < (T0 + T1) / 2)  buf |= 1;
    if (mode == MODE_RECV) {
        if (buf < 3 || buf == 4) { // 0,1,2,4 or 3,5,6,7
            TX_PORT &= ~TX_PIN;
        } else {
            TX_PORT |= TX_PIN;
        }
    } else {
        if (buf == 7) {
            // signal detected.
            mode = MODE_RECV;
        }
    }
    last_tcnt = t;
}

ISR(RX_INT_VECT) {
    if (RX_PIN & RX_PIN_MSK) return;
    if (mode != MODE_IDLE) return;
    bit_count = 0;
    compa_count = 0 - (F_CPU / 1200 / 200) / 2;
    RX_INT_REG &= ~(1 << RX_INT);
    TCNT = 0;
    OCRA = 200;
    mode = MODE_RX;
}

ISR(TIM0_COMPA_vect) {
    compa_count++;
    if (mode == MODE_SEND) {
        OUT_PORT ^= OUT_PIN_MASK;
    } else if (mode == MODE_RX) {
        if (compa_count == (F_CPU / 1200 / 200)) {
            compa_count = 0;
            if (bit_count == 8) {
                uart_recv_data = buf;
                RX_INT_REG |= (1 << RX_INT);
                bit_count = 0;
                mode = MODE_IDLE;
            } else if (bit_count < 8) {
                buf >>= 1;
                bit_count++;
                if (RX_PIN & RX_PIN_MSK) {
                    buf |= 0x80;
                }
            }
        }
    } else if (mode == MODE_TX) {
        if (compa_count == (F_CPU / 1200 / 200)) {
            compa_count = 0;
            if (bit_count == 8) {
                TX_PORT |= TX_PIN;
                bit_count = 0;
                mode = MODE_IDLE;
            } else if (bit_count < 8) {
                if (buf & 1) {
                    TX_PORT |= TX_PIN;
                } else {
                    TX_PORT &= ~TX_PIN;
                }
                buf >>= 1;
                bit_count++;
            }
        }
    } else {
        // timeout
        mode = MODE_IDLE;
        buf = 0;
        TX_PORT |= TX_PIN;
    }
}

void sendbit(uint8_t b) {
    compa_count = 0;
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
    while(compa_count < n);
}

void sendbyte(uint8_t d) {
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

void send_message(const uint8_t d[], uint8_t len) {
#if USE_COMPARATOR
    ACSR = 0;
#else
    EIMSK = 0x00;
#endif
    GIMSK &= ~(1 << PCIE);

    while(mode != MODE_IDLE);
    OUT_PORT |= OUT_PIN_B;
    OUT_PORT &= ~OUT_PIN_A;
    OUT_DDR |= OUT_PIN_MASK;

    mode = MODE_SEND;
    TCNT = 0;
    uint8_t sum = 0;

    sendbit(1);
    for (uint8_t i = 0; i<len; i++) {
        sendbyte(d[i]);
        sum += d[i];
    }
    sendbyte(0 - sum);

    OUT_PORT &= ~OUT_PIN_MASK;
    OUT_DDR &= ~OUT_PIN_MASK;

    TCNT = 0;
    OCRA = TLIMIT;
    last_tcnt = 0;
    mode = MODE_IDLE;

#if USE_COMPARATOR
    ACSR |= (1 << ACIE) | (1 << ACIS1);
#else
    EIMSK = 0x01;
#endif
    GIMSK |= (1 << PCIE);
}

void tx(uint8_t data) {
    while(mode != MODE_IDLE);
    buf = data;
    bit_count = 0;
    TX_PORT &= ~TX_PIN;
    TCNT = 0;
    OCRA = 200;
    compa_count = 0;
    mode = MODE_TX;
    while(mode != MODE_TX);
}

int main(void) __attribute__((OS_main));
int main(void) {
    OSCCAL = 0x61;
    RX_PORT = 0x04 | RX_PIN_MSK; // INPUT pull up.
    TX_DDR = TX_PIN; // TxD
    TX_PORT |= TX_PIN;

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

    RX_INT_REG |= (1 << RX_INT); // if use uart_rx
    GIMSK |= (1 << PCIE);

    // timer1
    TIMSK0 = 1 << OCIE0A;
    OCRA = TLIMIT;
    TCNT = 0;
    TCCR0A = (1 << WGM01); // TOP=OCRxA
    TCCR0B = (1 << CS00);

    sei();

    uint8_t o_count = 0;
    for (;;) {
        _delay_ms(1);
        if (uart_recv_data != 0) {
            if (uart_recv_data == 'O') {
                o_count++;
            } else {
                o_count = 0;
            }
            uart_recv_data = 0;
        }
        //  for debug...
        if ((PINB & 0x04) == 0 || o_count == 3) {
        //if ((PINB & 0x08) == 0) {
            o_count = 0;
            //uint8_t cmd[] = {0x40, ROOM, 0x68}; // ping
            //uint8_t cmd[] = {0xC0, ROOM, 0x1c}; // off
            //uint8_t cmd[] = {0x40, ROOM, 0x05, 0xc0}; // call
            uint8_t cmd[] = {0xC0, ROOM, 0x45, 0x8F}; // start
            send_message(cmd, 4);
            _delay_ms(140); // Send before '1C' (reject?).
            cmd[3] = 0x8C;  // open
            send_message(cmd, 4);
            _delay_ms(200);
            _delay_ms(200);
        }
    }
}
