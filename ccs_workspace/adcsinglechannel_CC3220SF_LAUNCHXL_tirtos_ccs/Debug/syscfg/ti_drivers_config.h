/*
 *  ======== ti_drivers_config.h ========
 *  Configured TI-Drivers module declarations
 *
 *  The macros defines herein are intended for use by applications which
 *  directly include this header. These macros should NOT be hard coded or
 *  copied into library source code.
 *
 *  Symbols declared as const are intended for use with libraries.
 *  Library source code must extern the correct symbol--which is resolved
 *  when the application is linked.
 *
 *  DO NOT EDIT - This file is generated for the CC3220SF_LAUNCHXL
 *  by the SysConfig tool.
 */
#ifndef ti_drivers_config_h
#define ti_drivers_config_h

#define CONFIG_SYSCONFIG_PREVIEW

#define CONFIG_CC3220SF_LAUNCHXL
#ifndef DeviceFamily_CC3220
#define DeviceFamily_CC3220
#endif

#include <ti/devices/DeviceFamily.h>

#include <stdint.h>

/* support C++ sources */
#ifdef __cplusplus
extern "C" {
#endif


/*
 *  ======== ADC ========
 */

/* P59, Analog Input 3 */
extern const uint_least8_t              CONFIG_ADC_0_CONST;
#define CONFIG_ADC_0                    0
/* P60, Analog Input 4 */
extern const uint_least8_t              CONFIG_ADC_1_CONST;
#define CONFIG_ADC_1                    1
/* P58, Analog Input 2 */
extern const uint_least8_t              CONFIG_ADC_2_CONST;
#define CONFIG_ADC_2                    2
#define CONFIG_TI_DRIVERS_ADC_COUNT     3


/*
 *  ======== Timer ========
 */

extern const uint_least8_t                  CONFIG_TIMER_0_CONST;
#define CONFIG_TIMER_0                      0
#define CONFIG_TI_DRIVERS_TIMER_COUNT       1

/*
 *  ======== UART ========
 */

/*
 *  TX: P55
 *  RX: P57
 *  XDS110 UART
 */
extern const uint_least8_t              CONFIG_UART_0_CONST;
#define CONFIG_UART_0                   0
#define CONFIG_TI_DRIVERS_UART_COUNT    1


/*
 *  ======== Board_init ========
 *  Perform all required TI-Drivers initialization
 *
 *  This function should be called once at a point before any use of
 *  TI-Drivers.
 */
extern void Board_init(void);

/*
 *  ======== Board_initGeneral ========
 *  (deprecated)
 *
 *  Board_initGeneral() is defined purely for backward compatibility.
 *
 *  All new code should use Board_init() to do any required TI-Drivers
 *  initialization _and_ use <Driver>_init() for only where specific drivers
 *  are explicitly referenced by the application.  <Driver>_init() functions
 *  are idempotent.
 */
#define Board_initGeneral Board_init

#ifdef __cplusplus
}
#endif

#endif /* include guard */
