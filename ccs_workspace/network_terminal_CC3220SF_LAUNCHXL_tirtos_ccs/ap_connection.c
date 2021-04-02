/*
 * ap_connection.c
 *
 *  Created on: Nov 9, 2020
 *      Author: NNobi
 */

/* Standard includes */
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

/* Example Header files */
#include "network_terminal.h"
#include "cmd_parser.h"
//#include "socket_cmd.h"
#include "radio_tool.h"
#include "wlan_cmd.h"

/* custom header files */
#include "ap_connection.h"



//#define MAC_FILTER_ARGS           " -f S_MAC -v 58:FB:84:5D:70:05 -e not_equals -a drop -m L1"
//#define MAC_FILTER_ARGS           " -f S_MAC -v 58:00:e3:43:6b:63 -e not_equals -a drop -m L1"
#define MAC_FILTER_ARGS           " -f S_MAC -v 6a:00:e3:43:6b:63 -e not_equals -a drop -m L1"
//#define MAC_FILTER_ARGS           " -f S_MAC -v BE:91:80:53:29:14 -e not_equals -a drop -m L1"



typedef union
{
    SlSockAddrIn6_t in6;       /* Socket info for Ipv6 */
    SlSockAddrIn_t in4;        /* Socket info for Ipv4 */
}sockAddr_t;

uint8_t Tx_data[MAX_TX_PACKET_SIZE];
uint32_t timestamps[2][NUM_READINGS];
float adc_readings[NUM_READINGS];
uint32_t hw_timestamps[1];

int32_t connectToAP()
{

    int32_t ret = 0;
    ConnectCmd_t ConnectParams;

    memset(&ConnectParams, 0x0, sizeof(ConnectCmd_t));

    uint8_t ssid[] = AP_SSID;
    uint8_t key[] = AP_KEY;

//    SlWlanSecParams_t secParams = { .Type = SL_WLAN_SEC_TYPE_OPEN,
//                                        .Key = (signed char *)key,
//                                        .KeyLen = strlen((const char *)key) };

    SlWlanSecParams_t secParams = { .Type = SL_WLAN_SEC_TYPE_WPA,
                                        .Key = (signed char *)key,
                                        .KeyLen = strlen((const char *)key) };

//    SlWlanSecParams_t secParams = { .Type = SL_WLAN_SEC_TYPE_WPA2_PLUS,
//                                        .Key = (signed char *)key,
//                                        .KeyLen = strlen((const char *)key) };



//    SlWlanSecParams_t secParams = { .Type = SL_WLAN_SEC_TYPE_WPA_WPA2,
//                                    .Key = (signed char *)key,
//                                    .KeyLen = strlen((const char *)key) };

    ConnectParams.ssid = ssid;
    ConnectParams.secParams = secParams;

    /*
     *  Check to see if the NWP is in STA role,
     *  since it has to be in STA role in order to connect to an AP.
     *  If it isn't - set role and reset the NWP.
     */
    if(app_CB.Role != ROLE_STA)
    {
        ret = sl_WlanSetMode(ROLE_STA);
        ASSERT_AND_CLEAN_CONNECT_NO_FREE(ret, WLAN_ERROR, &ConnectParams);

        ret = sl_Stop(SL_STOP_TIMEOUT);
        ASSERT_AND_CLEAN_CONNECT_NO_FREE(ret, DEVICE_ERROR, &ConnectParams);

        ret = sl_Start(0, 0, 0);
        ASSERT_AND_CLEAN_CONNECT_NO_FREE(ret, DEVICE_ERROR, &ConnectParams);

        /* Here we are in STA mode */
        app_CB.Role = ret;
    }

    uint32_t connected_to_ap = 0;

    while(!connected_to_ap)
    {
        /* Connect to AP */
        ret =
            sl_WlanConnect((const signed char *)(ConnectParams.ssid),
                           strlen(
                               (const char *)(ConnectParams.ssid)), 0,
                           &ConnectParams.secParams, 0);
        ASSERT_AND_CLEAN_CONNECT_NO_FREE(ret, WLAN_ERROR, &ConnectParams);

        /* Wait for connection events:
         * In order to verify that connection was successful,
         * we pend on two incoming events: Connected and Ip acquired.
         * The semaphores below are pend by this (Main) context.
         * They will be signaled once an asynchronous event
         * Indicating that the NWP has connected and acquired IP address is raised.
         * For further information, see this application read me file.
         */
        if(!IS_CONNECTED(app_CB.Status))
        {
            ret = sem_wait_timeout(&app_CB.CON_CB.connectEventSyncObj,
                                   WLAN_EVENT_TOUT);
            if(ret == TIMEOUT_SEM)
            {
                UART_PRINT("\n\r[wlanconnect] : Timeout expired connecting to AP: %s\n\r",
                           ConnectParams.ssid);
                handle_wifi_disconnection(app_CB.Status);
                continue;
            }
        }

        if(!IS_IP_ACQUIRED(app_CB.Status))
        {
            ret = sem_wait_timeout(&app_CB.CON_CB.ip4acquireEventSyncObj,
                                   WLAN_EVENT_TOUT);
            if(ret == TIMEOUT_SEM)
            {
                /* In next step try to get IPv6,
                  may be router/AP doesn't support IPv4 */
                UART_PRINT(
                    "\n\r[wlanconnect] : Timeout expired to acquire IPv4 address.\n\r");
            }
        }

//        if(!IS_IPV6G_ACQUIRED(app_CB.Status) &&
//           !IS_IPV6L_ACQUIRED(app_CB.Status) && !IS_IP_ACQUIRED(app_CB.Status))
//        {
//            UART_PRINT("\n\r[line:%d, error:%d] %s\n\r", __LINE__, -1,
//                       "Network Error");
//        }
        else
        {
            connected_to_ap = 1;

            UART_PRINT("\n\rconnectToAP() call successful, IP set to: IPv4=%d.%d.%d.%d , "
                                                "Gateway=%d.%d.%d.%d\n\r",

                                                SL_IPV4_BYTE(app_CB.CON_CB.IpAddr,3),
                                                SL_IPV4_BYTE(app_CB.CON_CB.IpAddr,2),
                                                SL_IPV4_BYTE(app_CB.CON_CB.IpAddr,1),
                                                SL_IPV4_BYTE(app_CB.CON_CB.IpAddr,0),

                                                SL_IPV4_BYTE(app_CB.CON_CB.GatewayIP,3),
                                                SL_IPV4_BYTE(app_CB.CON_CB.GatewayIP,2),
                                                SL_IPV4_BYTE(app_CB.CON_CB.GatewayIP,1),
                                                SL_IPV4_BYTE(app_CB.CON_CB.GatewayIP,0));
        }
    }

    return(0);
}


void handle_wifi_disconnection(uint32_t status)
{
    long lRetVal = -1;
    lRetVal = sl_WlanDisconnect();
    if(0 == lRetVal)
    {
        // Wait Till gets disconnected successfully..
        while(IS_CONNECTED(status))
        {
            Message("checking connection\n\r");
#if ENABLE_WIFI_DEBUG
            Message("looping at handle-disconn..\n\r");
#endif
            sleep(1);
        }
        Message("Stuck Debug :- Disconnection handled properly \n\r");
    }
}



//-----------------------------------------------------------------------------------------------------------------------------------------------
//ADC TESTING



int32_t time_beacons_and_accelerometer(ADC_Handle *adc0, ADC_Handle *adc1, ADC_Handle *adc2)
{
    /* ALWAYS DECLARE ALL VARIABLES AT TOP OF FUNCTION TO AVOID BUFFER ISSUES */
    uint32_t channel = 11;
    int current_ts_index = 0;
    _i16 cur_channel;
    _i16 numBytes;
    _i16 status;
    int32_t sent_bytes;
    int32_t bytes_to_send;
    int32_t buflen;
    uint32_t i=0;
    uint32_t j=0;
    _u32 nonBlocking = 1;
    uint32_t timestamp = 0;
    uint16_t beacInterval;
    frameInfo_t frameInfo;
    _i16 beaconRxSock;
    struct timespec cur_time;
    uint32_t last_beac_ts = 0;
    uint8_t Rx_frame[MAX_RX_PACKET_SIZE];
    uint32_t send_beac_ts = 0;
    uint32_t send_interval = 10000;
    int32_t last_local_ts = 0;

    sockAddr_t sAddr;
    uint16_t entry_port = ENTRY_PORT;
    SlSockAddr_t * sa;
    int32_t tcp_sock;
    int32_t addrSize;
    int32_t no_bytes_count = 0;
    int32_t beacon_count = 0;
    uint32_t badbeacon = 184812040;
    int x;
    float accel_sqr;

    int res0, res1, res2;
    uint16_t adcraw0, adcraw1, adcraw2;
    float adc0mv, adc1mv, adc2mv;

    /* filling the TCP server socket address */
    sAddr.in4.sin_family = SL_AF_INET;

    /* Since this is the client's side,
     * we must know beforehand the IP address
     * and the port of the server wer'e trying to connect.
     */
    sAddr.in4.sin_port = sl_Htons((unsigned short)entry_port);
    sAddr.in4.sin_addr.s_addr = 0;

    sa = (SlSockAddr_t*)&sAddr.in4;
    addrSize = sizeof(SlSockAddrIn6_t);

    beaconRxSock = enter_tranceiver_mode(1, channel);

    while(1)
    {
        clock_gettime(CLOCK_REALTIME, &cur_time);
        numBytes = sl_Recv(beaconRxSock, &Rx_frame, MAX_RX_PACKET_SIZE, 0);

        if(numBytes != SL_ERROR_BSD_EAGAIN)
        {
            if(numBytes < 0)
            {
                UART_PRINT("[line:%d, error:%d] %s\n\r", __LINE__, numBytes,
                           SL_SOCKET_ERROR);
                continue;
            }
            //UART_PRINT("Beacon recieved \n\r");


            parse_beacon_frame(Rx_frame, &frameInfo, 0);
            if(frameInfo.timestamp != badbeacon && frameInfo.timestamp != last_beac_ts){
                last_beac_ts = frameInfo.timestamp;
                beacon_count+=1;
                UART_PRINT("%d beacons recieved\n\r", beacon_count);
                last_local_ts = (int32_t) (cur_time.tv_sec * 1000 + cur_time.tv_nsec / 1000000);
            }

        }


        for(x=0; x<1; x++){
            //Get accel readings
            res0 = ADC_convert(*adc0, &adcraw0);

            //make sure readings were successful
            if (res0 == ADC_STATUS_SUCCESS) {
                adc0mv = (float)adcraw0/1.14;
            }
            else {
                UART_PRINT("CONFIG_ADC_0 convert failed\n\r");
            }

            res1 = ADC_convert(*adc1, &adcraw1);
            if (res1 == ADC_STATUS_SUCCESS) {
                adc1mv = (float)adcraw1/1.14;
            }
            else {
                UART_PRINT("CONFIG_ADC_1 convert failed\n\r");
            }

            res2 = ADC_convert(*adc2, &adcraw2);

            if (res2 == ADC_STATUS_SUCCESS) {
                adc2mv = (float)adcraw2/1.14;
            }
            else {
                UART_PRINT("CONFIG_ADC_2 convert failed\n\r");
            }

    //            UART_PRINT("adc0mv: %f\n\r", adc0mv);

            timestamps[0][current_ts_index] = (uint32_t) last_beac_ts;
            timestamps[1][current_ts_index] = (uint32_t) (cur_time.tv_sec * 1000 + cur_time.tv_nsec / 1000000);
            //calculate (acceleration)^2
            accel_sqr = (adc0mv * adc0mv) + (adc1mv * adc1mv) + (adc2mv * adc2mv);
            adc_readings[current_ts_index] = accel_sqr;
            //UART_PRINT("Beacon_ts interval: %d\n\r", last_beac_ts-frameInfo.timestamp);
            //UART_PRINT("Beacon_ts: %d\n\r", frameInfo.timestamp);

            current_ts_index = (current_ts_index + 1) % NUM_READINGS;
            usleep(9000);   //sleep for 10 ms
            clock_gettime(CLOCK_REALTIME, &cur_time);
        }



        if(last_local_ts != 0 && (int32_t)(cur_time.tv_sec * 1000 + cur_time.tv_nsec / 1000000) - last_local_ts >= BEACON_TIME_TRIGGER)
        {
            // stop transeiver modetest_ap_1
            // connnect to accept point
            // connect to tcp socket
            // send data
            // re-enable tranceiver mode

            beacon_count = 0;
            UART_PRINT("Exiting tranciever mode\n\r");
            status = sl_Close(beaconRxSock);
            ASSERT_ON_ERROR(status, SL_SOCKET_ERROR);

            UART_PRINT("its been %u ms (AP timestamp: %u ms), time to send time sync data, "
                   "will connect to AP and send in a few seconds\n\r", send_interval, frameInfo.timestamp/1000);
            sleep(2);

            status = connectToAP();
            if(status < 0)
            {
               UART_PRINT("could not connect to AP\n\r");
               return -1;
            }

            if(!sAddr.in4.sin_addr.s_addr)
               sAddr.in4.sin_addr.s_addr = sl_Htonl((unsigned int)app_CB.CON_CB.GatewayIP);

            /* Get socket descriptor - this would be the
            * socket descriptor for the TCP session.
            */
            tcp_sock = sl_Socket(sa->sa_family, SL_SOCK_STREAM, TCP_PROTOCOL_FLAGS);
            ASSERT_ON_ERROR(tcp_sock, SL_SOCKET_ERROR);

            status = -1;

            while(status < 0)
            {
               /* Calling 'sl_Connect' followed by server's
                * 'sl_Accept' would start session with
                * the TCP server. */
               status = sl_Connect(tcp_sock, sa, addrSize);
               if((status == SL_ERROR_BSD_EALREADY)&& (TRUE == nonBlocking))
               {
                   sleep(1);
                   continue;
               }
               else if(status < 0)
               {
                   UART_PRINT("[line:%d, error:%d] %s\n\r", __LINE__, status,
                              SL_SOCKET_ERROR);
                   sl_Close(tcp_sock);
                   UART_PRINT("No TCP socket to connect to, terminating program\n");
                   return(-1);
               }
               break;
            }

            memset(Tx_data, 0, MAX_TX_PACKET_SIZE);

            accel_to_string(timestamps, adc_readings, current_ts_index, Tx_data);

            //UART_PRINT("%s\n\r", Tx_data);

            sent_bytes = 0;
            bytes_to_send = strlen(Tx_data);

            status = sl_Send(tcp_sock, &Tx_data, strlen(Tx_data), 0);
            UART_PRINT("bytes sent: %i\n\r",status);
            UART_PRINT("String length of TX data: %i\n\r", strlen(Tx_data));


            status = sl_Close(tcp_sock);
            ASSERT_ON_ERROR(status, SL_SOCKET_ERROR);

            /* After calling sl_WlanDisconnect(),
            *    we expect WLAN disconnect asynchronous event.
            * Cleaning the former connection information from
            * the application control block
            * is handled in that event handler,
            * as well as getting the disconnect reason.
            */
            sleep(2);
            status = sl_WlanDisconnect();
            ASSERT_ON_ERROR(status, WLAN_ERROR);

            UART_PRINT("done sending time sync data and disconnected from AP"
                   ", will re-enter transceiver mode in a few seconds\n\r");
            sleep(2);
            beaconRxSock = enter_tranceiver_mode(0, channel);

            send_beac_ts += send_interval;
            UART_PRINT("next timestamp to send data at: %u\n\r", send_beac_ts);

            last_local_ts = 0;
        }
    }

    /* Calling 'close' with the socket descriptor,
    * once operation is finished. */
    status = sl_Close(beaconRxSock);
    ASSERT_ON_ERROR(status, SL_SOCKET_ERROR);

    return(0);
}




void load_cell_test(ADC_Handle *adc0){
    // prints accelerometer readings forever according to a continuous timer,
    // and waits 1 second between readings if the first line of the while loop is not commented out

    int res0;
    uint16_t adcraw0;
    float adc0mv;


    while(1) {
        sleep(1);

        //Get accel readings
        res0 = ADC_convert(*adc0, &adcraw0);

        //make sure readings were successful
        if (res0 == ADC_STATUS_SUCCESS) {
            adc0mv = (float)adcraw0/1.14;
        }
        else {
            UART_PRINT("CONFIG_ADC_0 convert failed\n\r");
        }

        UART_PRINT("Load cell mV: %f\n\r", adc0mv);
    }
}




int32_t time_beacons_and_load_cell(ADC_Handle *adc0)
{
    /* ALWAYS DECLARE ALL VARIABLES AT TOP OF FUNCTION TO AVOID BUFFER ISSUES */
    uint32_t channel = 11;
    int current_ts_index = 0;
    _i16 cur_channel;
    _i16 numBytes;
    _i16 status;
    int32_t sent_bytes;
    int32_t bytes_to_send;
    int32_t buflen;
    uint32_t i=0;
    uint32_t j=0;
    _u32 nonBlocking = 1;
    uint32_t timestamp = 0;
    uint16_t beacInterval;
    frameInfo_t frameInfo;
    _i16 beaconRxSock;
    //int32_t reading[MAX_ELEM_ARR_SIZE];
    struct timespec cur_time;
    uint32_t last_beac_ts = 0;
    //int32_t counter = 0;
    uint8_t Rx_frame[MAX_RX_PACKET_SIZE];
    uint32_t send_beac_ts = 0;
    uint32_t send_interval = 10000;
    int32_t last_local_ts = 0;

    sockAddr_t sAddr;
    uint16_t entry_port = ENTRY_PORT;
    SlSockAddr_t * sa;
    int32_t tcp_sock;
    int32_t addrSize;
    int32_t no_bytes_count = 0;
    int32_t beacon_count = 0;
    _u32 hw_timestamp;
    _u32 last_hw_timestamp = 0;
    uint32_t badbeacon = 184812040;

    int res0;
    uint16_t adcraw0;
    float adc0mv;
    int x;

    /* filling the TCP server socket address */
    sAddr.in4.sin_family = SL_AF_INET;

    /* Since this is the client's side,
     * we must know beforehand the IP address
     * and the port of the server wer'e trying to connect.
     */
    sAddr.in4.sin_port = sl_Htons((unsigned short)entry_port);
    sAddr.in4.sin_addr.s_addr = 0;

    sa = (SlSockAddr_t*)&sAddr.in4;
    addrSize = sizeof(SlSockAddrIn6_t);

    beaconRxSock = enter_tranceiver_mode(1, channel);

    while(1)
    {
//        if(no_bytes_count == 1000){
//            UART_PRINT("1000 loops with no bytes found\n\r");
//            no_bytes_count = 0;
//        }

        clock_gettime(CLOCK_REALTIME, &cur_time);

        //UART_PRINT("start of loop\n");
        numBytes = sl_Recv(beaconRxSock, &Rx_frame, MAX_RX_PACKET_SIZE, 0);

        if(numBytes != SL_ERROR_BSD_EAGAIN)
        {
            //UART_PRINT("num_bytes: %d\n\r", numBytes);
            if(numBytes < 0)
            {
                UART_PRINT("[line:%d, error:%d] %s\n\r", __LINE__, numBytes,
                           SL_SOCKET_ERROR);
                continue;
            }
            //UART_PRINT("Beacon recieved \n\r");


            hw_timestamp = parse_beacon_frame(Rx_frame, &frameInfo, 0);
            if(frameInfo.timestamp != badbeacon && frameInfo.timestamp != last_beac_ts){
                last_beac_ts = frameInfo.timestamp;
                beacon_count+=1;
                UART_PRINT("%d beacons recieved\n\r", beacon_count);
                last_local_ts = (int32_t) (cur_time.tv_sec * 1000 + cur_time.tv_nsec / 1000000);
            }

        }

        for(x=0; x<1; x++){
            //Get accel readings
            res0 = ADC_convert(*adc0, &adcraw0);

            //make sure readings were successful
            if (res0 == ADC_STATUS_SUCCESS) {
                adc0mv = (float)adcraw0/1.14;
            }
            else {
                UART_PRINT("CONFIG_ADC_0 convert failed\n\r");
            }

            //UART_PRINT("adc0mv: %f\n\r", adc0mv);

            timestamps[0][current_ts_index] = (uint32_t) last_beac_ts;
            timestamps[1][current_ts_index] = (uint32_t) (cur_time.tv_sec * 1000 + cur_time.tv_nsec / 1000000);
            adc_readings[current_ts_index] = adc0mv;
            //UART_PRINT("Beacon_ts interval: %d\n\r", last_beac_ts-frameInfo.timestamp);
            //UART_PRINT("Beacon_ts:\t%u\n\r", frameInfo.timestamp);
            //UART_PRINT("Beacon_ts in array: %u\n\r", timestamps[0][current_ts_index]);
    //        UART_PRINT("HW timestamp:\t%ul\n\r", hw_timestamp);
            //UART_PRINT("Load cell reading: %f\n\r", load_cell_readings[current_ts_index]);

            current_ts_index = (current_ts_index + 1) % NUM_READINGS;
    //        last_beac_ts = frameInfo.timestamp;
            last_hw_timestamp = hw_timestamp;
            usleep(9000);   //sleep for 10 ms
            clock_gettime(CLOCK_REALTIME, &cur_time);
        }


        if(last_local_ts != 0 && (int32_t)(cur_time.tv_sec * 1000 + cur_time.tv_nsec / 1000000) - last_local_ts >= BEACON_TIME_TRIGGER)
        {
            // stop transeiver modetest_ap_1
            // connnect to accept point
            // connect to tcp socket
            // send data
            // re-enable tranceiver mode

            beacon_count = 0;
            UART_PRINT("Exiting tranciever mode\n\r");
            status = sl_Close(beaconRxSock);
            ASSERT_ON_ERROR(status, SL_SOCKET_ERROR);

            UART_PRINT("its been %u ms (AP timestamp: %u ms), time to send time sync data, "
                   "will connect to AP and send in a few seconds\n\r", send_interval, frameInfo.timestamp/1000);
            sleep(2);

            status = connectToAP();
            if(status < 0)
            {
               UART_PRINT("could not connect to AP\n\r");
               return -1;
            }

            if(!sAddr.in4.sin_addr.s_addr)
               sAddr.in4.sin_addr.s_addr = sl_Htonl((unsigned int)app_CB.CON_CB.GatewayIP);

            /* Get socket descriptor - this would be the
            * socket descriptor for the TCP session.
            */
            tcp_sock = sl_Socket(sa->sa_family, SL_SOCK_STREAM, TCP_PROTOCOL_FLAGS);
            ASSERT_ON_ERROR(tcp_sock, SL_SOCKET_ERROR);

            status = -1;

            while(status < 0)
            {
               /* Calling 'sl_Connect' followed by server's
                * 'sl_Accept' would start session with
                * the TCP server. */
               status = sl_Connect(tcp_sock, sa, addrSize);
               if((status == SL_ERROR_BSD_EALREADY)&& (TRUE == nonBlocking))
               {
                   sleep(1);
                   continue;
               }
               else if(status < 0)
               {
                   UART_PRINT("[line:%d, error:%d] %s\n\r", __LINE__, status,
                              SL_SOCKET_ERROR);
                   sl_Close(tcp_sock);
                   UART_PRINT("No TCP socket to connect to, terminating program\n");
                   return(-1);
               }
               break;
            }

            memset(Tx_data, 0, MAX_TX_PACKET_SIZE);

            ts_to_string(timestamps, adc_readings, current_ts_index, Tx_data);
            //UART_PRINT("%s\n\r", Tx_data);

            //UART_PRINT("%s\n\r", Tx_data);

            sent_bytes = 0;
            bytes_to_send = strlen(Tx_data);

            status = sl_Send(tcp_sock, &Tx_data, strlen(Tx_data), 0);
            UART_PRINT("bytes sent: %i\n\r",status);
            UART_PRINT("String length of TX data: %i\n\r", strlen(Tx_data));

            status = sl_Close(tcp_sock);
            ASSERT_ON_ERROR(status, SL_SOCKET_ERROR);

            /* After calling sl_WlanDisconnect(),
            *    we expect WLAN disconnect asynchronous event.
            * Cleaning the former connection information from
            * the application control block
            * is handled in that event handler,
            * as well as getting the disconnect reason.
            */
            sleep(2);
            status = sl_WlanDisconnect();
            ASSERT_ON_ERROR(status, WLAN_ERROR);

            UART_PRINT("done sending time sync data and disconnected from AP"
                   ", will re-enter transceiver mode in a few seconds\n\r");
            sleep(2);
            beaconRxSock = enter_tranceiver_mode(0, channel);

            send_beac_ts += send_interval;
            UART_PRINT("next timestamp to send data at: %u\n\r", send_beac_ts);

            last_local_ts = 0;
        }
    }

    /* Calling 'close' with the socket descriptor,
    * once operation is finished. */
    status = sl_Close(beaconRxSock);
    ASSERT_ON_ERROR(status, SL_SOCKET_ERROR);

    return(0);
}







//-----------------------------------------------------------------------------------------------------------------------------------------------



int32_t test_time_beac_sync()
{
    /* ALWAYS DECLARE ALL VARIABLES AT TOP OF FUNCTION TO AVOID BUFFER ISSUES */
    uint32_t channel = 1;
    int current_ts_index = 0;
    _i16 cur_channel;
    _i16 numBytes;
    _i16 status;
    int32_t sent_bytes;
    int32_t bytes_to_send;
    int32_t buflen;
    uint32_t i=0;
    uint32_t j=0;
    _u32 nonBlocking = 1;
    uint32_t timestamp = 0;
    uint16_t beacInterval;
    frameInfo_t frameInfo;
    _i16 beaconRxSock;
    //int32_t reading[MAX_ELEM_ARR_SIZE];
    struct timespec cur_time;
    uint32_t last_beac_ts = 0;
    //int32_t counter = 0;
    uint8_t Rx_frame[MAX_RX_PACKET_SIZE];
    uint32_t send_beac_ts = 0;
    uint32_t send_interval = 30000;
    _u32 hw_timestamp;
    _u32 last_hw_timestamp = 0;
    uint32_t local_ts, last_local_ts;

    sockAddr_t sAddr;
    uint16_t entry_port = ENTRY_PORT;
    SlSockAddr_t * sa;
    int32_t tcp_sock;
    int32_t addrSize;
    int32_t no_bytes_count = 0;
    int32_t beacon_count = 0;

    /* filling the TCP server socket address */
    sAddr.in4.sin_family = SL_AF_INET;

    /* Since this is the client's side,
     * we must know beforehand the IP address
     * and the port of the server wer'e trying to connect.
     */
    sAddr.in4.sin_port = sl_Htons((unsigned short)entry_port);
    sAddr.in4.sin_addr.s_addr = 0;

    sa = (SlSockAddr_t*)&sAddr.in4;
    addrSize = sizeof(SlSockAddrIn6_t);

    beaconRxSock = enter_tranceiver_mode(1, channel);

    while(1)
    {
        if(no_bytes_count == 1000){
            //UART_PRINT("1000 loops with no bytes found\n\r");
            no_bytes_count = 0;
        }
        //UART_PRINT("start of loop\n");
        //sleep(1);
        clock_gettime(CLOCK_REALTIME, &cur_time);
        numBytes = sl_Recv(beaconRxSock, &Rx_frame, MAX_RX_PACKET_SIZE, 0);

        if(numBytes != SL_ERROR_BSD_EAGAIN)
        {
            //UART_PRINT("num_bytes: %d\n\r", numBytes);
            if(numBytes < 0)
            {
                UART_PRINT("[line:%d, error:%d] %s\n\r", __LINE__, numBytes,
                           SL_SOCKET_ERROR);
                continue;
            }
            //UART_PRINT("Beacon recieved \n\r");
            hw_timestamp = parse_beacon_frame(Rx_frame, &frameInfo, 0);
            beacon_count+=1;
            //UART_PRINT("%d beacons recieved\n\r", beacon_count);
        }
        else{
            no_bytes_count += 1;
            continue;
        }

        if(last_beac_ts == frameInfo.timestamp)
            continue;



        timestamps[0][current_ts_index] = (uint32_t) frameInfo.timestamp;
        local_ts = (uint32_t) (cur_time.tv_sec * 1000 + cur_time.tv_nsec / 1000000);
        timestamps[1][current_ts_index] = local_ts;
        hw_timestamps[current_ts_index] = hw_timestamp;

        //UART_PRINT("Beacon_ts interval: %d\n\r", last_beac_ts-frameInfo.timestamp);
//        UART_PRINT("Beacon_ts:   \t%u\n\r", frameInfo.timestamp);
//        UART_PRINT("hw_timestamp:\t%ul\n\r", hw_timestamp);
//        UART_PRINT("Local_ts    :\t%u\n\r", local_ts);
        //UART_PRINT("%u,%lu,%u\n", frameInfo.timestamp, hw_timestamp, local_ts);

        current_ts_index = (current_ts_index + 1) % NUM_READINGS;
        last_beac_ts = frameInfo.timestamp;

        if(send_beac_ts==0)
        {
            send_beac_ts = send_interval + (frameInfo.timestamp/1000 - (frameInfo.timestamp/1000 % send_interval));
            //UART_PRINT("%u = %u + (%u - (%u %% %u))\n\r", send_beac_ts, send_interval,frameInfo.timestamp/1000,frameInfo.timestamp/1000,
            //           send_interval);
        }

        if(current_ts_index == (NUM_READINGS - 1)){
            for(i=0;i<NUM_READINGS;i++){
                UART_PRINT("%u,%lu,%u\n", timestamps[0][i], hw_timestamps[i], timestamps[1][i]);
            }
            return 0;
        }

        if(frameInfo.timestamp/1000 >= send_beac_ts)
        {
            // stop transeiver mode
            // connnect to accept point
            // connect to tcp socket
            // send data
            // re-enable tranceiver mode

            beacon_count = 0;
            UART_PRINT("Exiting tranciever mode\n\r");
            status = sl_Close(beaconRxSock);
            ASSERT_ON_ERROR(status, SL_SOCKET_ERROR);

            UART_PRINT("its been %u ms (AP timestamp: %u ms), time to send time sync data, "
                    "will connect to AP and send in a few seconds\n\r", send_interval, frameInfo.timestamp/1000);
            sleep(2);

            status = connectToAP();
            if(status < 0)
            {
                UART_PRINT("could not connect to AP\n\r");
                return -1;
            }

            if(!sAddr.in4.sin_addr.s_addr)
                sAddr.in4.sin_addr.s_addr = sl_Htonl((unsigned int)app_CB.CON_CB.GatewayIP);

            /* Get socket descriptor - this would be the
             * socket descriptor for the TCP session.
             */
            tcp_sock = sl_Socket(sa->sa_family, SL_SOCK_STREAM, TCP_PROTOCOL_FLAGS);
            ASSERT_ON_ERROR(tcp_sock, SL_SOCKET_ERROR);

            status = -1;

            while(status < 0)
            {
                /* Calling 'sl_Connect' followed by server's
                 * 'sl_Accept' would start session with
                 * the TCP server. */
                status = sl_Connect(tcp_sock, sa, addrSize);
                if((status == SL_ERROR_BSD_EALREADY)&& (TRUE == nonBlocking))
                {
                    sleep(1);
                    continue;
                }
                else if(status < 0)
                {
                    UART_PRINT("[line:%d, error:%d] %s\n\r", __LINE__, status,
                               SL_SOCKET_ERROR);
                    sl_Close(tcp_sock);
                    UART_PRINT("No TCP socket to connect to, terminating program\n\r");
                    return(-1);
                }
                break;
            }

            memset(Tx_data, 0, MAX_TX_PACKET_SIZE);

            //ts_to_string(timestamps, [1.0, 2.0], current_ts_index, Tx_data);

            sent_bytes = 0;
            bytes_to_send = strlen(Tx_data);
            while(sent_bytes < bytes_to_send)
            {
                if(bytes_to_send - sent_bytes >= bytes_to_send)
                {
                    buflen = bytes_to_send;
                }
                else
                {
                    buflen = bytes_to_send - sent_bytes;
                }

                status = sl_Send(tcp_sock, &Tx_data, buflen, 0);
                if(status < 0)
                {
                    UART_PRINT("[line:%d, error:%d] %s\n\r", __LINE__, status,
                               SL_SOCKET_ERROR);
                    break;
                }
                sent_bytes += status;
            }

            status = sl_Close(tcp_sock);
            ASSERT_ON_ERROR(status, SL_SOCKET_ERROR);

            /* After calling sl_WlanDisconnect(),
             *    we expect WLAN disconnect asynchronous event.
             * Cleaning the former connection information from
             * the application control block
             * is handled in that event handler,
             * as well as getting the disconnect reason.
             */
            sleep(2);
            status = sl_WlanDisconnect();
            ASSERT_ON_ERROR(status, WLAN_ERROR);

            UART_PRINT("done sending time sync data and disconnected from AP"
                    ", will re-enter transceiver mode in a few seconds\n\r");
            sleep(2);
            beaconRxSock = enter_tranceiver_mode(0, channel);

            send_beac_ts += send_interval;
            UART_PRINT("next timestamp to send data at: %u\n\r", send_beac_ts);
        }
    }

    /* Calling 'close' with the socket descriptor,
    * once operation is finished. */
    status = sl_Close(beaconRxSock);
    ASSERT_ON_ERROR(status, SL_SOCKET_ERROR);

    return(0);
}


_u32 parse_beacon_frame(uint8_t * Rx_frame, frameInfo_t * frameInfo, uint8_t printInfo){
    // return value is the hardware timestamp from the proprietary header offset
    int32_t hdrOfs = 8;   // proprietary header offset
    uint32_t timestamp = 0;
    _u32 hw_timestamp;
    int32_t i = 31;     // last byte index of timestamp in beacon frame
    int32_t j;
    SlTransceiverRxOverHead_t *transHeader;
    _i8 buffer[15];
    memcpy(buffer, &Rx_frame, 14);
    transHeader = (SlTransceiverRxOverHead_t *)Rx_frame;
    hw_timestamp = transHeader->Timestamp;


    // can only work with 4 bytes out of the 8 byte timestamp value since this is a 32-bit machine
    while(Rx_frame[hdrOfs+i] == 0 && i > 28)
        i--;
    for(j=0;i-4+j<=i;j++)
        timestamp |= Rx_frame[hdrOfs+ i - 4 + j] << (j*8);

    frameInfo->timestamp = timestamp;

    if(printInfo)
    {
        frameInfo->frameControl = Rx_frame[hdrOfs+1] | (Rx_frame[hdrOfs] << 8);
        frameInfo->duration = Rx_frame[hdrOfs+3] | (Rx_frame[hdrOfs+2] << 8);
        memcpy(frameInfo->destAddr,&Rx_frame[hdrOfs+4], 6);
        memcpy(frameInfo->sourceAddr,&Rx_frame[hdrOfs+10], 6);
        memcpy(frameInfo->bssid,&Rx_frame[hdrOfs+16], 6);
        frameInfo->seqCtrl = Rx_frame[hdrOfs+23] | (Rx_frame[hdrOfs+22] << 8);
        frameInfo->beaconInterval = Rx_frame[hdrOfs+32] | (Rx_frame[hdrOfs+33] << 8); // remember beacon interval is backwards
        frameInfo->beaconIntervalMs = frameInfo->beaconInterval * 1024;
        frameInfo->capabilityInfo = Rx_frame[hdrOfs+35] | (Rx_frame[hdrOfs+34] << 8);
        frameInfo->ssidElemId = Rx_frame[hdrOfs+36];
        frameInfo->ssidLen = Rx_frame[hdrOfs+37];

        for(j=0;j<frameInfo->ssidLen;j++)
            frameInfo->ssid[j] = Rx_frame[hdrOfs+38+j];
        frameInfo->ssid[frameInfo->ssidLen + 1] = '\0';


        UART_PRINT("frame control: %04x\n\r", frameInfo->frameControl);
        UART_PRINT("duration: %04x\n\r", frameInfo->duration);
        UART_PRINT("dest addr: %02x:%02x:%02x:%02x:%02x:%02x\n\r", frameInfo->destAddr[0], frameInfo->destAddr[1],
                   frameInfo->destAddr[2], frameInfo->destAddr[3], frameInfo->destAddr[4], frameInfo->destAddr[5]);
        UART_PRINT("source addr: %02x:%02x:%02x:%02x:%02x:%02x\n\r", frameInfo->sourceAddr[0], frameInfo->sourceAddr[1],
                   frameInfo->sourceAddr[2], frameInfo->sourceAddr[3], frameInfo->sourceAddr[4], frameInfo->sourceAddr[5]);
        UART_PRINT("BSS ID: %02x:%02x:%02x:%02x:%02x:%02x\n\r", frameInfo->bssid[0], frameInfo->bssid[1],
                   frameInfo->bssid[2], frameInfo->bssid[3], frameInfo->bssid[4], frameInfo->bssid[5]);
        UART_PRINT("sequence control: %04x\n\r", frameInfo->seqCtrl);
        UART_PRINT("timestamp: %u\n\r", frameInfo->timestamp);
        UART_PRINT("beacon interval: %u TU\n\r", frameInfo->beaconInterval);
        UART_PRINT("capability info: %04x\n\r", frameInfo->capabilityInfo);
        UART_PRINT("SSID Element ID: %02x\n\r", frameInfo->ssidElemId);
        UART_PRINT("SSID Length: %02x\n\r", frameInfo->ssidLen);
        UART_PRINT("SSID: %s\n\r", frameInfo->ssid);
    }

    return hw_timestamp;
}

int32_t ts_to_string(uint32_t timestamps[][NUM_READINGS], float load_cell_readings[], uint32_t current_ts_index, uint8_t * buf)
{
    int32_t i = 0;
    int32_t buf_i = 0;
    uint8_t reading[40]; // max reading to string length is 24: "(4294967296,4294967296,4294967296|\'0')"

    for(i=current_ts_index; i<NUM_READINGS; i++){
        sprintf(reading, "%u,%u,%f|", timestamps[0][i], timestamps[1][i], load_cell_readings[i]);
        //UART_PRINT("Reading %s\r\n", reading);
        strcpy(&buf[buf_i], reading);
        buf_i += strlen(reading);
    }

    for(i=0; i<current_ts_index; i++){
        sprintf(reading, "%u,%u,%f|", timestamps[0][i], timestamps[1][i], load_cell_readings[i]);
        strcpy(&buf[buf_i], reading);
        buf_i += strlen(reading);
    }


    return 0;
}



int32_t accel_to_string(uint32_t timestamps[][NUM_READINGS], float accel_readings[], uint32_t current_ts_index, uint8_t * buf)
{
    int32_t i = 0;
    int32_t buf_i = 0;
    uint8_t reading[65]; // max reading to string length is 24: "(4294967296,4294967296,4294967296,4294967296,4294967296|\'0')"

    for(i=current_ts_index; i<NUM_READINGS; i++){
        sprintf(reading, "%u,%u,%f,0|", timestamps[0][i], timestamps[1][i], accel_readings[i]);
        strcpy(&buf[buf_i], reading);
        buf_i += strlen(reading);
    }

    for(i=0; i<current_ts_index; i++){
        sprintf(reading, "%u,%u,%f,0|", timestamps[0][i], timestamps[1][i], accel_readings[i]);
        strcpy(&buf[buf_i], reading);
        buf_i += strlen(reading);
    }


    return 0;
}


_i16 enter_tranceiver_mode(int32_t first_time, uint32_t channel)
{
    /* ALWAYS DECLARE ALL VARIABLES AT TOP OF FUNCTION TO AVOID BUFFER ISSUES */
    _i16 status;
    _u32 nonBlocking = 1;
    _i16 Tx_sock;
    uint8_t enableFilterArgs[] = "";

    if(first_time == 1){
        uint8_t createFilterArgs1[] = " -f FRAME_TYPE -v management -e not_equals -a drop -m L1";
        uint8_t createFilterArgs2[] = MAC_FILTER_ARGS;


        status = cmdCreateFilterCallback(createFilterArgs1);
        ASSERT_ON_ERROR(status, WLAN_ERROR);

        status = cmdCreateFilterCallback(createFilterArgs2);
        ASSERT_ON_ERROR(status, WLAN_ERROR);
    }


    status = cmdEnableFilterCallback(enableFilterArgs);
    ASSERT_ON_ERROR(status, WLAN_ERROR);


    /* To use transceiver mode, the device must be set in STA role, be disconnected, and have disabled
        previous connection policies that might try to automatically connect to an AP. */
    status = sl_WlanPolicySet(SL_WLAN_POLICY_CONNECTION, SL_WLAN_CONNECTION_POLICY(0, 0, 0, 0), NULL, 0);
    if( status )
    {
        UART_PRINT("[line:%d, error:%d]\n\r", __LINE__, status);
        return(-1);
    }

    Tx_sock = sl_Socket(SL_AF_RF, SL_SOCK_DGRAM, channel);
    ASSERT_ON_ERROR(Tx_sock, SL_SOCKET_ERROR);

    status = sl_SetSockOpt(Tx_sock, SL_SOL_SOCKET, SL_SO_NONBLOCKING, &nonBlocking, sizeof(nonBlocking));
    if(status < 0)
    {
        UART_PRINT("[line:%d, error:%d] %s\n\r", __LINE__, status,
                   SL_SOCKET_ERROR);
        sl_Close(Tx_sock);
        return(-1);
    }

    return Tx_sock;
}
