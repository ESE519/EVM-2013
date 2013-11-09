/*
 * File: ert_main.c
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

#include "simulinkmodeltoblinkLed.h"
#include "rtwtypes.h"
//#include "arm_cortex_m_wrapper.h"
#include <include.h>
#include <ulib.h>
#include <stdio.h>
#include <string.h>

volatile int IsrOverrun = 0;
static boolean_T OverrunFlag = 0;
void rt_OneStep(void)
{
  /* Check for overrun. Protect OverrunFlag against preemption. */
  if (OverrunFlag++) {
    IsrOverrun = 1;
    OverrunFlag--;
    return;
  }

  /* No interrupts to enable */
  ;
  simulinkmodeltoblinkLed_step();

  /* Get model outputs here */

  /* No interrupts to disable */
  ;
  OverrunFlag--;
}

/* Coder Target main loop
 */
#define UNUSED(x)                      x = x

int main(void)
{
	nrk_setup_ports();

  
	volatile boolean_T noErr;
  float modelBaseRate = 0.5;
  float systemClock = 0;
  SystemCoreClockUpdate();
  UNUSED(modelBaseRate);
  UNUSED(systemClock);
  rtmSetErrorStatus(simulinkmodeltoblinkLed_M, 0);

	simulinkmodeltoblinkLed_initialize();



  /* No scheduler to configure */
  ;
  noErr =
    rtmGetErrorStatus(simulinkmodeltoblinkLed_M) == (NULL);

  /* No interrupts to enable */
  ;
  ;
  while (noErr ) {
    rt_OneStep();
    noErr =
      rtmGetErrorStatus(simulinkmodeltoblinkLed_M) == (NULL);
  }

  /* Disable rt_OneStep() here */

  /* Terminate model */
  simulinkmodeltoblinkLed_terminate();
  ;
  nrk_led_set (RED_LED);
	return 0;
}

/*
 * File trailer for generated code.
 *
 * [EOF]
 */
