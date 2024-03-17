# Assignment 2

## Task 1

### For `etimer`

![Clock Second Screenshot](https://github.com/STAung07/contiki-ng/blob/WORKING/examples/assignment2/clock%20second.jpg?raw=true)
`CLOCK_SECOND = 128`

![Clock ticks per second Screenshot](https://github.com/STAung07/contiki-ng/blob/WORKING/examples/assignment2/clock%20ticks.jpg?raw=true)
Taking the clock ticks count between 2 consecutive prints, for example, 24103-23975=128.
`number of clock ticks = 128 per second`

### For `rtimer`

![RTIMER SECOND Screenshot](https://github.com/STAung07/contiki-ng/blob/WORKING/examples/assignment2/rtimer%20second.jpg?raw=true)
`RTIMER_SECOND = 65536`

`RTIMER clock ticks = 16384`

## Instruction of running the program

Ensure that you are in Linux environment and with Contiki OS and UniFlash software installed. You may refer to the [setup guide](https://weiserlab.github.io/wirelessnetworking/Assignment1_Windows.pdf) to install and set up all necessary dependencies if you haven't done so.

Furthermore, to run our programs for task 2 and task 3, ensure you copy the `Makefile` to the same directory of `task2.c` and `task3.c`.

### Task 2

1. Run the following command to compile and build the binary file:
   ```
   make TARGET=cc26x0-cc13x0 BOARD=sensortag/cc2650 task2
   ```
2. You will see a binary file called `task2.cc26x0-cc13x0` generated in the same folder.
3. Load `task2.cc26x0-cc13x0` to UniFlash and run the program on the sensor. Then you may start monitoring the behaviours of the CC2650 microcontroller.

### Task 3

1. Run the following command to compile and build the binary file:
   ```
   make TARGET=cc26x0-cc13x0 BOARD=sensortag/cc2650 task3
   ```
2. You will see a binary file called `task3.cc26x0-cc13x0` generated in the same folder.
3. Load `task3.cc26x0-cc13x0` to UniFlash and run the program on the sensor. Then you may start monitoring the behaviours of the CC2650 microcontroller.

## Group member

| Name                 | Student ID |
| -------------------- | ---------- |
| Galvin Chan Shi Yuan | A0217812J  |
| Mun Le Zong          | A0218136J  |
| Swann Tet Aung       | A0217516H  |
| Tan Rui Yang         | A0219814B  |
