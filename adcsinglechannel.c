/*
 * Copyright (c) 2016-2020, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 *  ======== adcsinglechannel.c ========
 */
#include <stdint.h>
#include <stddef.h>
#include <unistd.h>


//Jonah added start
#include <ti/drivers/Timer.h>
#include "ti_drivers_config.h"
#include <ti/devices/cc32xx/inc/hw_memmap.h>
//Jonah added end

/* POSIX Header files */
#include <pthread.h>

/* Driver Header files */
#include <ti/drivers/ADC.h>
#include <ti/display/Display.h>
#include <ti/drivers/GPIO.h>

/* Driver configuration */
#include "ti_drivers_config.h"

/* ADC sample count */
#define ADC_SAMPLE_COUNT  (10)

#define THREADSTACKSIZE   (768)

/* ADC conversion result variables */
uint16_t adcValue0;
uint16_t adcValue1;  //derek
uint16_t adcValue2;  //derek
uint32_t adcValue0MicroVolt;
uint32_t adcValue1MicroVolt;  //derek
uint32_t adcValue2MicroVolt;  //derek
//uint16_t adcValue1[ADC_SAMPLE_COUNT];
//uint32_t adcValue1MicroVolt[ADC_SAMPLE_COUNT];

static Display_Handle display;

/*
 *  ======== threadFxn0 ========
 *  Open an ADC instance and get a sampling result from a one-shot conversion.
 */
void *threadFxn0(void *arg0)
{
    ADC_Handle   adc0;
    ADC_Handle   adc1;
    ADC_Handle   adc2;
    ADC_Params   params;
    int_fast16_t res0;
    int_fast16_t res1;
    int_fast16_t res2;


    //Jonah added variables
    time_t time_new, time_old;

    ADC_Params_init(&params);
    adc0 = ADC_open(CONFIG_ADC_0, &params);  //derek
    adc1 = ADC_open(CONFIG_ADC_1, &params);  //derek
    adc2 = ADC_open(CONFIG_ADC_2, &params);  //derek

    if (adc0 == NULL) {
        Display_printf(display, 0, 0, "Error initializing CONFIG_ADC_0\n");
        while (1);
    }

    /////////////////////////////////Derek/////////////////////////////////////
    if (adc1 == NULL) {
            Display_printf(display, 0, 0, "Error initializing CONFIG_ADC_1\n");
            while (1);
        }
    if (adc0 == NULL) {
            Display_printf(display, 0, 0, "Error initializing CONFIG_ADC_2\n");
            while (1);
        }

    ////////////////////////////////////////////////////////////////////////////
    //JONAH ADDED START
    Timer_init();
    Timer_Handle    handle;
    Timer_Params    timer_params;
    Timer_Params_init(&timer_params);
    timer_params.periodUnits = Timer_PERIOD_US;
    timer_params.period = 10000000;
    timer_params.timerMode  = Timer_FREE_RUNNING;
    handle = Timer_open(CONFIG_TIMER_0, &timer_params);
    if(handle == NULL){
        Display_printf(display, 0, 0, "Error opening timer handle");
        while(1){

        }
    }
    int32_t status;
    status = Timer_start(handle);

    if (status == Timer_STATUS_ERROR) {
        Display_printf(display, 0, 0, "Error starting timer");
        //Timer_start() failed
        while (1);
    }

    uint32_t timer_clock;
    //ADCTimerEnable(ADC_BASE);
    //unsigned long timer_value;
    int i = 10;
    while(1){
        usleep(800000);

        res0 = ADC_convert(adc0, &adcValue0);
        res1 = ADC_convert(adc1, &adcValue1);  //derek
        res2 = ADC_convert(adc2, &adcValue2);  //derek
        //time_old = time_new;S
        //time_new = clock();

        if (res0 == ADC_STATUS_SUCCESS || res1 == ADC_STATUS_SUCCESS || res2 == ADC_STATUS_SUCCESS) {

            adcValue0MicroVolt = ADC_convertRawToMicroVolts(adc0, adcValue0);
            adcValue1MicroVolt = ADC_convertRawToMicroVolts(adc1, adcValue1);
            adcValue2MicroVolt = ADC_convertRawToMicroVolts(adc2, adcValue2);

            timer_clock = Timer_getCount(handle);

            Display_printf(display, 0, 0, "Difference %d, CONFIG_ADC_0 %d uV, CONFIG_ADC_1 %d uV, clock = %u\n",
                           adcValue1MicroVolt-adcValue0MicroVolt,adcValue0MicroVolt, adcValue1MicroVolt, (unsigned int)(timer_clock));

        }
        else {
            Display_printf(display, 0, 0, "CONFIG_ADC_0 convert failed\n");
        }
        if(i == 0){
            break;
        }
    }
    //JONAH ADDED END

    /* Blocking mode conversion */
    //
    //res = 0;//ADC_convert(adc, &adcValue0);

    //if (res == ADC_STATUS_SUCCESS) {

        //adcValue0MicroVolt = ADC_convertRawToMicroVolts(adc, adcValue0);

        //Display_printf(display, 0, 0, "CONFIG_ADC_0 raw result: %d\n", adcValue0);
        //Display_printf(display, 0, 0, "CONFIG_ADC_0 convert result: %d uV\n",
            //adcValue0MicroVolt);
    //}
    //else {
        //Display_printf(display, 0, 0, "CONFIG_ADC_0 convert failed\n");
    //}

    ADC_close(adc0);
    ADC_close(adc1);
    ADC_close(adc2);

    return (NULL);
}

 //  ======== mainThread ========

void *mainThread(void *arg0)
{
    pthread_t           thread0, thread1;
    pthread_attr_t      attrs;
    struct sched_param  priParam;
    int                 retc;
    int                 detachState;

    /* Call driver init functions */
    ADC_init();
    Display_init();

    /* Open the display for output */
    display = Display_open(Display_Type_UART, NULL);
    if (display == NULL) {
        /* Failed to open display driver */
        while (1);
    }

    Display_printf(display, 0, 0, "Starting the acdsinglechannel example\n");

    /* Create application threads */
    pthread_attr_init(&attrs);

    detachState = PTHREAD_CREATE_DETACHED;
    /* Set priority and stack size attributes */
    retc = pthread_attr_setdetachstate(&attrs, detachState);
    if (retc != 0) {
        /* pthread_attr_setdetachstate() failed */
        while (1);
    }

    retc |= pthread_attr_setstacksize(&attrs, THREADSTACKSIZE);
    if (retc != 0) {
        /* pthread_attr_setstacksize() failed */
        while (1);
    }

    /* Create threadFxn0 thread */
    priParam.sched_priority = 1;
    pthread_attr_setschedparam(&attrs, &priParam);

    retc = pthread_create(&thread0, &attrs, threadFxn0, NULL);
    if (retc != 0) {
        /* pthread_create() failed */
        while (1);
    }


    /* Create threadFxn1 thread */
    // JONAH ADDED COMMENT OUT retc = pthread_create(&thread1, &attrs, threadFxn1, (void* )0);


    return (NULL);
}
