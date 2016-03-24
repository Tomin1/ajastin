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
 * This is a small timer program for AtTiny85
 *
 * This code is made for AtTiny85, but should work with AtTiny[24]5 too.
 * Use 1 MHz clock with this code. It is quite safe to disable brown-out
 * detection as this code doesn't write any memory. The code runs
 * ISR(TIMER0_COMPA_vect) (FREQUENCY * NUMBER_OF_LEDS) times a second.
 * Time is adjusted by TIME in seconds.
 *
 * The code uses charlieplexing to drive 5 leds. Therefore every led
 * will be lit FREQUENCY times per second and their duty cycle is
 * 1/NUMBER_OF_LEDS.
 */

/* vim: set tw=72 */

#include <avr/cpufunc.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/sleep.h>
#include <stdint.h>
#include <util/delay.h>

// Register definitions
#define TCCR0A_VALUE (1 << WGM01)
#define TCCR0B_VALUE ((1 << CS01) | (1 << CS00))
#define OCR0A_VALUE 61;
#define TIMSK_VALUE (1 << OCIE0A)

// Other
#define TIME 60
#define FREQUENCY 50
#define MAXIMUM 0x0F
#define NUMBER_OF_LEDS 5
#define LED_ON (1 << 4)

volatile const unsigned char LEDS_HIGH[] = { // PORTB values
    0b01000,
    0b00010,
    0b10000,
    0b00010,
    0b01000
};

volatile const unsigned char LEDS_INPUT[] = { // DDRB values
    0b01010,
    0b10010,
    0b10010,
    0b01010,
    0b11000
};

volatile uint8_t counter;
volatile uint8_t leds;

ISR(TIMER0_COMPA_vect)
{
    static uint8_t multiplier = 0;
    static uint8_t time = 0;
    uint8_t state = (multiplier % NUMBER_OF_LEDS);
    PORTB = 0;
    if (leds & (1 << state)) {
        DDRB = LEDS_INPUT[state];
        PORTB = LEDS_HIGH[state];
    }
    if (multiplier >= FREQUENCY*NUMBER_OF_LEDS) {
        if (counter < MAXIMUM) {
            if (++time >= TIME) {
                counter++;
                time = 0;
            }
        }
        multiplier = 0;
    } else {
        ++multiplier;
    }
    OCR0A ^= 2; // Switch between values 61 and 62
}

int main(void)
{
    { // Start the timer
        cli();
        set_sleep_mode(SLEEP_MODE_IDLE);
        TCCR0A = TCCR0A_VALUE;
        TCCR0B = TCCR0B_VALUE;
        OCR0A = OCR0A_VALUE;
        TIMSK = TIMSK_VALUE;
        counter = 0;
        leds = LED_ON;
        sei();
    }
    {// Display the timer while it's running
        while (counter < MAXIMUM)
        {
            leds = counter | LED_ON;
            sleep_mode();
        }
        leds = counter;
    }
    while (1) // Timer finished, sleep forever!
        sleep_mode();
    return 0;
}
