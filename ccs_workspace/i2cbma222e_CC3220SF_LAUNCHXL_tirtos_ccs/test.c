/*
 * test.c
 *
 *  Created on: Nov 12, 2020
 *      Author: NNobi
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

int32_t init_accel()
{
//    I2C_Handle      i2c;
//    I2C_Params      i2cParams;
//
//    /* Call driver init functions */
//    /*In this particular example the interrupt feature of the bma222e sensor
//     * has not been used. If it needs to be used then we will need the GPIO
//     * module to recieve the interrupts.So the GPIO module initialization has
//     * been done here. But in this particular case it is not being used.
//     */
//    GPIO_init();
//    I2C_init();
//
//    I2C_Params_init(&i2cParams);
//    i2cParams.bitRate = I2C_400kHz;
//    i2cParams.transferMode = I2C_MODE_BLOCKING;
//    i2cParams.transferCallbackFxn = NULL;
//    i2c = I2C_open(CONFIG_I2C_BMA222E, &i2cParams);
//
//    /* This is actually a data readout template function provided by the bosch
//     * opensource support file(we are using this as is). Modification has been
//     * done to the API signature the i2c handle as the parameter,  also the API
//     * has been modified and the setting the device to power down mode after
//     * the data readout is removed (we are using this as initiailization
//     * function) This API does the initialization job for us hence we are using
//     * it without any modificaitons. User should be able to modify the
//     * bma2x2_support.c file as per his need. */
//    bma2x2_data_readout_template(i2c);
    return 0;
}


int32_t get_accel()
{
//    I2C_Handle      i2c;
//    I2C_Params      i2cParams;
//    /* structure to read the accelerometer data*/
//    /* the resolution is in 8 bit*/
//    struct bma2x2_accel_data_temp sample_xyzt;
    return 0;
}

