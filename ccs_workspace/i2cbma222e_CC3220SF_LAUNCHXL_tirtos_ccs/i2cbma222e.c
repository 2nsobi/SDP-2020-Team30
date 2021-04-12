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
 *    ======== i2cbma222e.c ========
 */
#include <stdint.h>
#include <stddef.h>

/* Math library */
#include "math.h"

/* POSIX Header files */
#include <unistd.h>

/* Driver Header files */
#include <ti/drivers/GPIO.h>
#include <ti/drivers/I2C.h>
#include <ti/display/Display.h>

/* Module Header */
#include <ti/sail/bma2x2/bma2x2.h>

/* Driver configuration */
#include "ti_drivers_config.h"

/*The temperature value read from the BMA222E is 8 bits and it is in 2s
 * complement form The slope of the temeprature is .5K/LSB and the center
 * temperature is 23 degree celsius Effectively the sensor can measure from
 * -41C(23-(128/2)) to +86.5C(23 +(127/2)
 */
#define CENTER_TEMPERATURE 23

extern s32 bma2x2_data_readout_template(I2C_Handle i2cHndl);

/*
 *  ======== mainThread ========
 */
void *mainThread(void *arg0)
{
    I2C_Handle      i2c;
    I2C_Params      i2cParams;
    /* structure to read the accelerometer data*/
    /* the resolution is in 8 bit*/
    struct bma2x2_accel_data_temp sample_xyzt;
    Display_Handle display;

    /* Call driver init functions */
    /*In this particular example the interrupt feature of the bma222e sensor
     * has not been used. If it needs to be used then we will need the GPIO
     * module to recieve the interrupts.So the GPIO module initialization has
     * been done here. But in this particular case it is not being used.
     */
    GPIO_init();
    I2C_init();

    /* Open the HOST display for output */
    display = Display_open(Display_Type_UART, NULL);
    if (display == NULL) 
    {
        while (1);
    }
    Display_print0(display, 0, 0, 
                   "Starting the i2cbma222e accelerometer example...\n\n");

    I2C_Params_init(&i2cParams);
    i2cParams.bitRate = I2C_400kHz;
    i2cParams.transferMode = I2C_MODE_BLOCKING;
    i2cParams.transferCallbackFxn = NULL;
    i2c = I2C_open(CONFIG_I2C_BMA222E, &i2cParams);
    if (i2c == NULL) 
    {
        Display_print0(display, 0, 0, "Error Initializing I2C\n");
    }
    else 
    {
        Display_print0(display, 0, 0, "I2C Initialized!\n");
    }

    /* This is actually a data readout template function provided by the bosch
     * opensource support file(we are using this as is). Modification has been
     * done to the API signature the i2c handle as the parameter,  also the API
     * has been modified and the setting the device to power down mode after
     * the data readout is removed (we are using this as initiailization
     * function) This API does the initialization job for us hence we are using
     * it without any modificaitons. User should be able to modify the
     * bma2x2_support.c file as per his need. */
    if(BMA2x2_INIT_VALUE != bma2x2_data_readout_template(i2c))
    {
        Display_print0(display, 0, 0, "Error Initializing bma222e\n");
    }


    while(1)
    {
        /*reads the accelerometer data in 8 bit resolution*/
        /*There are different API for reading out the accelerometer data in
         * 10,14,12 bit resolution*/
        if(BMA2x2_INIT_VALUE == bma2x2_read_accel_xyzt(&sample_xyzt))
        {
            Display_print4(display, 0, 0, "accelo: x = %d, y = %d, z = %d," 
                    " length = %f\n",sample_xyzt.x,sample_xyzt.y,sample_xyzt.z,
                    (float)(sqrt(pow(sample_xyzt.x, 2) + pow(sample_xyzt.y, 2) + pow(sample_xyzt.z, 2))));
        }
        else
        {
            Display_print0(display, 0, 0, 
                    "Error reading from the accelerometer\n");
        }
        sleep(1);
    }
}
