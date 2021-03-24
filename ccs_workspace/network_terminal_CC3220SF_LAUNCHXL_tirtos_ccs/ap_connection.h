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
#include "queue.h"

#define ASSERT_AND_CLEAN_CONNECT_NO_FREE(ret, errortype, ConnectParams)\
        {\
            if(ret < 0)\
            {\
                SHOW_WARNING(ret, errortype);\
                return -1;\
            }\
        }

typedef struct
{
    uint16_t frameControl;
    uint16_t duration;
    uint8_t destAddr[6];
    uint8_t sourceAddr[6];
    uint8_t bssid[6];
    uint16_t seqCtrl;
    uint32_t timestamp;
    uint16_t beaconInterval;
    uint32_t beaconIntervalMs;
    uint16_t capabilityInfo;
    uint8_t ssidElemId;
    uint8_t ssidLen;
    uint8_t ssid[33];
}frameInfo_t;

int32_t connectToAP();

uint16_t get_port_for_data_tx();

int32_t transmit_data_forever_test(uint16_t sockPort);

int32_t time_drift_test(uint16_t sockPort);

int32_t time_drift_test_l3(uint16_t sockPort);

int32_t time_drift_test_l2();

int32_t parse_beacon_frame(uint8_t * Rx_frame, frameInfo_t * frameInfo, uint8_t printInfo);

int32_t tx_accelerometer(uint16_t sockPort);

int32_t q_to_string(queue_t * q, uint8_t * buf);

int32_t test_time_beac_sync();

int32_t enter_tranceiver_mode(int32_t initialize);

#endif /* AP_CONNECTION_H_ */
