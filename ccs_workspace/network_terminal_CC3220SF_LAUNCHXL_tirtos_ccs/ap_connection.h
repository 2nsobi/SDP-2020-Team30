/*
 * ap_connection.h
 *
 *  Created on: Nov 9, 2020
 *      Author: NNobi
 */

#ifndef AP_CONNECTION_H_
#define AP_CONNECTION_H_

#include <stdint.h>

#include "network_terminal.h"

#define ASSERT_AND_CLEAN_CONNECT_NO_FREE(ret, errortype, ConnectParams)\
        {\
            if(ret < 0)\
            {\
                SHOW_WARNING(ret, errortype);\
                return -1;\
            }\
        }

int32_t connectToAP();

uint16_t get_port_for_data_tx();

int32_t transmit_data_forever_test(uint16_t sockPort);

int32_t time_drift_test(uint16_t sockPort);

#endif /* AP_CONNECTION_H_ */
