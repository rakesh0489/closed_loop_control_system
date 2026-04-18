//Actuator: Muscles of the system
//What it does: Takes output from controller (speed, direction)
//which Controls:
//Motor (PWM)
//Direction pins
//LEDs
//Where it is used: called inside actuator thresd (main.c)actuator_task();
//Why we use it:
//To convert decision → physical action
//Without actuator:
//system thinks but does nothing
#include <stdio.h>     // for input/output functions like printf
#include <stdlib.h>    // for system and general utilities
#include <unistd.h>    // for usleep() function
#include <pthread.h>   // for multithreading and mutex
#include "shared.h"   //  used to share the data between threads

FILE *logfile = NULL; //used to store  log datain file pointer

void gpio_set(int pin, const char *mode) // function to control GPIO pin (set high or low)
{
    char cmd[100]; // command string
    sprintf(cmd, "gpio-bcm2711 set %d %s", pin, mode);   //  to create command : gpio-bcm2711 set 13 dh
    system(cmd);  // to execute the command in system
}

void pwm(int duty)  //to generate PWM signal
{
    int period = 20000;  // total time period in microseconds

    if (duty <= 0) {     // if duty cycle is 0 or less, turn OFF motor
        gpio_set(13, "dl");  //set pin low
        return;  //exit function
    }

    int on = (period * duty) / 100;  // calculate ON time based on duty cycle
    int off = period - on;           // calculate OFF time

    for (int i = 0; i < 5; i++) {   // repeat PWM signal for some cycles
        gpio_set(13, "dh");         // turn ON (HIGH)
        usleep(on);                  // wait for ON time
        gpio_set(13, "dl");         // turn OFF (LOW)
        usleep(off);                // wait for OFF time
    }
}

void actuator_task()   // main actuator task
{
    if (logfile == NULL) {  // check if log file is not opened yet
        logfile = fopen("/tmp/log.csv", "w");   // open file in write mode

        if (!logfile) return;    //  exit if file not opened

        fprintf(logfile, // write header line in CSV file
        "Setpoint,Encoder,Speed,Direction,Green,Red,Blue,Yellow,White\n");
    }

    pthread_mutex_lock(&lock);  // used to lock the shared data

    int speed = data.speed;      // reads the  speed value
    int dir   = data.direction;   // reads the direction
    int enc   = data.encoder;     // reads the  encoder value
    int sp    = data.setpoint;   // reads the setpoint

    pthread_mutex_unlock(&lock);  // unlock after reading

    if (dir == 0 || speed == 0) {
        // if no direction or speed is zero then stop the motor
        gpio_set(13, "dl");  // PWM pin LOW
        gpio_set(17, "dl");  // direction pin LOW
        gpio_set(27, "dl");
    } else {                   // if direction is forward
        if (dir == 1) {
            gpio_set(17, "dh"); // set forward pin HIGH
            gpio_set(27, "dl"); // set reverse pin LOW
        } else {                 // if direction is reverse
            gpio_set(17, "dl");  //set forward pin LOW
            gpio_set(27, "dh");  //set reverse pin HIGH
        }
        pwm(speed);  // generate PWM signal based on speed
    }
    // LED conditions based on speed and direction
    int led_green  = (dir == 1);  // green for forward
    int led_red    = (dir == -1); // red for reverse
    int led_blue   = (speed > 0 && speed < 40); // blue for low speed
    int led_yellow = (speed >= 40 && speed < 70); // yellow for medium speed
    int led_white  = (speed >= 70);                // white for high speed


    // set LEDs ON or OFF based on conditions
    gpio_set(5,  led_green  ? "dh" : "dl"); // green LED
    gpio_set(6,  led_red    ? "dh" : "dl"); // red LED
    gpio_set(12, led_blue   ? "dh" : "dl"); // blue LED
    gpio_set(16, led_yellow ? "dh" : "dl"); // yellow LED
    gpio_set(20, led_white  ? "dh" : "dl"); // white LED

    fprintf(logfile, "%d,%d,%d,%d,%d,%d,%d,%d,%d\n",       // write data into log file
            sp, enc, speed, dir,
            led_green, led_red,
            led_blue, led_yellow, led_white);

    fflush(logfile);  // force write to file immediately

    printf("[CTRL] SP=%d Enc=%d Speed=%d Dir=%d\n",  // print current status in console
           sp, enc, speed, dir);
    fflush(stdout);
}
