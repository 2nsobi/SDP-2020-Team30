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
#include "queue.h"

#include <ti/sysbios/knl/Task.h>

/* Application defines */
#define WLAN_EVENT_TOUT             (10000)
#define TIMEOUT_SEM                 (-1)
#define TCP_PROTOCOL_FLAGS          0
#define BUF_LEN                     (MAX_BUF_SIZE - 20)

/* custom defines */
#define AP_SSID                     "test_ap_1"
#define AP_KEY                      "12345678"
#define ENTRY_PORT                  10000
#define BILLION                     1000000000
#define MESSAGE_SIZE                50
#define NUM_READINGS                300
#define MAX_RX_PACKET_SIZE          1544
#define MAX_TX_PACKET_SIZE          3000
#define MAC_FILTER_ARGS             " -f S_MAC -v 5A:FB:84:5D:70:05 -e not_equals -a drop -m L1"
#define BEACON_CHANNEL              11


/* for on-board accelerometer */
#include <ti/sail/bma2x2/bma2x2.h>

typedef union
{
    SlSockAddrIn6_t in6;       /* Socket info for Ipv6 */
    SlSockAddrIn_t in4;        /* Socket info for Ipv4 */
}sockAddr_t;

uint8_t Tx_data[MAX_TX_PACKET_SIZE];


int32_t connectToAP()
{
    int32_t ret;
    ConnectCmd_t ConnectParams;

    uint8_t ssid[] = AP_SSID;
    uint8_t key[] = AP_KEY;
    SlWlanSecParams_t secParams = { .Type = SL_WLAN_SEC_TYPE_WPA_WPA2,
                                    .Key = (signed char *)key,
                                    .KeyLen = strlen((const char *)key) };

    uint32_t connected_to_ap;

    connected_to_ap = 0;
    ret = 0;
    memset(&ConnectParams, 0x0, sizeof(ConnectCmd_t));

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
//                handle_wifi_disconnection(app_CB.Status);
//                continue;
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

        if(!IS_IPV6G_ACQUIRED(app_CB.Status) &&
           !IS_IPV6L_ACQUIRED(app_CB.Status) && !IS_IP_ACQUIRED(app_CB.Status))
        {
            UART_PRINT("\n\r[line:%d, error:%d] %s\n\r", __LINE__, -1,
                       "Network Error");
        }
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


//void handle_wifi_disconnection(uint32_t status)
//{
//    long lRetVal = -1;
//    lRetVal = sl_WlanDisconnect();
//    if(0 == lRetVal)
//    {
//        // Wait Till gets disconnected successfully..
//        while(IS_CONNECTED(status))
//        {
//            Message("checking connection\n\r");
//#if ENABLE_WIFI_DEBUG
//            Message("looping at handle-disconn..\n\r");
//#endif
//            sleep(1);
//        }
//        Message("Stuck Debug :- Disconnection handled properly \n\r");
//    }
//}


uint16_t get_port_for_data_tx()
{
    int32_t sock;
    int32_t status;
    uint32_t i = 0;
    SlSockAddr_t        *sa;
    int32_t addrSize;
    sockAddr_t sAddr;
    uint16_t portNumber = ENTRY_PORT;
    uint8_t notBlocking = 0;

    uint32_t sent_bytes = 0;
    uint8_t custom_msg[25];
    sprintf(custom_msg,"cc3220sf ipv4:%u",app_CB.CON_CB.IpAddr);
    uint32_t msg_size = strlen(custom_msg);
    uint32_t num_copies = 1;
    uint32_t bytes_to_send = (num_copies * msg_size);

    uint32_t rcvd_bytes = 0;
    uint32_t bytes_to_rcv = 5;
    uint8_t rcvd_msg_buff[bytes_to_rcv+1];
    uint8_t rcvd_msg[bytes_to_rcv+1];
    uint16_t receivedPort;

    uint32_t max_retries = 5;
    uint32_t tries = 0;

    UART_PRINT("\n\rENTRY_PORT: %x\n\r",portNumber);

    /* filling the TCP server socket address */
    sAddr.in4.sin_family = SL_AF_INET;

    /* Since this is the client's side,
     * we must know beforehand the IP address
     * and the port of the server wer'e trying to connect.
     */
    sAddr.in4.sin_port = sl_Htons((unsigned short)portNumber);
    sAddr.in4.sin_addr.s_addr = sl_Htonl((unsigned int)app_CB.CON_CB.GatewayIP);

    sa = (SlSockAddr_t*)&sAddr.in4;
    addrSize = sizeof(SlSockAddrIn6_t);

    /* Get socket descriptor - this would be the
     * socket descriptor for the TCP session.
     */
    sock = sl_Socket(sa->sa_family, SL_SOCK_STREAM, TCP_PROTOCOL_FLAGS);
    ASSERT_ON_ERROR(sock, SL_SOCKET_ERROR);

#ifdef SECURE_SOCKET

    SlDateTime_t dateTime;
    dateTime.tm_day = DEVICE_DATE;
    dateTime.tm_mon = DEVICE_MONTH;
    dateTime.tm_year = DEVICE_YEAR;

    sl_DeviceSet(SL_DEVICE_GENERAL, SL_DEVICE_GENERAL_DATE_TIME,
                 sizeof(SlDateTime_t), (uint8_t *)(&dateTime));

    /* Set the following to enable Server Authentication */
    sl_SetSockOpt(sock,SL_SOL_SOCKET,SL_SO_SECURE_FILES_CA_FILE_NAME,
                  ROOT_CA_CERT_FILE, strlen(
                      ROOT_CA_CERT_FILE));

#ifdef CLIENT_AUTHENTICATION
    /* Set the following to pass Client Authentication */
    sl_SetSockOpt(sock,SL_SOL_SOCKET,SL_SO_SECURE_FILES_PRIVATE_KEY_FILE_NAME,
                  PRIVATE_KEY_FILE, strlen(
                      PRIVATE_KEY_FILE));
    sl_SetSockOpt(sock,SL_SOL_SOCKET,SL_SO_SECURE_FILES_CERTIFICATE_FILE_NAME,
                  TRUSTED_CERT_CHAIN, strlen(
                      TRUSTED_CERT_CHAIN));
#endif
#endif

    status = -1;

    while(status < 0)
    {
        /* Calling 'sl_Connect' followed by server's
         * 'sl_Accept' would start session with
         * the TCP server. */
        status = sl_Connect(sock, sa, addrSize);
        if((status == SL_ERROR_BSD_EALREADY)&& (TRUE == notBlocking))
        {
            sleep(1);
            continue;
        }
        else if(status < 0)
        {
            UART_PRINT("[line:%d, error:%d] %s, retrying...\n\r", __LINE__, status,
                       SL_SOCKET_ERROR);
            sl_Close(sock);

            sleep(1);

            if(tries >= max_retries){
                UART_PRINT("[nnaji msg] error: unable to connect to AP's entry socket (Gateway IP=%d.%d.%d.%d, port=%s)"
                        " after %i retries\n\r",
                        SL_IPV4_BYTE(app_CB.CON_CB.GatewayIP,3),
                        SL_IPV4_BYTE(app_CB.CON_CB.GatewayIP,2),
                        SL_IPV4_BYTE(app_CB.CON_CB.GatewayIP,1),
                        SL_IPV4_BYTE(app_CB.CON_CB.GatewayIP,0),
                        portNumber,
                        max_retries);
                return(-1);
            }

            tries++;
            continue;
        }
        break;
    }

    i = 0;

    /////////////////////////////////////////////
    // send IP of this device to AP
    /////////////////////////////////////////////

    UART_PRINT("[nnaji] bytes_to_send=%i\n\r", bytes_to_send);

    while(sent_bytes < bytes_to_send)
    {
        /* Send packets to the server */
        status = sl_Send(sock, &custom_msg, msg_size, 0);

        UART_PRINT("[nnaji] return status value from sl_Send(): %i\n\r", status);

        if((status == SL_ERROR_BSD_EAGAIN) && (TRUE == notBlocking))
        {
            sleep(1);
            continue;
        }
        else if(status < 0)
        {
            UART_PRINT("[line:%d, error:%d] %s\n\r", __LINE__, status,
                       SL_SOCKET_ERROR);
            sl_Close(sock);
            return(-1);
        }
        i++;
        sent_bytes += status;
        UART_PRINT("[nnaji] bytes sent to far: %i\n\r", sent_bytes);
    }

    UART_PRINT("Sent %u packets (%u bytes) successfully\n\r",
               i,
               sent_bytes);

    ////////////////////////////////////////////////////////
    // receive port number for socket communication w/ AP
    ////////////////////////////////////////////////////////

    while(rcvd_bytes < bytes_to_rcv)
    {
        status = sl_Recv(sock, &rcvd_msg_buff, bytes_to_rcv+1, 0);
        if((status == SL_ERROR_BSD_EAGAIN) && (TRUE == notBlocking))
        {
            sleep(1);
            continue;
        }
        else if(status < 0)
        {
            UART_PRINT("[line:%d, error:%d] %s\n\r", __LINE__, status,
                       BSD_SOCKET_ERROR);
            sl_Close(sock);
            return(-1);
        }
        else if(status == 0)
        {
            UART_PRINT("TCP Server closed the connection\n\r");
            break;
        }
        else if(status > 0)
        {
            strcpy(rcvd_msg, (const char *)rcvd_msg_buff);
        }
        rcvd_bytes += status;
    }

    UART_PRINT("Received %u packets (%u bytes) successfully\n\r",
               (rcvd_bytes / bytes_to_rcv), rcvd_bytes);

    UART_PRINT("[nnaji] msg recieved: \"%s\"\n\r",&rcvd_msg);

    receivedPort = (uint16_t)atoi((const char *)rcvd_msg);
    UART_PRINT("[nnaji] port recieved: \"%i\"\n\r",receivedPort);

    /* Calling 'close' with the socket descriptor,
     * once operation is finished. */
    status = sl_Close(sock);
    ASSERT_ON_ERROR(status, SL_SOCKET_ERROR);

    return(receivedPort);
}

int32_t transmit_data_forever_test(uint16_t sockPort)
{
    int32_t sock;
    int32_t status;
    uint32_t i = 0;
    SlSockAddr_t        *sa;
    int32_t addrSize;
    sockAddr_t sAddr;
    uint8_t notBlocking = 0;

    uint32_t sent_bytes = 0;
    uint32_t buflen = 60;
    uint8_t custom_msg[buflen];

    /* for on-board accelerometer */
    /* structure to read the accelerometer data*/
    /* the resolution is in 8 bit*/
    struct bma2x2_accel_data_temp sample_xyzt;

    UART_PRINT("\n\rsockPort: %x\n\r",sockPort);

    /* filling the TCP server socket address */
    sAddr.in4.sin_family = SL_AF_INET;

    /* Since this is the client's side,
     * we must know beforehand the IP address
     * and the port of the server wer'e trying to connect.
     */
    sAddr.in4.sin_port = sl_Htons((unsigned short)sockPort);
    sAddr.in4.sin_addr.s_addr = sl_Htonl((unsigned int)app_CB.CON_CB.GatewayIP);

    sa = (SlSockAddr_t*)&sAddr.in4;
    addrSize = sizeof(SlSockAddrIn6_t);

    /* Get socket descriptor - this would be the
     * socket descriptor for the TCP session.
     */
    sock = sl_Socket(sa->sa_family, SL_SOCK_STREAM, TCP_PROTOCOL_FLAGS);
    ASSERT_ON_ERROR(sock, SL_SOCKET_ERROR);

#ifdef SECURE_SOCKET

    SlDateTime_t dateTime;
    dateTime.tm_day = DEVICE_DATE;
    dateTime.tm_mon = DEVICE_MONTH;
    dateTime.tm_year = DEVICE_YEAR;

    sl_DeviceSet(SL_DEVICE_GENERAL, SL_DEVICE_GENERAL_DATE_TIME,
                 sizeof(SlDateTime_t), (uint8_t *)(&dateTime));

    /* Set the following to enable Server Authentication */
    sl_SetSockOpt(sock,SL_SOL_SOCKET,SL_SO_SECURE_FILES_CA_FILE_NAME,
                  ROOT_CA_CERT_FILE, strlen(
                      ROOT_CA_CERT_FILE));

#ifdef CLIENT_AUTHENTICATION
    /* Set the following to pass Client Authentication */
    sl_SetSockOpt(sock,SL_SOL_SOCKET,SL_SO_SECURE_FILES_PRIVATE_KEY_FILE_NAME,
                  PRIVATE_KEY_FILE, strlen(
                      PRIVATE_KEY_FILE));
    sl_SetSockOpt(sock,SL_SOL_SOCKET,SL_SO_SECURE_FILES_CERTIFICATE_FILE_NAME,
                  TRUSTED_CERT_CHAIN, strlen(
                      TRUSTED_CERT_CHAIN));
#endif
#endif

    status = -1;

    while(status < 0)
    {
        /* Calling 'sl_Connect' followed by server's
         * 'sl_Accept' would start session with
         * the TCP server. */
        status = sl_Connect(sock, sa, addrSize);
        if((status == SL_ERROR_BSD_EALREADY)&& (TRUE == notBlocking))
        {
            sleep(1);
            continue;
        }
        else if(status < 0)
        {
            UART_PRINT("[line:%d, error:%d] %s\n\r", __LINE__, status,
                       SL_SOCKET_ERROR);
            sl_Close(sock);
            return(-1);
        }
        break;
    }

    i = 0;

    /////////////////////////////////////////////
    // send sine(i) forever to AP
    /////////////////////////////////////////////

    UART_PRINT("[nnaji msg] buflen: %i\n\r",buflen);

    while(1)
    {
        /*reads the accelerometer data in 8 bit resolution*/
        /*There are different API for reading out the accelerometer data in
         * 10,14,12 bit resolution*/
        if(BMA2x2_INIT_VALUE != bma2x2_read_accel_xyzt(&sample_xyzt))
        {
            UART_PRINT("Error reading from the accelerometer\n");
        }

        memset(custom_msg,0,strlen(custom_msg));
        sprintf(custom_msg,"%i,%.10d,%.10d,%.10d", i, sample_xyzt.x, sample_xyzt.y, sample_xyzt.z);

        /* Send packets to the server */
        status = sl_Send(sock, &custom_msg, buflen, 0);

        if((status == SL_ERROR_BSD_EAGAIN) && (TRUE == notBlocking))
        {
            sleep(1);
            continue;
        }
        else if(status < 0)
        {
            UART_PRINT("[line:%d, error:%d] %s\n\r", __LINE__, status,
                       SL_SOCKET_ERROR);
            break;
        }


        sent_bytes += status;
//        UART_PRINT("[nnaji] bytes sent to far (i=%i): %i\n\r", i, sent_bytes);
        i++;
    }

    UART_PRINT("Sent %u packets (%u bytes) successfully\n\r",
               i,
               sent_bytes);

    /* Calling 'close' with the socket descriptor,
     * once operation is finished. */
    status = sl_Close(sock);
    ASSERT_ON_ERROR(status, SL_SOCKET_ERROR);

    return(0);
}

int32_t time_drift_test(uint16_t sockPort)
{
    int32_t sock;
    int32_t status;
    SlSockAddr_t        *sa;
    int32_t addrSize;
    sockAddr_t sAddr;
    uint8_t notBlocking = 0;

    /* ALWAYS DECLARE ALL VARIABLES AT TOP OF FUNCTION TO AVOID BUFFER ISSUES */
    uint32_t buflen = MESSAGE_SIZE;
    struct timespec last_time;
    struct timespec cur_time;
    uint8_t rcv_msg_buff[buflen];
    uint8_t send_msg_buff[buflen];
    uint32_t cur_time_sec = 0;
    uint32_t cur_time_nsec = 0;
    uint8_t counting = 0;
    int32_t send_status;

    UART_PRINT("\n\rsockPort: %x\n\r",sockPort);

    /* filling the TCP server socket address */
    sAddr.in4.sin_family = SL_AF_INET;

    /* Since this is the client's side,
     * we must know beforehand the IP address
     * and the port of the server wer'e trying to connect.
     */
    sAddr.in4.sin_port = sl_Htons((unsigned short)sockPort);
    sAddr.in4.sin_addr.s_addr = sl_Htonl((unsigned int)app_CB.CON_CB.GatewayIP);

    sa = (SlSockAddr_t*)&sAddr.in4;
    addrSize = sizeof(SlSockAddrIn6_t);

    /* Get socket descriptor - this would be the
     * socket descriptor for the TCP session.
     */
    sock = sl_Socket(sa->sa_family, SL_SOCK_STREAM, TCP_PROTOCOL_FLAGS);
    ASSERT_ON_ERROR(sock, SL_SOCKET_ERROR);

#ifdef SECURE_SOCKET

    SlDateTime_t dateTime;
    dateTime.tm_day = DEVICE_DATE;
    dateTime.tm_mon = DEVICE_MONTH;
    dateTime.tm_year = DEVICE_YEAR;

    sl_DeviceSet(SL_DEVICE_GENERAL, SL_DEVICE_GENERAL_DATE_TIME,
                 sizeof(SlDateTime_t), (uint8_t *)(&dateTime));

    /* Set the following to enable Server Authentication */
    sl_SetSockOpt(sock,SL_SOL_SOCKET,SL_SO_SECURE_FILES_CA_FILE_NAME,
                  ROOT_CA_CERT_FILE, strlen(
                      ROOT_CA_CERT_FILE));

#ifdef CLIENT_AUTHENTICATION
    /* Set the following to pass Client Authentication */
    sl_SetSockOpt(sock,SL_SOL_SOCKET,SL_SO_SECURE_FILES_PRIVATE_KEY_FILE_NAME,
                  PRIVATE_KEY_FILE, strlen(
                      PRIVATE_KEY_FILE));
    sl_SetSockOpt(sock,SL_SOL_SOCKET,SL_SO_SECURE_FILES_CERTIFICATE_FILE_NAME,
                  TRUSTED_CERT_CHAIN, strlen(
                      TRUSTED_CERT_CHAIN));
#endif
#endif

    status = -1;

    while(status < 0)
    {
        /* Calling 'sl_Connect' followed by server's
         * 'sl_Accept' would start session with
         * the TCP server. */
        status = sl_Connect(sock, sa, addrSize);
        if((status == SL_ERROR_BSD_EALREADY)&& (TRUE == notBlocking))
        {
            sleep(1);
            continue;
        }
        else if(status < 0)
        {
            UART_PRINT("[line:%d, error:%d] %s\n\r", __LINE__, status,
                       SL_SOCKET_ERROR);
            sl_Close(sock);
            return(-1);
        }
        break;
    }

    //////////////////////////////////////////////////////
    // wait to receive requests for system time from AP
    //////////////////////////////////////////////////////

    memset(rcv_msg_buff,0,strlen(rcv_msg_buff));
    memset(send_msg_buff,0,strlen(send_msg_buff));
    UART_PRINT("[nnaji msg] buflen: %i\n\r",buflen);

    clock_gettime(CLOCK_REALTIME,&last_time);

    while(1)
    {
        status = sl_Recv(sock, &rcv_msg_buff, buflen, 0);
        if((status == SL_ERROR_BSD_EAGAIN) && (TRUE == notBlocking))
        {
            sleep(1);
            continue;
        }
        else if(status < 0)
        {
            UART_PRINT("[line:%d, error:%d] %s\n\r", __LINE__, status,
                       SL_SOCKET_ERROR);
            break;
        }

        if(status > 0){
            UART_PRINT("msg from laptop: %s, strlen=%i\n\r", rcv_msg_buff, strlen(rcv_msg_buff));

            if(strcmp("get_time",rcv_msg_buff) == 0 && counting)
            {
                clock_gettime(CLOCK_REALTIME, &cur_time);

                cur_time_sec = cur_time.tv_sec - last_time.tv_sec;
                cur_time_nsec = cur_time.tv_nsec - last_time.tv_nsec;

                sprintf(send_msg_buff,"time_sec=%u,time_nsec=%u",cur_time_sec,cur_time_nsec);

                send_status = sl_Send(sock, &send_msg_buff, buflen, 0);

                if(send_status < 0)
                {
                    UART_PRINT("[line:%d, error:%d] %s\n\r", __LINE__, send_status,
                               SL_SOCKET_ERROR);
                    break;
                }
            }
            else if(strcmp("start_counting",rcv_msg_buff) == 0)
            {
                clock_gettime(CLOCK_REALTIME,&last_time);

                UART_PRINT("[nnaji start_counting] time since clock initialization: sec=%u, nsec=%u\n\r",
                           last_time.tv_sec, last_time.tv_nsec);

                counting = 1;

                strcpy(send_msg_buff,"I am starting to count");

                send_status = sl_Send(sock, &send_msg_buff, buflen, 0);

                if(send_status < 0)
                {
                    UART_PRINT("[line:%d, error:%d] %s\n\r", __LINE__, send_status,
                               SL_SOCKET_ERROR);
                    break;
                }
            }
            else
            {
                strcpy(send_msg_buff,"don't know what to do with this msg");

                send_status = sl_Send(sock, &send_msg_buff, buflen, 0);

                if(send_status < 0)
                {
                    UART_PRINT("[line:%d, error:%d] %s\n\r", __LINE__, send_status,
                               SL_SOCKET_ERROR);
                    break;
                }
            }
        }

        memset(rcv_msg_buff,0,strlen(rcv_msg_buff));
        memset(send_msg_buff,0,strlen(send_msg_buff));
    }

    /* Calling 'close' with the socket descriptor,
     * once operation is finished. */
    status = sl_Close(sock);
    ASSERT_ON_ERROR(status, SL_SOCKET_ERROR);

    return(0);
}

int32_t tx_accelerometer(uint16_t sockPort)
{
    /* ALWAYS DECLARE ALL VARIABLES AT TOP OF FUNCTION TO AVOID BUFFER ISSUES */
    uint32_t buflen = MESSAGE_SIZE;
    uint32_t channel = 11;
    _i16 cur_channel;
    _i16 numBytes;
    _i16 status;
    uint8_t Rx_frame[MAX_RX_PACKET_SIZE];
    uint32_t i=0;
    uint32_t j=0;
    _u32 nonBlocking = 1;
    uint32_t timestamp = 0;
    uint16_t beacInterval;
    uint32_t last_ts = 0;
    frameInfo_t frameInfo;
    _i16 beaconRxSock;
    queue_t q;
    int32_t reading[MAX_ELEM_ARR_SIZE];
    struct timespec cur_time;

    /* for on-board accelerometer */
    /* structure to read the accelerometer data*/
    /* the resolution is in 8 bit*/
    struct bma2x2_accel_data_temp sample_xyzt;

    uint8_t createFilterArgs1[] = " -f FRAME_TYPE -v management -e not_equals -a drop -m L1";
    uint8_t createFilterArgs2[] = " -f S_MAC -v 72:BC:10:69:9C:B6 -e not_equals -a drop -m L1";
//    uint8_t createFilterArgs3[] = " -f S_MAC -v 72:BC:10:69:9C:B6 -e equals -a event -m L1";
//    uint8_t createFilterArgs2[] = " -f S_MAC -v 58:00:e3:43:6b:63 -e not_equals -a drop -m L1";
//    uint8_t createFilterArgs3[] = " -f S_MAC -v 58:00:e3:43:6b:63 -e equals -a event -m L1";
    uint8_t enableFilterArgs[] = "";

    status = cmdCreateFilterCallback(createFilterArgs1);
    ASSERT_ON_ERROR(status, WLAN_ERROR);

    status = cmdCreateFilterCallback(createFilterArgs2);
    ASSERT_ON_ERROR(status, WLAN_ERROR);

//    status = cmdCreateFilterCallback(createFilterArgs3);
//    ASSERT_ON_ERROR(status, WLAN_ERROR);

    status = cmdEnableFilterCallback(enableFilterArgs);
    ASSERT_ON_ERROR(status, WLAN_ERROR);

    memset(Rx_frame, 0, MAX_RX_PACKET_SIZE);

    UART_PRINT("[nnaji msg] buflen: %i\n\r", buflen);

    /* To use transceiver mode, the device must be set in STA role, be disconnected, and have disabled
        previous connection policies that might try to automatically connect to an AP. */
    status = sl_WlanPolicySet(SL_WLAN_POLICY_CONNECTION, SL_WLAN_CONNECTION_POLICY(0, 0, 0, 0), NULL, 0);
    if( status )
    {
        UART_PRINT("[line:%d, error:%d]\n\r", __LINE__, status);
        return(-1);
    }

    beaconRxSock = sl_Socket(SL_AF_RF, SL_SOCK_DGRAM, channel);
    ASSERT_ON_ERROR(beaconRxSock, SL_SOCKET_ERROR);

    status = sl_SetSockOpt(beaconRxSock, SL_SOL_SOCKET, SL_SO_NONBLOCKING, &nonBlocking, sizeof(nonBlocking));
    if(status < 0)
    {
        UART_PRINT("[line:%d, error:%d] %s\n\r", __LINE__, status,
                   SL_SOCKET_ERROR);
        sl_Close(beaconRxSock);
        return(-1);
    }

    initQueue(&q);
    while(1)
    {
        numBytes = sl_Recv(beaconRxSock, &Rx_frame, MAX_RX_PACKET_SIZE, 0);
        if(numBytes != SL_ERROR_BSD_EAGAIN)
        {
            if(numBytes < 0)
            {
                UART_PRINT("[line:%d, error:%d] %s\n\r", __LINE__, numBytes,
                           SL_SOCKET_ERROR);
                break;
            }

            parse_beacon_frame(Rx_frame, &frameInfo, 0);
            last_ts = frameInfo.timestamp;
        }

        reading[0] = last_ts;

        clock_gettime(CLOCK_REALTIME, &cur_time);
        reading[1] = (int32_t) (cur_time.tv_sec * 1000 + cur_time.tv_nsec / 1000000);

        /*reads the accelerometer data in 8 bit resolution*/
        /*There are different API for reading out the accelerometer data in
         * 10,14,12 bit resolution*/
        if(BMA2x2_INIT_VALUE != bma2x2_read_accel_xyzt(&sample_xyzt))
        {
            UART_PRINT("Error reading from the accelerometer\n\r");
        }

        reading[2] = (int32_t) sample_xyzt.x;
        reading[3] = (int32_t) sample_xyzt.y;
        reading[4] = (int32_t) sample_xyzt.z;

        if(qFull(&q))
            deque(&q);
        enque(&q, reading);

//        UART_PRINT("most recent reading: {%u, %u, %i, %i, %i}\n\r", reading[0], reading[1], reading[2], reading[3], reading[4]);
//        UART_PRINT("q contents:\n\r");
//        for(i=0;i<q.size;i++)
//            UART_PRINT("%i: {%u, %u, %i, %i, %i}\n\r", i, q.arr[qIndex(&q,i)][0], q.arr[qIndex(&q,i)][1],
//                       q.arr[qIndex(&q,i)][2], q.arr[qIndex(&q,i)][3], q.arr[qIndex(&q,i)][4]);

        memset(Tx_data, 0, MAX_TX_PACKET_SIZE);
        q_to_string(&q, Tx_data);
        UART_PRINT("%s\n\rlength: %i\n\r", Tx_data, strlen(Tx_data));

        UART_PRINT("\n\r");
        sleep(2);
    }

    /* Calling 'close' with the socket descriptor,
    * once operation is finished. */
    status = sl_Close(beaconRxSock);
    ASSERT_ON_ERROR(status, SL_SOCKET_ERROR);

    return(0);
}

int32_t test_time_beac_sync()
{
    /* ALWAYS DECLARE ALL VARIABLES AT TOP OF FUNCTION TO AVOID BUFFER ISSUES */
    int16_t cur_channel;
    int16_t numBytes;
    int16_t status;
    int32_t sent_bytes;
    int32_t bytes_to_send;
    int32_t buflen;
    uint32_t i=0;
    uint32_t j=0;
    uint32_t nonBlocking = 1;
    uint32_t timestamp = 0;
    uint16_t beacInterval;
    frameInfo_t frameInfo;
    int32_t beaconRxSock;
    int32_t backup;
    queue_t q;
//    int32_t reading[MAX_ELEM_ARR_SIZE];
    struct timespec cur_time;
    uint32_t last_beac_ts = 0;
    int32_t counter = 0;
    uint8_t Rx_frame[MAX_RX_PACKET_SIZE];
    uint32_t send_beac_ts = 0;
    uint32_t send_interval = 10000;

    sockAddr_t sAddr;
    uint16_t entry_port = ENTRY_PORT;
    SlSockAddr_t * sa;
    int32_t tcp_sock;
    int32_t addrSize;

//    int32_t front, back, size, capacity;
//    int32_t arr[OUR_MAX_QUEUE_SIZE][MAX_ELEM_ARR_SIZE];

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

    beaconRxSock = enter_tranceiver_mode(1);
    UART_PRINT("beaconRxSock set to %i in test_time_beac_sync()\n\r", beaconRxSock);

//    initQueue(&q);
    while(1)
    {
        backup = beaconRxSock;
        numBytes = sl_Recv(beaconRxSock, &Rx_frame, MAX_RX_PACKET_SIZE, 0);
        if(numBytes != SL_ERROR_BSD_EAGAIN)
        {
            if(numBytes == SL_ERROR_BSD_INEXE){
                UART_PRINT("[line:%d, error:%d] %s, sleeping for 5 secs\n\r", __LINE__, numBytes,
                                           SL_SOCKET_ERROR);
                sleep(5);
                continue;
            }
            else if(numBytes < 0)
            {
                UART_PRINT("[line:%d, error:%d] %s\n\r", __LINE__, numBytes,
                           SL_SOCKET_ERROR);
//                break;
                continue;
            }

            parse_beacon_frame(Rx_frame, &frameInfo, 0);
        }
        else
            continue;

        if(last_beac_ts == frameInfo.timestamp)
            continue;

        if(send_beac_ts==0)
        {
            UART_PRINT("send_beac_ts = %u + (%u - (%u %% %u))\n\r", send_interval,frameInfo.timestamp/1000,frameInfo.timestamp/1000,
                       send_interval);
            UART_PRINT("%u\n\r", send_beac_ts);
            send_beac_ts = send_interval + (frameInfo.timestamp/1000 - (frameInfo.timestamp/1000 % send_interval));
            UART_PRINT("%u\n\r", send_beac_ts);
        }

//        reading[0] = (int32_t) frameInfo.timestamp;

        clock_gettime(CLOCK_REALTIME, &cur_time);
//        reading[1] = (int32_t) (cur_time.tv_sec * 1000 + cur_time.tv_nsec / 1000000);

//        reading[2] = 0;
//        reading[3] = 0;
//        reading[4] = 0;

//        if(qFull(&q))
//        {
//            UART_PRINT("start deque\n\r");
//            deque(&q);
//            UART_PRINT("end deque\n\r");
//        }
//        UART_PRINT("start enque\n\r");
//        enque(&q, reading);
//        UART_PRINT("end enque\n\r");

//        UART_PRINT("most recent reading: {%u, %u, %i, %i, %i}\n\r", reading[0], reading[1], reading[2], reading[3], reading[4]);
//        UART_PRINT("q contents:\n\r");
//        for(i=0;i<q.size;i++)
//            UART_PRINT("%i: {%u, %u, %i, %i, %i}\n\r", i, q.arr[qIndex(&q,i)][0], q.arr[qIndex(&q,i)][1],
//                       q.arr[qIndex(&q,i)][2], q.arr[qIndex(&q,i)][3], q.arr[qIndex(&q,i)][4]);

//        status = sl_WlanDisconnect();
//        ASSERT_ON_ERROR(status, WLAN_ERROR);

//        if(counter / 5000.0f > 1)
        if(frameInfo.timestamp/1000 >= send_beac_ts)
        {
            // stop transeiver mode
            // connnect to accept point
            // connect to tcp socket
            // send data
            // re-enable tranceiver mode

            UART_PRINT("about to close socket %i\n\r",beaconRxSock);
            status = sl_Close(beaconRxSock);
            ASSERT_ON_ERROR(status, SL_SOCKET_ERROR);
            UART_PRINT("closing socket %i\n\r",beaconRxSock);

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
            UART_PRINT("sa->sa_family: %u\n\r", sa->sa_family);
            tcp_sock = sl_Socket(sa->sa_family, SL_SOCK_STREAM, TCP_PROTOCOL_FLAGS);
            ASSERT_ON_ERROR(tcp_sock, SL_SOCKET_ERROR);
            UART_PRINT("created socket %i\n\r",tcp_sock);

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
                    return(-1);
                }
                break;
            }

//            memset(Tx_data, 0, MAX_TX_PACKET_SIZE);
//            q_to_string(&q, Tx_data);

            sprintf(Tx_data, "%u %u", frameInfo.timestamp/1000, cur_time.tv_sec * 1000 + cur_time.tv_nsec / 1000000);

//            sent_bytes = 0;
////            bytes_to_send = strlen(Tx_data);
//            bytes_to_send = MAX_TX_PACKET_SIZE;
//            while(sent_bytes < bytes_to_send)
//            {
//                if(bytes_to_send - sent_bytes >= bytes_to_send)
//                {
//                    buflen = bytes_to_send;
//                }
//                else
//                {
//                    buflen = bytes_to_send - sent_bytes;
//                }
//
//                status = sl_Send(tcp_sock, &Tx_data, buflen, 0);
//                if(status < 0)
//                {
//                    UART_PRINT("[line:%d, error:%d] %s\n\r", __LINE__, status,
//                               SL_SOCKET_ERROR);
//                    break;
//                }
//                sent_bytes += status;
//            }
            status = sl_Send(tcp_sock, &Tx_data, MAX_TX_PACKET_SIZE, 0);
            if(status < 0)
            {
                UART_PRINT("[line:%d, error:%d] %s\n\r", __LINE__, status,
                           SL_SOCKET_ERROR);
                break;
            }
            UART_PRINT("sent %i bytes of data\n\r", status);

            UART_PRINT("closing socket %i\n\r", tcp_sock);
            status = sl_Close(tcp_sock);
            ASSERT_ON_ERROR(status, SL_SOCKET_ERROR);

            /* After calling sl_WlanDisconnect(),
             *    we expect WLAN disconnect asynchronous event.
             * Cleaning the former connection information from
             * the application control block
             * is handled in that event handler,
             * as well as getting the disconnect reason.
             */
//            sleep(2);
            status = sl_WlanDisconnect();
            ASSERT_ON_ERROR(status, WLAN_ERROR);

            while(IS_CONNECTED(app_CB.Status));
            UART_PRINT("disconnected form AP and done sending time sync data and disconnected from AP"
                    ", will re-enter transceiver mode in a few seconds\n\r");

            sleep(2);
            beaconRxSock = enter_tranceiver_mode(0);
            UART_PRINT("beaconRxSock set to %i in test_time_beac_sync()\n\r", beaconRxSock);

            send_beac_ts += send_interval;
            UART_PRINT("next timestamp to send data at: %u\n\r", send_beac_ts);
        }
        else
        {
            UART_PRINT("%u\n\r", frameInfo.timestamp/1000);
            UART_PRINT("beaconRxSock: %i\n\r", beaconRxSock);
            UART_PRINT("backup: %i\n\r", backup);
            UART_PRINT("sa->sa_family: %u\n\r", sa->sa_family);

            if(backup!=beaconRxSock)
                beaconRxSock = backup;
        }
//        else
//        {
//            if(counter % 10000 == 0)
//            {
//                UART_PRINT("%u %u\n\r", frameInfo.timestamp,last_beac_ts);
//                UART_PRINT("%u\n\r", (frameInfo.timestamp-last_beac_ts)/1000);
//                UART_PRINT("%i %f\n\r\n\r", counter, counter / 60000.0f);
//            }
//        }

//        if(last_beac_ts!=-1)
//        {
//            counter += (frameInfo.timestamp - last_beac_ts)/1000;
//        }
        last_beac_ts = frameInfo.timestamp;
    }

    /* Calling 'close' with the socket descriptor,
    * once operation is finished. */
    status = sl_Close(beaconRxSock);
    ASSERT_ON_ERROR(status, SL_SOCKET_ERROR);

    return(0);
}

int32_t time_drift_test_l2(){
    /* ALWAYS DECLARE ALL VARIABLES AT TOP OF FUNCTION TO AVOID BUFFER ISSUES */
    uint32_t buflen = MESSAGE_SIZE;
    uint8_t rcv_msg_buff[buflen];
    uint32_t channel = 11;
    uint32_t max_packet_size = 1544;
    _i16 channels[11] = {1,2,3,4,5,6,7,8,9,10,11};
    _i16 cur_channel;
    _i16 numBytes;
    _i16 status;
    struct SlTimeval_t timeVal;
    uint8_t Rx_frame[max_packet_size];
    uint32_t i=0;
    uint32_t j=0;
    uint32_t k=0;
    _u32 nonBlocking = 1;
    int32_t hdrOfs = 8;   // proprietary header offset
    uint32_t timestamp = 0;
    uint16_t beacInterval;
    int32_t last_ts = -1;
    int32_t numBeacons = 100;
    uint16_t expectedInterval = 0;
    int32_t time_diffs[numBeacons];
    frameInfo_t frameInfo;
    int32_t misses = 0;
    _i16 beaconRxSock;

//    uint8_t createFilterArgs1[] = " -f FRAME_TYPE -v management -e not_equals -a drop -m L1";
    uint8_t createFilterArgs1[] = " -f FRAME_TYPE -v management -e not_equals -a event -m L1";
//    uint8_t createFilterArgs2[] = " -f S_MAC -v 72:BC:10:69:9C:B6 -e not_equals -a drop -m L1";
//    uint8_t createFilterArgs3[] = " -f S_MAC -v 72:BC:10:69:9C:B6 -e equals -a event -m L1";
    uint8_t createFilterArgs2[] = " -f S_MAC -v 58:00:e3:43:6b:63 -e not_equals -a drop -m L1";
//    uint8_t createFilterArgs3[] = " -f S_MAC -v 58:00:e3:43:6b:63 -e equals -a event -m L1";
    uint8_t enableFilterArgs[] = "";

    status = cmdCreateFilterCallback(createFilterArgs2);
    ASSERT_ON_ERROR(status, WLAN_ERROR);

    status = cmdCreateFilterCallback(createFilterArgs1);
    ASSERT_ON_ERROR(status, WLAN_ERROR);

//    status = cmdCreateFilterCallback(createFilterArgs2);
//    ASSERT_ON_ERROR(status, WLAN_ERROR);

//    status = cmdCreateFilterCallback(createFilterArgs3);
//    ASSERT_ON_ERROR(status, WLAN_ERROR);

    status = cmdEnableFilterCallback(enableFilterArgs);
    ASSERT_ON_ERROR(status, WLAN_ERROR);

    timeVal.tv_sec = 2;             // Seconds
    timeVal.tv_usec = 0;             // Microseconds. 10000 microseconds resolution

//    memset(rcv_msg_buff, 0, strlen(rcv_msg_buff));
    memset(Rx_frame, 0, max_packet_size);

    UART_PRINT("[nnaji msg] buflen: %i\n\r", buflen);

    /* To use transceiver mode, the device must be set in STA role, be disconnected, and have disabled
        previous connection policies that might try to automatically connect to an AP. */
    status = sl_WlanPolicySet(SL_WLAN_POLICY_CONNECTION, SL_WLAN_CONNECTION_POLICY(0, 0, 0, 0), NULL, 0);
    if( status )
    {
        UART_PRINT("[line:%d, error:%d]\n\r", __LINE__, status);
        return(-1);
    }
//    status = sl_WlanDisconnect();
//    if( status )
//    {
//        UART_PRINT("[line:%d, error:%d]\n\r", __LINE__, status);
//        return(-1);
//    }

    beaconRxSock = sl_Socket(SL_AF_RF, SL_SOCK_DGRAM, channel);
//    beaconRxSock = sl_Socket(SL_AF_RF, SL_SOCK_RAW, channel);
    ASSERT_ON_ERROR(beaconRxSock, SL_SOCKET_ERROR);

//    status = sl_SetSockOpt(beaconRxSock, SL_SOL_SOCKET, SL_SO_RCVTIMEO, (_u8 *)&timeVal, sizeof(timeVal));    // Enable receive timeout
//    if(status < 0)
//    {
//        UART_PRINT("[line:%d, error:%d] %s\n\r", __LINE__, status,
//                   SL_SOCKET_ERROR);
//        sl_Close(beaconRxSock);
//        return(-1);
//    }

    status = sl_SetSockOpt(beaconRxSock, SL_SOL_SOCKET, SL_SO_NONBLOCKING, &nonBlocking, sizeof(nonBlocking));
    if(status < 0)
    {
        UART_PRINT("[line:%d, error:%d] %s\n\r", __LINE__, status,
                   SL_SOCKET_ERROR);
        sl_Close(beaconRxSock);
        return(-1);
    }

    while(1)
//    while(i < numBeacons)
    {
        for(i=0;i<11;i++)
        {
            cur_channel = channels[i];
            status = sl_SetSockOpt(beaconRxSock, SL_SOL_SOCKET, SL_SO_CHANGE_CHANNEL, &cur_channel, sizeof(cur_channel));
            if(status < 0)
            {
                UART_PRINT("[line:%d, error:%d] %s, channel: %i\n\r", __LINE__, status,
                           SL_SOCKET_ERROR, cur_channel);

                if(numBytes != SL_ERROR_BSD_EAGAIN)
                    break;
            }

            numBytes = sl_Recv(beaconRxSock, &Rx_frame, max_packet_size, 0);
            if(numBytes != SL_ERROR_BSD_EAGAIN)
            {
                UART_PRINT("[nnaji msg] numBytes from channel %i: %i\n\r", cur_channel, numBytes);
                if(numBytes < 0)
                {
                    UART_PRINT("[line:%d, error:%d] %s, channel: %i\n\r", __LINE__, numBytes,
                               SL_SOCKET_ERROR, cur_channel);

                    break;
                }

                UART_PRINT("first 50 bytes: ");
                for(j=0;j<50;j++){
                    UART_PRINT("%x ", Rx_frame[j]);
                }
                UART_PRINT("\n\r");
            }

//            memset(rcv_msg_buff,0,strlen(rcv_msg_buff));
//            memset(Rx_frame, 0, max_packet_size);
        }

//        numBytes = sl_Recv(beaconRxSock, &Rx_frame, max_packet_size, 0);
//        if(numBytes != SL_ERROR_BSD_EAGAIN)
//        {
//            if(numBytes < 0)
//            {
//                UART_PRINT("[line:%d, error:%d] %s\n\r", __LINE__, numBytes,
//                           SL_SOCKET_ERROR);
//                break;
//            }
//
//            parse_beacon_frame(Rx_frame, &frameInfo, 0);
//            time_diffs[i] = (frameInfo.timestamp - timestamp)/1000;
//            i++;
//            timestamp = frameInfo.timestamp;
//
//            if(expectedInterval == 0)
//            {
//                expectedInterval = frameInfo.beaconIntervalMs / 1000;
//            }
//        }

    }

//    for(i=0;i<numBeacons;i++)
//    {
//        if(time_diffs[i] > expectedInterval && i != 0)
//        {
//            misses++;
//        }
//        UART_PRINT("time diff %i: %i\n\r", i, time_diffs[i]);
//    }
//    UART_PRINT("missed beacons: %i\n\r", misses);

    /* Calling 'close' with the socket descriptor,
    * once operation is finished. */
    status = sl_Close(beaconRxSock);
    ASSERT_ON_ERROR(status, SL_SOCKET_ERROR);

    return(0);
}

int32_t time_drift_test_l3(uint16_t sockPort){
    /* ALWAYS DECLARE ALL VARIABLES AT TOP OF FUNCTION TO AVOID BUFFER ISSUES */
    uint32_t buflen = MESSAGE_SIZE;
    uint8_t rcv_msg_buff[buflen];
    _i16 sock;
    uint32_t max_packet_size = 1544;
    _i16 numBytes;
    _i16 status;
    struct SlTimeval_t timeVal;
    uint8_t Rx_frame[max_packet_size];
    _u16 broadcast_port = 10012;
    SlSockAddrIn_t  sAddr;
    _i16 AddrSize = sizeof(SlSockAddrIn_t);
    int32_t i;

    timeVal.tv_sec =  30;             // Seconds
    timeVal.tv_usec = 0;             // Microseconds. 10000 microseconds resolution

    memset(rcv_msg_buff, 0, strlen(rcv_msg_buff));
    memset(Rx_frame, 0, max_packet_size);

    UART_PRINT("[nnaji msg] sockPort (for transmitting data to AP): %i\n\r", sockPort);
    UART_PRINT("[nnaji msg] buflen: %i\n\r", buflen);

    sock = sl_Socket(SL_AF_INET, SL_SOCK_DGRAM, 0);
    ASSERT_ON_ERROR(sock, SL_SOCKET_ERROR);

//    status = sl_SetSockOpt(sock, SL_SOL_SOCKET, SL_SO_RCVTIMEO, (_u8 *)&timeVal, sizeof(timeVal));    // Enable receive timeout
//    if(status < 0)
//    {
//        UART_PRINT("[line:%d, error:%d] %s\n\r", __LINE__, status,
//                   SL_SOCKET_ERROR);
//        sl_Close(sock);
//        return(-1);
//    }

    sAddr.sin_family = SL_AF_INET;
    sAddr.sin_port = sl_Htons(broadcast_port);
    sAddr.sin_addr.s_addr = SL_INADDR_ANY;

    status = sl_Bind(sock, (SlSockAddr_t *)&sAddr, AddrSize);
    if(status < 0)
    {
        UART_PRINT("[line:%d, error:%d] %s\n\r", __LINE__, status,
                   SL_SOCKET_ERROR);
        sl_Close(sock);
        return(-1);
    }

    while(1)
    {
        numBytes = sl_RecvFrom(sock, Rx_frame, max_packet_size, 0, (SlSockAddr_t *)&sAddr, (SlSocklen_t*)&AddrSize);
        UART_PRINT("[nnaji msg] numBytes: %i\n\r", numBytes);
        if(numBytes < 0)
        {
            UART_PRINT("[line:%d, error:%d] %s\n\r", __LINE__, numBytes,
                       SL_SOCKET_ERROR);
            break;
        }

        UART_PRINT("first 50 bytes: ");
        for(i=0;i<50;i++){
            UART_PRINT("%x ", Rx_frame[i]);
        }
        UART_PRINT("\n\r");

        UART_PRINT("msg as str: %s\n\r", Rx_frame);

        sleep(1);
    }

    /* Calling 'close' with the socket descriptor,
    * once operation is finished. */
    status = sl_Close(sock);
    ASSERT_ON_ERROR(status, SL_SOCKET_ERROR);

    return(0);
}

int32_t parse_beacon_frame(uint8_t * Rx_frame, frameInfo_t * frameInfo, uint8_t printInfo){
    int32_t hdrOfs = 8;   // proprietary header offset
    uint32_t timestamp = 0;
    int32_t i = 31;     // last byte index of timestamp in beacon frame
    int32_t j;

    frameInfo->frameControl = Rx_frame[hdrOfs+1] | (Rx_frame[hdrOfs] << 8);
    frameInfo->duration = Rx_frame[hdrOfs+3] | (Rx_frame[hdrOfs+2] << 8);
    memcpy(frameInfo->destAddr,&Rx_frame[hdrOfs+4], 6);
    memcpy(frameInfo->sourceAddr,&Rx_frame[hdrOfs+10], 6);
    memcpy(frameInfo->bssid,&Rx_frame[hdrOfs+16], 6);
    frameInfo->seqCtrl = Rx_frame[hdrOfs+23] | (Rx_frame[hdrOfs+22] << 8);

    // can only work with 4 bytes out of the 8 byte timestamp value since this is a 32-bit machine
    while(Rx_frame[hdrOfs+i] == 0 && i > 28)
        i--;
    for(j=0;i-4+j<=i;j++)
        timestamp |= Rx_frame[hdrOfs+ i - 4 + j] << (j*8);

    frameInfo->timestamp = timestamp;
    frameInfo->beaconInterval = Rx_frame[hdrOfs+32] | (Rx_frame[hdrOfs+33] << 8); // remember beacon interval is backwards
    frameInfo->beaconIntervalMs = frameInfo->beaconInterval * 1024;
    frameInfo->capabilityInfo = Rx_frame[hdrOfs+35] | (Rx_frame[hdrOfs+34] << 8);
    frameInfo->ssidElemId = Rx_frame[hdrOfs+36];
    frameInfo->ssidLen = Rx_frame[hdrOfs+37];

    for(j=0;j<frameInfo->ssidLen;j++)
        frameInfo->ssid[j] = Rx_frame[hdrOfs+38+j];
    frameInfo->ssid[frameInfo->ssidLen + 1] = '\0';

    if(printInfo)
    {
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

    return 0;
}

int32_t q_to_string(queue_t * q, uint8_t * buf)
{
    int32_t i = 0;
    int32_t q_i;
    int32_t buf_i = 0;
    uint8_t reading[61]; // max reading to string length is 60: "(4294967296, 4294967296, 4294967296, 4294967296, 4294967296)"

    for(i=0;i<q->size;i++)
    {
        q_i = qIndex(q, i);
        sprintf(reading, "%u,%u,%i,%i,%i|", q->arr[q_i][0], q->arr[q_i][1], q->arr[q_i][2], q->arr[q_i][3], q->arr[q_i][4]);
        strcpy(&buf[buf_i], reading);
        buf_i += strlen(reading); // +2 for comma and space
    }

    return 0;
}

int32_t enter_tranceiver_mode(int32_t initialize)
{
    /* ALWAYS DECLARE ALL VARIABLES AT TOP OF FUNCTION TO AVOID BUFFER ISSUES */
    uint32_t channel = BEACON_CHANNEL;
    int16_t status;
    uint32_t nonBlocking = 1;
    int32_t Tx_sock;

    if(initialize)
    {
        uint8_t createFilterArgs1[] = " -f FRAME_TYPE -v management -e not_equals -a drop -m L1";
        uint8_t createFilterArgs2[] = MAC_FILTER_ARGS;
        uint8_t enableFilterArgs[] = "";

        /* To use transceiver mode, the device must be set in STA role, be disconnected, and have disabled
            previous connection policies that might try to automatically connect to an AP. */
        status = sl_WlanPolicySet(SL_WLAN_POLICY_CONNECTION, SL_WLAN_CONNECTION_POLICY(0, 0, 0, 0), NULL, 0);
        if( status )
        {
            UART_PRINT("[line:%d, error:%d]\n\r", __LINE__, status);
            return(-1);
        }

        status = cmdCreateFilterCallback(createFilterArgs1);
        ASSERT_ON_ERROR(status, WLAN_ERROR);

        status = cmdCreateFilterCallback(MAC_FILTER_ARGS);
        ASSERT_ON_ERROR(status, WLAN_ERROR);

        status = cmdEnableFilterCallback(enableFilterArgs);
        ASSERT_ON_ERROR(status, WLAN_ERROR);
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

    UART_PRINT("created socket %i in enter_tranceiver_mode()\n\r", Tx_sock);
    return Tx_sock;
}
