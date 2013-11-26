/*
 * File: simulinkmodeltoblinkLed.h
 *
 * Code generated for Simulink model 'simulinkmodeltoblinkLed'.
 *
 * Model version                  : 1.1
 * Simulink Coder version         : 8.5 (R2013b) 08-Aug-2013
 * C/C++ source code generated on : Fri Nov 08 20:41:42 2013
 *
 * Target selection: ert.tlc
 * Embedded hardware selection: ARM Compatible->ARM Cortex
 * Code generation objectives: Unspecified
 * Validation result: Not run
 */

#ifndef RTW_HEADER_simulinkmodeltoblinkLed_h_
#define RTW_HEADER_simulinkmodeltoblinkLed_h_
#ifndef simulinkmodeltoblinkLed_COMMON_INCLUDES_
# define simulinkmodeltoblinkLed_COMMON_INCLUDES_
#include <stddef.h>
#include <string.h>
#include "rtwtypes.h"
#endif                                 /* simulinkmodeltoblinkLed_COMMON_INCLUDES_ */

#include "simulinkmodeltoblinkLed_types.h"

/* Macros for accessing real-time model data structure */
#ifndef rtmGetErrorStatus
# define rtmGetErrorStatus(rtm)        ((rtm)->errorStatus)
#endif

#ifndef rtmSetErrorStatus
# define rtmSetErrorStatus(rtm, val)   ((rtm)->errorStatus = (val))
#endif

/* Block states (auto storage) for system '<Root>' */
typedef struct {
  int32_T clockTickCounter;            /* '<Root>/Pulse Generator' */
} DW_simulinkmodeltoblinkLed_T;

/* External outputs (root outports fed by signals with auto storage) */
typedef struct {
  real_T Out1;                         /* '<Root>/Out1' */
} ExtY_simulinkmodeltoblinkLed_T;

/* Parameters (auto storage) */
struct P_simulinkmodeltoblinkLed_T_ {
  real_T PulseGenerator_Amp;           /* Expression: 1
                                        * Referenced by: '<Root>/Pulse Generator'
                                        */
  real_T PulseGenerator_Period;        /* Computed Parameter: PulseGenerator_Period
                                        * Referenced by: '<Root>/Pulse Generator'
                                        */
  real_T PulseGenerator_Duty;          /* Computed Parameter: PulseGenerator_Duty
                                        * Referenced by: '<Root>/Pulse Generator'
                                        */
  real_T PulseGenerator_PhaseDelay;    /* Expression: 0
                                        * Referenced by: '<Root>/Pulse Generator'
                                        */
};

/* Real-time Model Data Structure */
struct tag_RTM_simulinkmodeltoblinkL_T {
  const char_T * volatile errorStatus;
};

/* Block parameters (auto storage) */
extern P_simulinkmodeltoblinkLed_T simulinkmodeltoblinkLed_P;

/* Block states (auto storage) */
extern DW_simulinkmodeltoblinkLed_T simulinkmodeltoblinkLed_DW;

/* External outputs (root outports fed by signals with auto storage) */
extern ExtY_simulinkmodeltoblinkLed_T simulinkmodeltoblinkLed_Y;

/* Model entry point functions */
extern void simulinkmodeltoblinkLed_initialize(void);
extern void simulinkmodeltoblinkLed_step(void);
extern void simulinkmodeltoblinkLed_terminate(void);

/* Real-time Model object */
extern RT_MODEL_simulinkmodeltoblink_T *const simulinkmodeltoblinkLed_M;

/*-
 * The generated code includes comments that allow you to trace directly
 * back to the appropriate location in the model.  The basic format
 * is <system>/block_name, where system is the system number (uniquely
 * assigned by Simulink) and block_name is the name of the block.
 *
 * Use the MATLAB hilite_system command to trace the generated code back
 * to the model.  For example,
 *
 * hilite_system('<S3>')    - opens system 3
 * hilite_system('<S3>/Kp') - opens and selects block Kp which resides in S3
 *
 * Here is the system hierarchy for this model
 *
 * '<Root>' : 'simulinkmodeltoblinkLed'
 */
#endif                                 /* RTW_HEADER_simulinkmodeltoblinkLed_h_ */

/*
 * File trailer for generated code.
 *
 * [EOF]
 */
