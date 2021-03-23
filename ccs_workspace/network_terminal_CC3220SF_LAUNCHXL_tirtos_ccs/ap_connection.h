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

/* Application defines */
#define WLAN_EVENT_TOUT             (10000)
#define TIMEOUT_SEM                 (-1)
#define TCP_PROTOCOL_FLAGS          0
#define BUF_LEN                     (MAX_BUF_SIZE - 20)

/* custom defines */
#define AP_SSID                     "Jonah_AP"
#define AP_KEY                      "1234567890"
#define ENTRY_PORT                  10000
#define BILLION                     1000000000
#define MESSAGE_SIZE                50
#define NUM_READINGS                50
#define MAX_RX_PACKET_SIZE          1544
#define MAX_TX_PACKET_SIZE          30000

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

int32_t ts_to_string(uint32_t timestamps[][NUM_READINGS], uint32_t current_ts_index, uint8_t * buf);

int32_t test_time_beac_sync();

_i16 enter_tranceiver_mode(int32_t first_time, uint32_t channel);

#endif /* AP_CONNECTION_H_ */
