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

PROCESS(task2, "task2");
AUTOSTART_PROCESSES(&task2);

static int state;
static int num_buzzed;

#define IDLE 0
#define BUZZ 1
#define WAIT 2
/*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*/
static void init_mpu_reading(void);
static void get_mpu_reading(void);

static void print_mpu_reading(int reading)
{
  if (reading < 0)
  {
    printf("-");
    reading = -reading;
  }

  printf("%d.%02d", reading / 100, reading % 100);
}

static void init_mpu_reading(void)
{
  mpu_9250_sensor.configure(SENSORS_ACTIVE, MPU_9250_SENSOR_TYPE_ALL);
}

static int sample_IMU_with_prints() {
    int total_reading = 0;
    int value;

    // printf("MPU Gyro: X=");
    // value = mpu_9250_sensor.value(MPU_9250_SENSOR_TYPE_GYRO_X);
    // print_mpu_reading(value);
    // printf(" deg/sec\n");
    // total_reading += value;

    // printf("MPU Gyro: Y=");
    // value = mpu_9250_sensor.value(MPU_9250_SENSOR_TYPE_GYRO_Y);
    // print_mpu_reading(value);
    // printf(" deg/sec\n");
    // total_reading += value;

    // printf("MPU Gyro: Z=");
    // value = mpu_9250_sensor.value(MPU_9250_SENSOR_TYPE_GYRO_Z);
    // print_mpu_reading(value);
    // printf(" deg/sec\n");
    // total_reading += value;

    printf("MPU Acc: X=");
    value = mpu_9250_sensor.value(MPU_9250_SENSOR_TYPE_ACC_X);
    print_mpu_reading(value);
    printf(" G\r\n");
    total_reading += value;

    printf("MPU Acc: Y=");
    value = mpu_9250_sensor.value(MPU_9250_SENSOR_TYPE_ACC_Y);
    print_mpu_reading(value);
    printf(" G\r\n");
    total_reading += value;

    printf("MPU Acc: Z=");
    value = mpu_9250_sensor.value(MPU_9250_SENSOR_TYPE_ACC_Z);
    print_mpu_reading(value);
    printf(" G\r\n");
    total_reading += value;

    return total_reading;
}

static int sample_IMU_acc() {
    int total_reading = 0;
    int value;

    // value = mpu_9250_sensor.value(MPU_9250_SENSOR_TYPE_GYRO_X);
    // total_reading += value;

    // value = mpu_9250_sensor.value(MPU_9250_SENSOR_TYPE_GYRO_Y);
    // total_reading += value;

    // value = mpu_9250_sensor.value(MPU_9250_SENSOR_TYPE_GYRO_Z);
    // total_reading += value;

    value = mpu_9250_sensor.value(MPU_9250_SENSOR_TYPE_ACC_X);
    total_reading += value;

    value = mpu_9250_sensor.value(MPU_9250_SENSOR_TYPE_ACC_Y);
    total_reading += value;

    value = mpu_9250_sensor.value(MPU_9250_SENSOR_TYPE_ACC_Z);
    total_reading += value;

    return total_reading;
}

static void wait(float seconds)
{
    clock_time_t curr_tick;
    clock_time_t end_tick;
    curr_tick = clock_time();
    end_tick = curr_tick + (seconds * CLOCK_SECOND);
    
    while (clock_time() < end_tick);
}

PROCESS_THREAD(task2, ev, data)
{
    PROCESS_BEGIN();
    init_mpu_reading();
    buzzer_init();
    buzzer_start(2794);
    wait(1);
    buzzer_stop();
    bool sampled = false;
    int prev_IMU_reading;
    int curr_IMU_reading;
    state = IDLE;
    while (1) {
        switch (state) {
            case IDLE:
                // sample IMU readings for any significant change every 0.025s
                wait(0.025);
                curr_IMU_reading = sample_IMU_with_prints();
                printf("current IMU reading: %d\r\n", curr_IMU_reading);
                // significant motion detected
                if (sampled && (curr_IMU_reading - prev_IMU_reading)/prev_IMU_reading > 0.5) {
                    printf("Significant motion detected!\r\n");
                    state = BUZZ;
                    num_buzzed = 0;
                }
                prev_IMU_reading = curr_IMU_reading;
                sampled = true;
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

