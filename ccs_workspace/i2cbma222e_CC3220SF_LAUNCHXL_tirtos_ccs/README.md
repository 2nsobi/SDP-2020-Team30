## Example Summary

Sample application to read from BMA222E accelerometer, The example reads out
accelerometer information(x,y,z) and temperature information from the BMA222E
sensor.

## Peripherals & Pin Assignments

SysConfig generates the driver configurations into the __ti_drivers_config.c__
and __ti_drivers_config.h__ files. Information on pins and resources used
is present in both generated files. The SysConfig user interface can also be
utilized to determine pins and resources used.

* `CONFIG_I2C_BMA222E` - I2C peripheral instance used to communicate with BMA222E Sensor

## BoosterPacks, Board Resources & Jumper Settings

> If you're using an IDE (such as CCS or IAR), please refer to Board.html in your project
directory for resources used and board-specific jumper settings. Otherwise, you can find
Board.html in the directory &lt;PLUGIN_INSTALL_DIR&gt;/source/ti/boards/&lt;BOARD&gt;.

## Example Usage

* Open a serial session (e.g. [`PuTTY`](http://www.putty.org/ "PuTTY's Homepage"),teraterm, CCS terminal, etc.) to the appropriate COM port.
    * The COM port can be determined via Device Manager in Windows or via `ls /dev/tty*` in Linux.

The connection will have the following settings:
```
    Baud-rate:     115200
    Data bits:          8
    Stop bits:          1
    Parity:          None
    Flow Control:    None
```

* Run the example.

* The example collects the samples for accelerometer data and temperature from
  BMA222E driver and displays it on Serial Console as shown below.
```

    Starting the i2cbma222e accelerometer example...

    I2C Initialized!

    accelo: x = -256, y = -128, z = 4160, temp = 24.5000

    accelo: x = -256, y = -128, z = 4160, temp = 24.0000

    accelo: x = -256, y = -128, z = 4160, temp = 24.0000

```

## Application Design Details

This application uses one thread:

`mainThread` - performs the following actions:

1. Opens and initializes an I2C Driver.

2. Initialize the BMA222E sensor(the initialization is done using the readout template API).

3. Periodically read the Temperature and accelerometer readings from the BMA222E sensor.
