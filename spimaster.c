/*
 * Copyright (c) 2018-2019, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
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
 *  ======== spimaster.c ========
 */
#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* POSIX Header files */
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

/* Driver Header files */
#include <ti/drivers/GPIO.h>
#include <ti/drivers/SPI.h>
#include <ti/display/Display.h>
#include <ti/drivers/I2C.h>

/* Driver configuration */
#include "ti_drivers_config.h"


/*   Beginning of Derek added   */

#define TASKSTACKSIZE       640

/* RTC Results Register */
#define DS3231_RESULT_REG          0x0000

/* I2C Slave Address */
#define DS3231_ADDR      0b1101000



/*
 * Data structure containing currently supported I2C TMP sensors.
 * Sensors are ordered by descending preference.
 */
static const struct {
    uint8_t address;
    uint8_t resultReg;
    char *id;
} sensors[1] = {
    { DS3231_ADDR, DS3231_RESULT_REG, "3213" }
};

static uint8_t slaveAddress;
static Display_Handle display;

static void i2cErrorHandler(I2C_Transaction *transaction,
    Display_Handle display);

/*  End of Derek added   */



#define THREADSTACKSIZE (1024)

#define SPI_MSG_LENGTH  (30)
#define MASTER_MSG      ("Reset clock to 0")  //derek

#define MAX_LOOP        (100)   //derek

static Display_Handle display;

unsigned char masterRxBuffer[SPI_MSG_LENGTH];
unsigned char masterTxBuffer[SPI_MSG_LENGTH];

/* Semaphore to block master until slave is ready for transfer */
sem_t masterSem;

/*
 *  ======== slaveReadyFxn ========
 *  Callback function for the GPIO interrupt on CONFIG_SPI_SLAVE_READY.
 */
void slaveReadyFxn(uint_least8_t index)
{
    sem_post(&masterSem);
}

/*
 *  ======== masterThread ========
 *  Master SPI sends a message to slave while simultaneously receiving a
 *  message from the slave.
 */
void *masterThread(void *arg0)
{
    SPI_Handle      masterSpi;
    SPI_Params      spiParams;
    SPI_Transaction transaction;
    uint32_t        i;
    bool            transferOK;
    int32_t         status;

    /*
     * CONFIG_SPI_MASTER_READY & CONFIG_SPI_SLAVE_READY are GPIO pins connected
     * between the master & slave.  These pins are used to synchronize
     * the master & slave applications via a small 'handshake'.  The pins
     * are later used to synchronize transfers & ensure the master will not
     * start a transfer until the slave is ready.  These pins behave
     * differently between spimaster & spislave examples:
     *
     * spimaster example:
     *     * CONFIG_SPI_MASTER_READY is configured as an output pin.  During the
     *       'handshake' this pin is changed from low to high output.  This
     *       notifies the slave the master is ready to run the application.
     *       Afterwards, the pin is used by the master to notify the slave it
     *       has opened CONFIG_SPI_MASTER.  When CONFIG_SPI_MASTER is opened, this
     *       pin will be pulled low.
     *
     *     * CONFIG_SPI_SLAVE_READY is configured as an input pin. During the
     *       'handshake' this pin is read & a high value will indicate the slave
     *       ready to run the application.  Afterwards, a falling edge interrupt
     *       will be configured on this pin.  When the slave is ready to perform
     *       a transfer, it will pull this pin low.
     *
     * Below we set CONFIG_SPI_MASTER_READY & CONFIG_SPI_SLAVE_READY initial
     * conditions for the 'handshake'.
     */
    GPIO_setConfig(CONFIG_SPI_MASTER_READY, GPIO_CFG_OUTPUT | GPIO_CFG_OUT_LOW);
    GPIO_setConfig(CONFIG_SPI_SLAVE_READY, GPIO_CFG_INPUT);

    /*
     * Handshake - Set CONFIG_SPI_MASTER_READY high to indicate master is ready
     * to run.  Wait CONFIG_SPI_SLAVE_READY to be high.
     */
    GPIO_write(CONFIG_SPI_MASTER_READY, 1);
    while (GPIO_read(CONFIG_SPI_SLAVE_READY) == 0) {}

    /* Handshake complete; now configure interrupt on CONFIG_SPI_SLAVE_READY */
    GPIO_setConfig(CONFIG_SPI_SLAVE_READY, GPIO_CFG_IN_PU | GPIO_CFG_IN_INT_FALLING);
    GPIO_setCallback(CONFIG_SPI_SLAVE_READY, slaveReadyFxn);
    GPIO_enableInt(CONFIG_SPI_SLAVE_READY);

    /*
     * Create synchronization semaphore; the master will wait on this semaphore
     * until the slave is ready.
     */
    status = sem_init(&masterSem, 0, 0);
    if (status != 0) {
        Display_printf(display, 0, 0, "Error creating masterSem\n");

        while(1);
    }

    /* Open SPI as master (default) */
    SPI_Params_init(&spiParams);
    spiParams.frameFormat = SPI_POL0_PHA1;
    spiParams.bitRate = 10000000;
    masterSpi = SPI_open(CONFIG_SPI_MASTER, &spiParams);
    if (masterSpi == NULL) {
        Display_printf(display, 0, 0, "Error initializing master SPI\n");
        while (1);
    }
    else {
        Display_printf(display, 0, 0, "Master SPI initialized\n");
    }

    /*
     * Master has opened CONFIG_SPI_MASTER; set CONFIG_SPI_MASTER_READY high to
     * inform the slave.
     */
    GPIO_write(CONFIG_SPI_MASTER_READY, 0);

    /* Copy message to transmit buffer */
    strncpy((char *) masterTxBuffer, MASTER_MSG, SPI_MSG_LENGTH);

    for (i = 0; i < MAX_LOOP; i++) {
        /*
         * Wait until slave is ready for transfer; slave will pull
         * CONFIG_SPI_SLAVE_READY low.
         */
        sem_wait(&masterSem);

        /* Initialize master SPI transaction structure */
        masterTxBuffer[sizeof(MASTER_MSG) - 1];   //derek
        memset((void *) masterRxBuffer, 0, SPI_MSG_LENGTH);
        transaction.count = SPI_MSG_LENGTH;
        transaction.txBuf = (void *) masterTxBuffer;
        transaction.rxBuf = (void *) masterRxBuffer;

        /* Toggle user LED, indicating a SPI transfer is in progress */
        //GPIO_toggle(CONFIG_GPIO_LED_1);      derek

        /* Perform SPI transfer */
        transferOK = SPI_transfer(masterSpi, &transaction);
        if (transferOK) {
            Display_printf(display, 0, 0, "Master received: %s", masterRxBuffer);
        }
        else {
            Display_printf(display, 0, 0, "Unsuccessful master SPI transfer");
        }

        /* Sleep for a bit before starting the next SPI transfer  */
        sleep(1);
    }

    SPI_close(masterSpi);

    /* Example complete - set pins to a known state */
    GPIO_disableInt(CONFIG_SPI_SLAVE_READY);
    GPIO_setConfig(CONFIG_SPI_SLAVE_READY, GPIO_CFG_OUTPUT | GPIO_CFG_OUT_LOW);
    GPIO_write(CONFIG_SPI_MASTER_READY, 0);

    Display_printf(display, 0, 0, "\nDone");

    return (NULL);
}

/*
 *  ======== mainThread ========
 */
void *mainThread(void *arg0)
{
    // i2c stuff

    uint16_t        sample;
    int16_t         clock_time;
    uint8_t         txBuffer[1];
    uint8_t         rxBuffer[2];
    int8_t          i;
    I2C_Handle      i2c;
    I2C_Params      i2cParams;
    I2C_Transaction i2cTransaction;

    //back to the normal stuff

    pthread_t           thread0;
    pthread_attr_t      attrs;
    struct sched_param  priParam;
    int                 retc;
    int                 detachState;

    /* Call driver init functions. */
    Display_init();
    GPIO_init();
    SPI_init();
    I2C_init();  //derek

    /* Configure the LED pins */
    GPIO_setConfig(CONFIG_GPIO_LED_0, GPIO_CFG_OUT_STD | GPIO_CFG_OUT_LOW);
    //GPIO_setConfig(CONFIG_GPIO_LED_1, GPIO_CFG_OUT_STD | GPIO_CFG_OUT_LOW);    derek

    /* Open the display for output */
    display = Display_open(Display_Type_UART, NULL);
    if (display == NULL) {
        /* Failed to open display driver */
        while (1);
    }

    /* Turn on user LED */
    GPIO_write(CONFIG_GPIO_LED_0, CONFIG_GPIO_LED_ON);

    Display_printf(display, 0, 0, "Starting the SPI master with I2C example");   //derek
    Display_printf(display, 0, 0, "This example requires external wires to be "
        "connected to the header pins. Please see the Board.html for details.\n");


    /* Create I2C for usage */
    I2C_Params_init(&i2cParams);
    i2cParams.bitRate = I2C_400kHz;
    i2c = I2C_open(0, &i2cParams);
    if (i2c == NULL) {
        Display_printf(display, 0, 0, "Error Initializing I2C\n");
        while (1);
    }
    else {
        Display_printf(display, 0, 0, "I2C Initialized!\n");
    }

    /* Common I2C transaction setup */
    i2cTransaction.writeBuf   = txBuffer;
    i2cTransaction.writeCount = 1;
    i2cTransaction.readBuf    = rxBuffer;
    i2cTransaction.readCount  = 0;

    /*
     * Determine which I2C sensors are present by querying known I2C
     * slave addresses.
     */
    i2cTransaction.slaveAddress = sensors[0].address;
    txBuffer[0] = sensors[0].resultReg;
    if (I2C_transfer(i2c, &i2cTransaction)) {
        slaveAddress = sensors[0].address;
        Display_printf(display, 0, 0, "Detected DS%s RTC with slave"
        " address 0x%x", sensors[0].id, sensors[0].address);
    } else {
        i2cErrorHandler(&i2cTransaction, display);
    }

    // If we never assigned a slave address
    if (slaveAddress == 0) {
        Display_printf(display, 0, 0, "Failed to detect a sensor!");
        I2C_close(i2c);
        while (1);
    }

    Display_printf(display, 0, 0, "\nUsing last known sensor for samples.");
    i2cTransaction.slaveAddress = slaveAddress;

    i2cTransaction.readCount  = 2;
    for (sample = 0; sample < 200; sample++) {
        if (I2C_transfer(i2c, &i2cTransaction)) {
            /*
             * Extract degrees C from the received data;
             * see TMP sensor datasheet
             */
            clock_time = (rxBuffer[0] << 8) | (rxBuffer[1]);

            /*
             * If the MSB is set '1', then we have a 2's complement
             * negative value which needs to be sign extended
             */
            if (rxBuffer[0] & 0x80) {
                clock_time |= 0xF000;
            }

            Display_printf(display, 0, 0, "Sample %u: %d (C)",
                sample, clock_time);
        }
        else {
            i2cErrorHandler(&i2cTransaction, display);
        }

        /* Sleep for 1 second */
        usleep(100000);
    }

    I2C_close(i2c);
    Display_printf(display, 0, 0, "I2C closed!");

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

    /* Create master thread */
    priParam.sched_priority = 1;
    pthread_attr_setschedparam(&attrs, &priParam);

    retc = pthread_create(&thread0, &attrs, masterThread, NULL);
    if (retc != 0) {
        /* pthread_create() failed */
        while (1);
    }

    return (NULL);
}

/*
 *  ======== i2cErrorHandler ========
 */
static void i2cErrorHandler(I2C_Transaction *transaction,
    Display_Handle display)
{
    switch (transaction->status) {
    case I2C_STATUS_TIMEOUT:
        Display_printf(display, 0, 0, "I2C transaction timed out!");
        break;
    case I2C_STATUS_CLOCK_TIMEOUT:
        Display_printf(display, 0, 0, "I2C serial clock line timed out!");
        break;
    case I2C_STATUS_ADDR_NACK:
        Display_printf(display, 0, 0, "I2C slave address 0x%x not"
            " acknowledged!", transaction->slaveAddress);
        break;
    case I2C_STATUS_DATA_NACK:
        Display_printf(display, 0, 0, "I2C data byte not acknowledged!");
        break;
    case I2C_STATUS_ARB_LOST:
        Display_printf(display, 0, 0, "I2C arbitration to another master!");
        break;
    case I2C_STATUS_INCOMPLETE:
        Display_printf(display, 0, 0, "I2C transaction returned before completion!");
        break;
    case I2C_STATUS_BUS_BUSY:
        Display_printf(display, 0, 0, "I2C bus is already in use!");
        break;
    case I2C_STATUS_CANCEL:
        Display_printf(display, 0, 0, "I2C transaction cancelled!");
        break;
    case I2C_STATUS_INVALID_TRANS:
        Display_printf(display, 0, 0, "I2C transaction invalid!");
        break;
    case I2C_STATUS_ERROR:
        Display_printf(display, 0, 0, "I2C generic error!");
        break;
    default:
        Display_printf(display, 0, 0, "I2C undefined error case!");
        break;
    }
}///*
// * Copyright (c) 2018-2019, Texas Instruments Incorporated
// * All rights reserved.
// *
// * Redistribution and use in source and binary forms, with or without
// * modification, are permitted provided that the following conditions
// * are met:
// *
// * *  Redistributions of source code must retain the above copyright
// *     notice, this list of conditions and the following disclaimer.
// *
// * *  Redistributions in binary form must reproduce the above copyright
// *     notice, this list of conditions and the following disclaimer in the
// *     documentation and/or other materials provided with the distribution.
// *
// * *  Neither the name of Texas Instruments Incorporated nor the names of
// *     its contributors may be used to endorse or promote products derived
// *     from this software without specific prior written permission.
// *
// * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
// * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
// * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
// * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
// * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
// * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// */
//
///*
// *  ======== spimaster.c ========
// */
//#include <stddef.h>
//#include <stdint.h>
//#include <string.h>
//
///* POSIX Header files */
//#include <pthread.h>
//#include <semaphore.h>
//#include <unistd.h>
//
///* Driver Header files */
//#include <ti/drivers/GPIO.h>
//#include <ti/drivers/SPI.h>
//#include <ti/display/Display.h>
//
///* Driver configuration */
//#include "ti_drivers_config.h"
//
//#define THREADSTACKSIZE (1024)
//
//#define SPI_MSG_LENGTH  (30)
//#define MASTER_MSG      ("Hello from master, msg#: ")
//
//#define MAX_LOOP        (1000)
//
//static Display_Handle display;
//
//unsigned char masterRxBuffer[SPI_MSG_LENGTH];
//unsigned char masterTxBuffer[SPI_MSG_LENGTH];
//
///* Semaphore to block master until slave is ready for transfer */
//sem_t masterSem;
//
///*
// *  ======== slaveReadyFxn ========
// *  Callback function for the GPIO interrupt on CONFIG_SPI_SLAVE_READY.
// */
//void slaveReadyFxn(uint_least8_t index)
//{
//    sem_post(&masterSem);
//}
//
///*
// *  ======== masterThread ========
// *  Master SPI sends a message to slave while simultaneously receiving a
// *  message from the slave.
// */
//void *masterThread(void *arg0)
//{
//    SPI_Handle      masterSpi;
//    SPI_Params      spiParams;
//    SPI_Transaction transaction;
//    uint32_t        i;
//    bool            transferOK;
//    int32_t         status;
//
//    /*
//     * CONFIG_SPI_MASTER_READY & CONFIG_SPI_SLAVE_READY are GPIO pins connected
//     * between the master & slave.  These pins are used to synchronize
//     * the master & slave applications via a small 'handshake'.  The pins
//     * are later used to synchronize transfers & ensure the master will not
//     * start a transfer until the slave is ready.  These pins behave
//     * differently between spimaster & spislave examples:
//     *
//     * spimaster example:
//     *     * CONFIG_SPI_MASTER_READY is configured as an output pin.  During the
//     *       'handshake' this pin is changed from low to high output.  This
//     *       notifies the slave the master is ready to run the application.
//     *       Afterwards, the pin is used by the master to notify the slave it
//     *       has opened CONFIG_SPI_MASTER.  When CONFIG_SPI_MASTER is opened, this
//     *       pin will be pulled low.
//     *
//     *     * CONFIG_SPI_SLAVE_READY is configured as an input pin. During the
//     *       'handshake' this pin is read & a high value will indicate the slave
//     *       ready to run the application.  Afterwards, a falling edge interrupt
//     *       will be configured on this pin.  When the slave is ready to perform
//     *       a transfer, it will pull this pin low.
//     *
//     * Below we set CONFIG_SPI_MASTER_READY & CONFIG_SPI_SLAVE_READY initial
//     * conditions for the 'handshake'.
//     */
//    GPIO_setConfig(CONFIG_SPI_MASTER_READY, GPIO_CFG_OUTPUT | GPIO_CFG_OUT_LOW);
//    GPIO_setConfig(CONFIG_SPI_SLAVE_READY, GPIO_CFG_INPUT);
//
//    /*
//     * Handshake - Set CONFIG_SPI_MASTER_READY high to indicate master is ready
//     * to run.  Wait CONFIG_SPI_SLAVE_READY to be high.
//     */
//    GPIO_write(CONFIG_SPI_MASTER_READY, 1);
//    while (GPIO_read(CONFIG_SPI_SLAVE_READY) == 0) {}
//
//    /* Handshake complete; now configure interrupt on CONFIG_SPI_SLAVE_READY */
//    GPIO_setConfig(CONFIG_SPI_SLAVE_READY, GPIO_CFG_IN_PU | GPIO_CFG_IN_INT_FALLING);
//    GPIO_setCallback(CONFIG_SPI_SLAVE_READY, slaveReadyFxn);
//    GPIO_enableInt(CONFIG_SPI_SLAVE_READY);
//
//    /*
//     * Create synchronization semaphore; the master will wait on this semaphore
//     * until the slave is ready.
//     */
//    status = sem_init(&masterSem, 0, 0);
//    if (status != 0) {
//        Display_printf(display, 0, 0, "Error creating masterSem\n");
//
//        while(1);
//    }
//
//    /* Open SPI as master (default) */
//    SPI_Params_init(&spiParams);
//    spiParams.frameFormat = SPI_POL0_PHA1;
//    spiParams.bitRate = 10000000;
//    masterSpi = SPI_open(CONFIG_SPI_MASTER, &spiParams);
//    if (masterSpi == NULL) {
//        Display_printf(display, 0, 0, "Error initializing master SPI\n");
//        while (1);
//    }
//    else {
//        Display_printf(display, 0, 0, "Master SPI initialized\n");
//    }
//
//    /*
//     * Master has opened CONFIG_SPI_MASTER; set CONFIG_SPI_MASTER_READY high to
//     * inform the slave.
//     */
//    GPIO_write(CONFIG_SPI_MASTER_READY, 0);
//
//    /* Copy message to transmit buffer */
//    strncpy((char *) masterTxBuffer, MASTER_MSG, SPI_MSG_LENGTH);
//
//    for (i = 0; i < MAX_LOOP; i++) {
//        /*
//         * Wait until slave is ready for transfer; slave will pull
//         * CONFIG_SPI_SLAVE_READY low.
//         */
//        sem_wait(&masterSem);
//
//        /* Initialize master SPI transaction structure */
//        masterTxBuffer[sizeof(MASTER_MSG) - 1] = (i % 10) + '0';
//        memset((void *) masterRxBuffer, 0, SPI_MSG_LENGTH);
//        transaction.count = SPI_MSG_LENGTH;
//        transaction.txBuf = (void *) masterTxBuffer;
//        transaction.rxBuf = (void *) masterRxBuffer;
//
//        /* Toggle user LED, indicating a SPI transfer is in progress */
//        GPIO_toggle(CONFIG_GPIO_LED_1);
//
//        /* Perform SPI transfer */
//        transferOK = SPI_transfer(masterSpi, &transaction);
//        if (transferOK) {
//            Display_printf(display, 0, 0, "Master received: %s", masterRxBuffer);
//        }
//        else {
//            Display_printf(display, 0, 0, "Unsuccessful master SPI transfer");
//        }
//
//        /* Sleep for a bit before starting the next SPI transfer  */
//        sleep(3);
//    }
//
//    SPI_close(masterSpi);
//
//    /* Example complete - set pins to a known state */
//    GPIO_disableInt(CONFIG_SPI_SLAVE_READY);
//    GPIO_setConfig(CONFIG_SPI_SLAVE_READY, GPIO_CFG_OUTPUT | GPIO_CFG_OUT_LOW);
//    GPIO_write(CONFIG_SPI_MASTER_READY, 0);
//
//    Display_printf(display, 0, 0, "\nDone");
//
//    return (NULL);
//}
//
///*
// *  ======== mainThread ========
// */
//void *mainThread(void *arg0)
//{
//    pthread_t           thread0;
//    pthread_attr_t      attrs;
//    struct sched_param  priParam;
//    int                 retc;
//    int                 detachState;
//
//    /* Call driver init functions. */
//    Display_init();
//    GPIO_init();
//    SPI_init();
//
//    /* Configure the LED pins */
//    GPIO_setConfig(CONFIG_GPIO_LED_0, GPIO_CFG_OUT_STD | GPIO_CFG_OUT_LOW);
//    GPIO_setConfig(CONFIG_GPIO_LED_1, GPIO_CFG_OUT_STD | GPIO_CFG_OUT_LOW);
//
//    /* Open the display for output */
//    display = Display_open(Display_Type_UART, NULL);
//    if (display == NULL) {
//        /* Failed to open display driver */
//        while (1);
//    }
//
//    /* Turn on user LED */
//    GPIO_write(CONFIG_GPIO_LED_0, CONFIG_GPIO_LED_ON);
//
//    Display_printf(display, 0, 0, "Starting the SPI master example");
//    Display_printf(display, 0, 0, "This example requires external wires to be "
//        "connected to the header pins. Please see the Board.html for details.\n");
//
//    /* Create application threads */
//    pthread_attr_init(&attrs);
//
//    detachState = PTHREAD_CREATE_DETACHED;
//    /* Set priority and stack size attributes */
//    retc = pthread_attr_setdetachstate(&attrs, detachState);
//    if (retc != 0) {
//        /* pthread_attr_setdetachstate() failed */
//        while (1);
//    }
//
//    retc |= pthread_attr_setstacksize(&attrs, THREADSTACKSIZE);
//    if (retc != 0) {
//        /* pthread_attr_setstacksize() failed */
//        while (1);
//    }
//
//    /* Create master thread */
//    priParam.sched_priority = 1;
//    pthread_attr_setschedparam(&attrs, &priParam);
//
//    retc = pthread_create(&thread0, &attrs, masterThread, NULL);
//    if (retc != 0) {
//        /* pthread_create() failed */
//        while (1);
//    }
//
//    return (NULL);
//}
