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

/* custom header files */
#include "ap_connection.h"

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

typedef union
{
    SlSockAddrIn6_t in6;       /* Socket info for Ipv6 */
    SlSockAddrIn_t in4;        /* Socket info for Ipv4 */
}sockAddr_t;


int32_t connectToAP()
{
    int32_t ret = 0;
    ConnectCmd_t ConnectParams;

    /* Call the command parser */
    memset(&ConnectParams, 0x0, sizeof(ConnectCmd_t));


    uint8_t ssid[] = AP_SSID;
    uint8_t key[] = AP_KEY;
    SlWlanSecParams_t secParams = { .Type = SL_WLAN_SEC_TYPE_WPA_WPA2,
                                    .Key = (signed char *)key,
                                    .KeyLen = strlen((const char *)key) };

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


uint16_t get_port_for_data_tx()
{
    int32_t sock;
    int32_t status;
    uint32_t i = 0;
    int32_t nonBlocking;
    SlSockAddr_t        *sa;
    int32_t addrSize;
    sockAddr_t sAddr;
    uint16_t portNumber = ENTRY_PORT;
    uint8_t notBlocking = 0;

    int32_t buflen;
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
        if(bytes_to_send - sent_bytes >= BUF_LEN)
        {
            buflen = BUF_LEN;
        }
        else
        {
            buflen = bytes_to_send - sent_bytes;
        }

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
    UART_PRINT("[nnaji] port recieved: \"%x\"\n\r",receivedPort);

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
    uint32_t buflen = 50;
    uint8_t custom_msg[buflen];

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
    // send IP of this device to AP
    /////////////////////////////////////////////

    UART_PRINT("[nnaji msg] buflen: %i\n\r",buflen);

    while(1)
    {
        memset(custom_msg,0,strlen(custom_msg));
        sprintf(custom_msg,"i=%i, sin(i)=%.10f", i, sin((double)i));

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
        UART_PRINT("[nnaji] bytes sent to far (i=%i): %i\n\r", i, sent_bytes);

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
    uint32_t i = 0;
    SlSockAddr_t        *sa;
    int32_t addrSize;
    sockAddr_t sAddr;
    uint8_t notBlocking = 0;

    /* ALWAYS DECLARE ALL VARIABLES AT TOP OF FUNCTION TO AVOID BUFFER ISSUES */
    uint32_t buflen = 50;
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

    i = 0;

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
                clock_gettime(CLOCK_REALTIME,&cur_time);

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

