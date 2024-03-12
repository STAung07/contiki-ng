/*
 * Copyright (C) 2015, Intel Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include "contiki.h"
#include "sys/etimer.h"
#include "sys/rtimer.h"
#include "buzzer.h"
#include "board-peripherals.h"
#include <stdint.h>
#include <stdlib.h>

PROCESS(task2, "task2");
AUTOSTART_PROCESSES(&task2);

static int state;
static int num_buzzed;

#define IDLE 0
#define BUZZ 1
#define WAIT 2

#define LIGHT_INTENSITY_THRESHOLd 300
#define LIGHT_SENSING_FREQUENCY 2

static int prev_light_intensity = 0;
static int curr_light_intensity = 0;

// RTimer for light sensor
static struct rtimer timer_rtimer;
static rtimer_clock_t timeout_rtimer = RTIMER_SECOND / LIGHT_SENSING_FREQUENCY;

/*---------------------------------------------------------------------------*/
static int get_light_reading(void);
static void init_opt_reading(void);

/*---------------------------------------------------------------------------*/

static void init_opt_reading(void) {
    SENSORS_ACTIVATE(opt_3001_sensor);
}

static int get_light_reading() {
    int value = opt_3001_sensor.value(0);

    init_opt_reading();

    if (value != CC26XX_SENSOR_READING_ERROR) {
        return value / 100;
    }

    printf("Light sensor is warming up...\r\n");
    return 0;
}   

void do_rtimer_timeout(struct rtimer *timer, void *ptr) {
    rtimer_clock_t now = RTIMER_NOW();

    rtimer_set(&timer_rtimer, RTIMER_NOW() + timeout_rtimer, 0, do_rtimer_timeout, NULL);

    prev_light_intensity = curr_light_intensity;
    curr_light_intensity = get_light_reading();
}

static void wait(int seconds)
{
    clock_time_t curr_tick;
    clock_time_t end_tick;
    curr_tick = clock_time();
    end_tick = curr_tick + seconds * CLOCK_SECOND;
    
    while (clock_time() < end_tick);
}

PROCESS_THREAD(task2, ev, data)
{
    PROCESS_BEGIN();
    init_opt_reading();

    state = IDLE;

    while (1) {
        rtimer_set(&timer_rtimer, RTIMER_NOW() + timeout_rtimer, 0, do_rtimer_timeout, NULL);
        PROCESS_YIELD();

        switch (state) {
            case IDLE :
                // TODO: add condition for IMU
                if (abs(curr_light_intensity - prev_light_intensity) > 300) {
                    state = BUZZ;
                    num_buzzed = 0;
                }
                break;

            case BUZZ:
                if (num_buzzed == 4){
                    state = IDLE;
                } else {
                    num_buzzed++;
                    buzzer_start(2794);
                    wait(2);
                    state = WAIT;
                }
                break;

            case WAIT:
                buzzer_stop();
                wait(2);
                state = BUZZ;
                break;

            default:
                // DIE
                break;
        }
    }

    PROCESS_END();
}
