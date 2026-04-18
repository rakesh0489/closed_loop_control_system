#include <stdio.h>      // for printf()
#include <time.h>       // for time functions like clock_gettime
#include <pthread.h>    // for threads and mutex
#include <unistd.h>     // for usleep()
#include <stdlib.h>     // for system() and exit functions
#include <signal.h>     // for signal handling (Ctrl+C etc.)
#include <sched.h>      // for thread scheduling (priority)
#include "shared.h"     // shared data between threads

#define PERIOD_NS 10000000   // time period = 10 milliseconds (in nanoseconds)

// external functions (defined in other files)
extern int sensor_task();        // function to read sensor
extern void controller_task();   // function for control logic
extern void actuator_task();     // function to control motor/LEDs

volatile int running = 1;   // flag to control program execution

// function to handle stop signal
void stop_handler(int sig)
{
    running = 0;   // stop all loops

    // immediately stop motor (PWM and direction pins OFF)
    system("gpio-bcm2711 set 13 dl"); // PWM OFF
    system("gpio-bcm2711 set 17 dl"); // IN1 OFF
    system("gpio-bcm2711 set 27 dl"); // IN2 OFF

    // turn OFF all LEDs
    system("gpio-bcm2711 set 5 dl");
    system("gpio-bcm2711 set 6 dl");
    system("gpio-bcm2711 set 12 dl");
    system("gpio-bcm2711 set 16 dl");
    system("gpio-bcm2711 set 20 dl");
}
 // SENSOR THREAD
void* sensor_thread(void* arg)
{
    struct timespec next;  // structure to store next wake-up time

    clock_gettime(CLOCK_MONOTONIC, &next); // get current time

    // loop runs until program is stopped
    while (running)
    {
        next.tv_nsec += PERIOD_NS;  // add 10ms to next execution time

        // handle overflow of nanoseconds
        if (next.tv_nsec >= 1000000000) {
            next.tv_sec++;               // increase seconds
            next.tv_nsec -= 1000000000;  // adjust nanoseconds
        }

        // sleep until exact next time
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next, NULL);

        int enc = sensor_task();  // read encoder value

        pthread_mutex_lock(&lock);   // lock shared data
        data.encoder = enc;          // update encoder value
        pthread_mutex_unlock(&lock); // unlock after update
    }
    return NULL;  // exit thread
}

// CONTROLLER THREAD
void* controller_thread(void* arg)
{
    struct timespec next;  // structure for timing

    clock_gettime(CLOCK_MONOTONIC, &next); // get current time

    while (running)
    {
        next.tv_nsec += PERIOD_NS;  // schedule next run

        // adjust time if overflow happens
        if (next.tv_nsec >= 1000000000) {
            next.tv_sec++;
            next.tv_nsec -= 1000000000;
        }

        // wait until exact time
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next, NULL);

        controller_task();  // execute control algorithm
    }
    return NULL;  // exit thread
}

// ---------------- ACTUATOR THREAD ----------------
void* actuator_thread(void* arg)
{
    struct timespec next;  // structure for timing

    clock_gettime(CLOCK_MONOTONIC, &next); // get current time

    while (running)
    {
        next.tv_nsec += PERIOD_NS;  // schedule next run

        // adjust overflow
        if (next.tv_nsec >= 1000000000) {
            next.tv_sec++;
            next.tv_nsec -= 1000000000;
        }

        // wait until exact time
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next, NULL);

        actuator_task();  // update motor and LEDs
    }
    return NULL;  // exit thread
}

// function to clean up system (turn OFF everything)
void cleanup()
{
    // repeat multiple times to ensure proper shutdown
    for (int i = 0; i < 5; i++) {

        // turn OFF motor pins
        system("gpio-bcm2711 set 13 dl");
        system("gpio-bcm2711 set 17 dl");
        system("gpio-bcm2711 set 27 dl");

        // turn OFF all LEDs
        system("gpio-bcm2711 set 5 dl");
        system("gpio-bcm2711 set 6 dl");
        system("gpio-bcm2711 set 12 dl");
        system("gpio-bcm2711 set 16 dl");
        system("gpio-bcm2711 set 20 dl");

        usleep(10000);  // small delay (10ms)
    }

    printf("\nSYSTEM STOPPED\n");  // print message
}

// ---------------- MAIN FUNCTION ----------------
int main()
{
    pthread_t t_sensor, t_controller, t_actuator; // thread variables
    struct sched_param param;  // structure for priority settings

    signal(SIGINT, stop_handler);   // handle Ctrl+C
    signal(SIGTERM, stop_handler);  // handle termination signal

    atexit(cleanup);  // call cleanup when program exits

    pthread_mutex_init(&lock, NULL); // initialize mutex

    // initialize shared data values
    data.encoder = 0;
    data.setpoint = 50;   // desired speed
    data.integral = 0;    // for PI/PID control
    data.prev_error = 0;  // previous error

    printf("PROGRAM STARTED\n");  // print start message

    // 🔥 SENSOR THREAD (MEDIUM PRIORITY)
    param.sched_priority = 15;  // set priority
    pthread_create(&t_sensor, NULL, sensor_thread, NULL); // create thread
    pthread_setschedparam(t_sensor, SCHED_RR, &param);    // apply scheduling

    // 🔥 CONTROLLER THREAD (HIGH PRIORITY)
    param.sched_priority = 20;  // higher priority
    pthread_create(&t_controller, NULL, controller_thread, NULL);
    pthread_setschedparam(t_controller, SCHED_RR, &param);

    // 🔥 ACTUATOR THREAD (MEDIUM PRIORITY)
    param.sched_priority = 15;  // medium priority
    pthread_create(&t_actuator, NULL, actuator_thread, NULL);
    pthread_setschedparam(t_actuator, SCHED_RR, &param);

    // wait until all threads finish
    pthread_join(t_sensor, NULL);
    pthread_join(t_controller, NULL);
    pthread_join(t_actuator, NULL);

    cleanup();  // final cleanup call

    return 0;  // end program
}
