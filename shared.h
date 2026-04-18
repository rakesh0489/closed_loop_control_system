#ifndef PID_SHARED_H              // Prevent multiple inclusion of this header
#define PID_SHARED_H

#include <pthread.h>             // Required for mutex (thread synchronization)

// Shared structure between sensor, controller, actuator
typedef struct {
    int encoder;                 // Feedback value (from Hall sensor)
    int speed;                   // PWM speed (0–100)
    int direction;               // Motor direction (1 forward, -1 reverse, 0 stop)
    int setpoint;                // Target value (from rotary encoder)
    float integral;              // Integral term for PID
    int prev_error;              // Previous error (for derivative)

} shared_data_t;

// Declare shared variables (defined in sensor.c)
extern shared_data_t data;       // Global shared data
extern pthread_mutex_t lock;    // Mutex for safe access

#endif

