//SENSOR: Eyes of the system
//what it does: Reads input from hardware (Hall sensor / encoder)
//Converts physical signal → digital value
//Updates encoder count
//Where it is used: Called inside sensor thread (in closed_loop_control_system.c)
//with the help of the sensor the system becomes closed loop
#include <stdio.h>      // for input/output functions like fgets
#include <stdlib.h>     // for popen(), pclose()
#include <string.h>     // for string functions like strstr
#include "shared.h"     // shared data structure and mutex

shared_data_t data;         // global shared data (used by all threads)
pthread_mutex_t lock;      // mutex lock to protect shared data

// function to read GPIO pin value (0 or 1)
int read_gpio(int pin)
{
    char cmd[100], buffer[128];  // command string and output buffer

    // create command like: gpio-bcm2711 get 22
    sprintf(cmd, "gpio-bcm2711 get %d", pin);

    // run command and open pipe to read output
    FILE *fp = popen(cmd, "r");

    // if pipe fails, return 0
    if (!fp) return 0;

    // read output from command into buffer
    fgets(buffer, sizeof(buffer), fp);

    // close the pipe
    pclose(fp);

    // check if output contains "level=1"
    // if yes return 1, else return 0
    return strstr(buffer, "level=1") ? 1 : 0;
}

// sensor task function (reads LM393 hall effect sensor and counts pulses)
int sensor_task()
{
    static int last = 1;      // store previous sensor value
    static int encoder = 0;   // store encoder count

    int hall = read_gpio(22);  // read hall sensor from GPIO pin 22

    static int debounce = 0;   // debounce flag to avoid multiple counts

    // detect falling edge (1 → 0 transition)
    // and check debounce is not active
    if (last == 1 && hall == 0 && debounce == 0) {
        encoder++;      // increase encoder count
        debounce = 1;   // activate debounce
    }

    // if signal goes HIGH again, reset debounce
    if (hall == 1)
        debounce = 0;

    last = hall;  // store current value as last for next cycle

    return encoder;  // return updated encoder count
}
