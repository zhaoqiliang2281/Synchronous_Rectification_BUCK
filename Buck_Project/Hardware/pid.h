#ifndef PID_H_
#define PID_H_

#include "main.h"

typedef struct {
    float kp, ki, kd;
    float target;
    float limit;       // 输出增量限幅
    float integral;
    float last_error;
    float last2_error;
    float out;
} PID_T;

void  pid_init(PID_T *p, float kp, float ki, float kd, float target, float limit);
void  pid_set_target(PID_T *p, float target);
void  pid_reset(PID_T *p);
float pid_calc_incremental(PID_T *p, float current);


void pid_set_params(PID_T * p, float _kp, float _ki, float _kd);
void pid_get_params(PID_T * p, float *_kp, float *_ki, float *_kd);
float pid_get_target(PID_T *p);
float pid_get_out(PID_T *p);

#endif
