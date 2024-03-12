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
#define IMU_SENSING_FREQUENCY 40

static int prev_light_intensity = 0;
static int curr_light_intensity = 0;
int prev_IMU_reading;
int curr_IMU_reading;
bool sampled = false;

// RTimer for light sensor
static struct rtimer timer_rtimer;
static rtimer_clock_t timeout_rtimer = RTIMER_SECOND / IMU_SENSING_FREQUENCY;
static int ticks;

/*---------------------------------------------------------------------------*/
static int get_light_reading(void);
static void init_opt_reading(void);

/*---------------------------------------------------------------------------*/
static void init_mpu_reading(void);
static void get_mpu_reading(void);

static void init_mpu_reading(void)
{
  mpu_9250_sensor.configure(SENSORS_ACTIVE, MPU_9250_SENSOR_TYPE_ALL);
}

static int sample_IMU_acc() {
    int total_reading = 0;
    int value;

    value = mpu_9250_sensor.value(MPU_9250_SENSOR_TYPE_ACC_X);
    total_reading += value;

    value = mpu_9250_sensor.value(MPU_9250_SENSOR_TYPE_ACC_Y);
    total_reading += value;

    value = mpu_9250_sensor.value(MPU_9250_SENSOR_TYPE_ACC_Z);
    total_reading += value;

    return total_reading;
}

static void init_opt_reading(void) {
    SENSORS_ACTIVATE(opt_3001_sensor);
}

static int get_light_reading() {
    int value = opt_3001_sensor.value(0);

    init_opt_reading();

    if(value != CC26XX_SENSOR_READING_ERROR) {
        init_opt_reading();
        return value / 100;
    }
    
    // printf("Light sensor is warming up...\r\n");
    return 0;
}   

void do_rtimer_timeout(struct rtimer *timer, void *ptr) {
    if (ticks == 10) {
        ticks = 0;
        
        prev_light_intensity = curr_light_intensity;
        curr_light_intensity = get_light_reading();
        // printf("Current Light: %d\r\n", curr_light_intensity);
    }

    ticks++;
    prev_IMU_reading = curr_IMU_reading;
    curr_IMU_reading = sample_IMU_acc();
    // printf("current IMU reading: %d\r\n", curr_IMU_reading);
    sampled = true;
    rtimer_clock_t now = RTIMER_NOW();

    rtimer_set(&timer_rtimer, RTIMER_NOW() + timeout_rtimer, 0, do_rtimer_timeout, NULL);
}

static void wait(int seconds)
{
    rtimer_clock_t curr_tick;
    rtimer_clock_t end_tick;
    curr_tick = RTIMER_NOW();
    end_tick = curr_tick + seconds * RTIMER_SECOND;
    
    while (RTIMER_NOW() < end_tick);
}

PROCESS_THREAD(task2, ev, data)
{
    PROCESS_BEGIN();
    init_opt_reading();
    init_mpu_reading();
    rtimer_set(&timer_rtimer, RTIMER_NOW() + timeout_rtimer, 0, do_rtimer_timeout, NULL);

    state = IDLE;
    ticks = 0;
    while (1) {
        switch (state) {
            case IDLE :
                // TODO: add condition for IMU
                
                if (abs(curr_light_intensity - prev_light_intensity) > 300) {
                    state = BUZZ;
                    num_buzzed = 0;
                }
                if (abs(curr_IMU_reading - prev_IMU_reading) > 100) {   
                    // printf("Significant motion detected!\r\n");
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
        PROCESS_YIELD();
    }

    PROCESS_END();
}
