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

/* Application defines */
#define WLAN_EVENT_TOUT             (10000)
#define TIMEOUT_SEM                 (-1)
#define TCP_PROTOCOL_FLAGS          0
#define BUF_LEN                     (MAX_BUF_SIZE - 20)

/* custom defines */
#define AP_SSID                     "true_base_ap"
#define AP_KEY                      "sdp-2021"
#define ENTRY_PORT                  10000
#define BILLION                     1000000000
#define MESSAGE_SIZE                50
#define NUM_READINGS                900
#define MAX_RX_PACKET_SIZE          1544
#define MAX_TX_PACKET_SIZE          100000
#define BEACON_TIME_TRIGGER         2000    // in ms
#define START_CHANNEL               1
//#define MAC_FILTER_ARGS           " -f S_MAC -v 58:FB:84:5D:70:05 -e not_equals -a drop -m L1"
//#define MAC_FILTER_ARGS           " -f S_MAC -v 58:00:e3:43:6b:63 -e not_equals -a drop -m L1"
//#define MAC_FILTER_ARGS           " -f S_MAC -v 6a:00:e3:43:6b:63 -e not_equals -a drop -m L1"
//#define MAC_FILTER_ARGS           " -f S_MAC -v BE:91:80:53:29:14 -e not_equals -a drop -m L1"
#define MAC_FILTER_ARGS             " -f S_MAC -v 6C:B0:CE:85:7B:D8 -e not_equals -a drop -m L1"
#define CHANNEL_TIMEOUT             1       //in sec
#define SUCCESSFUL_BEACONS_THRESH   30

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
    uint32_t beaconIntervalUs;
    uint16_t capabilityInfo;
    uint8_t ssidElemId;
    uint8_t ssidLen;
    uint8_t ssid[33];
}frameInfo_t;

int32_t connectToAP(int32_t max_retries);

void load_cell_test(ADC_Handle *adc0);

int32_t time_beacons_and_load_cell(ADC_Handle *adc0);

int32_t time_beacons_and_accelerometer(ADC_Handle *adc0, ADC_Handle *adc1, ADC_Handle *adc2);

int32_t accel_to_string(uint32_t timestamps[][NUM_READINGS], float accel_readings[NUM_READINGS], uint32_t current_ts_index, uint8_t * buf);

_u32 parse_beacon_frame(uint8_t * Rx_frame, frameInfo_t * frameInfo, uint8_t printInfo);

int32_t test_time_beac_sync();

_i16 enter_tranceiver_mode(uint32_t channel, int32_t first_time, int32_t enable_filters);

#endif /* AP_CONNECTION_H_ */
