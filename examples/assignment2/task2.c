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
/*---------------------------------------------------------------------------*/
static int get_light_reading();
static void init_opt_reading();
/*---------------------------------------------------------------------------*/

static int get_light_reading() {
    int value = opt_3001_sensor.value(0);
    if (value != CC26XX_SENSOR_READING_ERROR) {
        printf("no reading error!\r\n");
        init_opt_reading();
        return value / 100;
    }
    
    printf("Light sensor is warming up...\r\n");
    init_opt_reading();
    return 0;
}   


static void wait(int seconds)
{
    clock_time_t curr_tick;
    clock_time_t end_tick;
    curr_tick = clock_time();
    end_tick = curr_tick + seconds * CLOCK_SECOND;
    
    while (clock_time() < end_tick);
}

static void init_opt_reading(void) {
    SENSORS_ACTIVATE(opt_3001_sensor);
}

PROCESS_THREAD(task2, ev, data)
{
    PROCESS_BEGIN();
    init_opt_reading();
    wait(1);

    state = IDLE;
    int curr_light_intensity = get_light_reading();
    int prev_light_intensity = curr_light_intensity;
    printf("Curr light intensity: %d | Prev light intensity: %d\r\n", curr_light_intensity, prev_light_intensity);


    clock_time_t curr_light_ticks = clock_time();
    clock_time_t prev_light_ticks = curr_light_ticks;
    const int LIGHT_TICKS_REQUIRED = CLOCK_SECOND / LIGHT_SENSING_FREQUENCY;

    while (1) {
        // every 32 ticks, detect curr_light_intensity
        if (state == IDLE) {
            curr_light_ticks = clock_time();
            // printf("Current light ticks: %d | Prev light ticks: %d\r\n", curr_light_ticks, prev_light_ticks);

            if (curr_light_ticks - prev_light_ticks >= LIGHT_TICKS_REQUIRED) {
                init_opt_reading();
                prev_light_ticks = curr_light_ticks;
                prev_light_intensity = curr_light_intensity;
                curr_light_intensity = get_light_reading();
                printf("Curr light intensity: %d | Prev light intensity: %d\r\n", curr_light_intensity, prev_light_intensity);

            }

        }

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
