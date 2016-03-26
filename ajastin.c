/**
 * Ajastin
 * Copyright (C) Tomi Leppänen, 2016
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * This code is made for AtTiny85, but should work with AtTiny45 too.
 * Use 1 MHz clock with this code. It is quite safe to disable brown-out
 * detection as this code doesn't write any memory. The code runs
 * ISR(TIMER0_COMPA_vect) at about 10 Hz. Time is adjusted by TIME in
 * seconds.
 */

/* vim: set tw=72 */

#include <avr/cpufunc.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/sleep.h>
#include <util/delay.h>

// Register definitions
#define PORTB_VALUE 0x00
#define DDRB_VALUE 0x1F
#define TCCR0A_VALUE (1 << WGM01)
#define TCCR0B_VALUE ((1 << CS02) | (1 << CS00))
#define OCR0A_VALUE 96;
#define TIMSK_VALUE (1 << OCIE0A)

// Pin definition(s)
#define LED_ON (1 << PB4)

// Other
#define TIME 60
#define MULTIPLIER 10
#define MAXIMUM 15

volatile unsigned char counter;
volatile unsigned int time;
volatile unsigned char half;

ISR(TIMER0_COMPA_vect)
{
    if (++time > TIME * MULTIPLIER) {
        counter++;
        time = 0;
        half = 0;
    } else if (time > TIME/2 * MULTIPLIER) {
        half = 1;
    }
}

int main(void)
{
    unsigned char toggle = 0;
    { // Setup
        cli();
        set_sleep_mode(SLEEP_MODE_IDLE);
        PORTB = PORTB_VALUE;
        DDRB = DDRB_VALUE;
        TCCR0A = TCCR0A_VALUE;
        TCCR0B = TCCR0B_VALUE;
        OCR0A = OCR0A_VALUE;
        TIMSK = TIMSK_VALUE;
        counter = 0;
        time = 0;
        half = 0;
        PORTB = LED_ON;
        sei();
    }
    // Display
    while (counter < MAXIMUM)
    {
        if (half) {
            if (toggle)
                PORTB = LED_ON | (counter+1);
            else
                PORTB = LED_ON | counter;
            toggle = !toggle;
        } else {
            PORTB = LED_ON | counter;
        }
        sleep_mode();
    }
    PORTB &= ~LED_ON;
    return 0;
}
