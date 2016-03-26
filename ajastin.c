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
 *
 * Piezo is driven by TIMER0_COMPA vector so it runs approximately at
 * the frequency of (FREQUENCY * NUMBER_OF_LEDS).
 *
 * Usage:
 * Power on. Set time by pressing the button until the time you want is
 * shown. Wait for about BUTTON_WAIT_TIME seconds. Now the LED_ON led
 * lights and the timer starts counting. When it finished the piezo
 * speaker starts buzzing. You can stop it and power down the counter by
 * pressing the button. To restart the timer press reset button.  The
 * button doesn't do anything while counting and the reset button resets
 * the timer back to setting the timer.
 */

/* vim: set tw=72 */

#include <avr/cpufunc.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/sleep.h>
#include <stdint.h>
#include <util/delay.h>

#define TIME 60
#define FREQUENCY 50
#define MAXIMUM 0x0F
#define NUMBER_OF_LEDS 5
#define LED_ON (1 << 4)
#define PORTB_NULL (1 << PB2)
#define BUTTON (1 << PB2)
#define BUTTON_WAIT_TIME 5
#define PIEZO (1 << PB4)

const unsigned char LEDS_HIGH[] = { // PORTB values
    0b01100,
    0b00101,
    0b00110,
    0b00101,
    0b01100
};

const unsigned char LEDS_INPUT[] = { // DDRB values
    0b11001,
    0b10011,
    0b10011,
    0b11001,
    0b11010
};

volatile uint8_t counter;
volatile uint8_t counter_max;
volatile uint8_t leds;
volatile uint8_t stopped;
// stopped is the state of this program:
// 0 = timer running,
// 1 = counter_max not determined (before timer),
// 2 = counter_max refreshed (before timer),
// 3 = counter_max determined (before timer),
// 4 = piezo active (after timer),
// 5 = timer finished (after timer)

ISR(TIMER0_COMPA_vect)
{
    static uint8_t multiplier = 0;
    static uint8_t time = 0;
    static uint8_t piezo = 0;
    uint8_t state = (multiplier % NUMBER_OF_LEDS);
    PORTB = PORTB_NULL; // Drive the leds
    if (leds & (1 << state)) {
        DDRB = LEDS_INPUT[state];
        PORTB = LEDS_HIGH[state];
    }
    if (stopped == 4 && piezo | 1) { // Run the buzzer
        piezo ^= 2;
        if (piezo | 2)
            PORTB |= PIEZO;
        else
            PORTB &= ~PIEZO;
    }
    if (multiplier >= FREQUENCY*NUMBER_OF_LEDS) {
        switch (stopped) {
            case 0: // While counter is active
                if (++time >= TIME) {
                    counter++;
                    time = 0;
                }
                break;
            case 1: // While determining counter_max
                if (++time >= BUTTON_WAIT_TIME) {
                    stopped = 3;
                    time = 0;
                }
                break;
            case 2: // Above and if counter_max was refreshed
                time = 0;
                stopped = 1;
                break;
            case 4: // Start or stop piezo
                piezo ^= 1;
                break;
            default:
                break;
        }
        multiplier = 0;
    } else {
        multiplier++;
    }
    OCR0A ^= 2; // Switch between values 61 and 62
}

ISR(INT0_vect)
{
    if (stopped < 3) {
        if (++counter_max > MAXIMUM)
            counter_max = 1;
        stopped = 2;
    } else if (stopped == 4) {
        cli();
        stopped = 5;
    }
}

int main(void)
{
    { // Set up timers, sleep and such
        cli();
        set_sleep_mode(SLEEP_MODE_IDLE);
        PRR = ((1 << PRTIM1) | (1 << PRUSI) | (1 << PRADC));
        // TIMER0
        TCCR0A = (1 << WGM01);
        TCCR0B = ((1 << CS01) | (1 << CS00));
        OCR0A = 61;
        OCR0B = 0;
        TIMSK = (1 << OCIE0A);
        // INT0
        MCUCR |= (1 << ISC01);
        GIMSK = (1 << INT0);
        PORTB = PORTB_NULL;
        counter = leds = 0;
        counter_max = MAXIMUM;
        stopped = 1;
        sei();
    }
    { // Set up the counter and start
        while (stopped < 3)
            leds = counter_max;
            sleep_mode();
        leds = LED_ON;
    }
    { // Prepare for the timer
        cli();
        GIMSK = 0; // Stop INT0
        stopped = 0;
        TCNT0 = 0; // Clear TIMER0
        sei();
    }
    { // Display the timer while it's running
        while (counter < counter_max)
        {
            leds = counter | LED_ON;
            sleep_mode();
        }
        stopped = 4;
        leds = counter;
    }
    { // Yell!
        cli();
        GIMSK = (1 << INT0); // Start INT0
        sei();
        while (stopped == 4) // Sleep!
            sleep_mode();
        GIMSK = 0; // Stop INT0
        TCCR0A &= ~((1 << COM0B1) | (1 << COM0B0)); // Stop yelling!
    }
    { // Timer finished, sleep forever!
        set_sleep_mode(SLEEP_MODE_PWR_DOWN);
        sleep_mode();
    }
    return 0;
}
