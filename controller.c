//Controller: Brain of the system
//What it does: It takes setpoint(desired speed),encoder(actual speed) and calculates error difference
//-->Applies control logic to PID Produces output to speed value
//Where it is used: Called inside controller thread (in closed_loop_control_system.c)
//controller_task()
//Why we use it: To decide what to do
//Without controller:
//-->motor runs blindly
//With controller:
//-->system corrects itself automatically ✅
#include <stdlib.h>
#include "shared.h"

#define KP 1.2
#define KI 0.05
#define KD 0.3

void controller_task()
{
    pthread_mutex_lock(&lock);

    int error = data.setpoint - data.encoder;

    data.integral += error;
    if (data.integral > 50) data.integral = 50;
    if (data.integral < -50) data.integral = -50;

    int derivative = error - data.prev_error;

    float output = (KP * error) +
                   (KI * data.integral) +
                   (KD * derivative);

    if (abs(error) < 5)
        output = output / 2;

    data.prev_error = error;

    if (output > 0)
        data.direction = 1;
    else if (output < 0)
        data.direction = -1;
    else
        data.direction = 0;

    if (output < 0) output = -output;

    if (output > 100) output = 100;

    data.speed = (int)output;

    if (data.speed > 0 && data.speed < 60)
        data.speed = 60;

    if (abs(error) <= 2) {
        data.speed = 0;
        data.direction = 0;
        data.integral = 0;
    }

    pthread_mutex_unlock(&lock);
}
