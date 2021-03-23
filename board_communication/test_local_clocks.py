import socket
from windows_ap import WindowsSoftAP
import time
import os
from PyAccessPoint import pyaccesspoint
import datetime
import logging

WINDOWS = False
LINUX_HOTSPOT = True
ENTRY_PORT = 10000

# time of test in minutes
TEST_LEN = 2

ROOT_FOLDER = ""

def str_date():
    # returns the date as a string formatted as <year>-<month>-<day> hours:minutes:seconds
    a = str(datetime.datetime.now())
    return a


def setup():
    global ROOT_FOLDER, logger

    # if data folder does not exist, create it
    data_folder = os.path.join(os.getcwd(), "timestamp_data")
    if not os.path.exists(data_folder):
        os.mkdir(data_folder)

    # create experiment folder
    date = str_date()
    formatted_date = "2_boards_" + str(date).replace(" ", "_").split(".")[0].replace(":", ";")
    ROOT_FOLDER = os.path.join(os.getcwd(), "timestamp_data", formatted_date)
    os.mkdir(ROOT_FOLDER)

    # setup logging
    logger = logging.getLogger("experiment_log")
    logger.setLevel(logging.INFO)
    logfile = os.path.join(ROOT_FOLDER, "laptop.log")
    logging.basicConfig(filename=logfile, format='%(message)s')
    logging.getLogger("experiment_log").addHandler(logging.StreamHandler())


def create_files(uid):
    global ROOT_FOLDER
    # uid: client IP address
    ip_last3 = uid.split(".")[-1]
    filename = ip_last3 + ".txt"
    board_file = os.path.join(os.getcwd(), "timestamp_data", ROOT_FOLDER, filename)
    if not os.path.exists(board_file):
        f = open(board_file, 'a')
        f.write("beacon_timestamp,local_timestamp")
        f.close()
    return board_file


def setup_ap():
    access_point = pyaccesspoint.AccessPoint(use_custom_hostapd=False)
    access_point.start()

    if access_point.is_running():
        logger.info("Hostapd is running")
    else:
        logger.info("Hostapd failed to start")
        exit(-1)
    return access_point


def linux(end_time, linux_hotspot = LINUX_HOTSPOT):
    global ENTRY_PORT
    try:
        if linux_hotspot:
            server_ip = "10.42.0.1"
        else:
            ap = setup_ap()
            server_ip = ap.ip
        ENTRY_PORT = 10000


        entry_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)  # Create a TCP/IP socket
        entry_address = (server_ip, ENTRY_PORT)
        entry_socket.bind(entry_address)  # Bind the socket to the port
        entry_socket.listen(2)  # Listen for incoming connections
        last_time = time.time()

        while last_time < end_time:
            current_time = time.time()
            remaining = end_time - current_time
            hours_remaining = str(int(remaining / 3600)).zfill(2)
            mins_remaining = str(int(remaining / 60) % 60).zfill(2)
            secs_remaining = str(int(remaining % 60)).zfill(2)
            logger.info("{}:{}:{} remaining in the experiement".format(hours_remaining, mins_remaining, secs_remaining))
            logger.info('waiting for a connection')
            connection, client_address = entry_socket.accept()

            logger.info(f'*** connection from {client_address} ***')
            data = connection.recv(100000)

            logger.info(data)
            logger.info("It's been {} seconds since last connection".format(current_time - last_time))
            parse_mcu_msg(data, client_address)
            last_time = current_time
    except Exception as e:
        logger.info(e)
    finally:
        if not linux_hotspot:
            ap.stop()


def parse_mcu_msg(data, uid):
    ip_addr = uid[0]
    data = str(data)
    data = data.replace("b'", "")
    data = data.replace("'", "")
    data = data.split("|")
    try:
        for reading in data:
            reading = reading.split(",")
            if len(reading)<2:
                continue
            beacon_ts = reading[0]
            local_ts = reading[1]
            if int(beacon_ts) == 3200171710 or int (local_ts) == 3200171710:
                continue
            logger.info("Recieved from board {}:\n\tBeacon timestamp:\t{}\n\tLocal timestamp:\t{}".format(ip_addr, beacon_ts,
                                                                                                    local_ts))
            store_data(ip_addr, beacon_ts, local_ts)
    except Exception as e:
        logger.info("Unexpected data format, exception:")
        logger.info(e)


def store_data(uid, beacon_timestamp, local_time):
    board_file = create_files(uid)
    f = open(os.path.join(os.getcwd(), "timestamp_data", board_file), 'a')
    f.write('\n' + str(beacon_timestamp) + ',' + str(local_time))
    f.close()


def windows(end_time):
    global ENTRY_PORT
    ap = WindowsSoftAP()
    # ap.start_ap()
    server_ip = ap.get_ipv4()
    if server_ip is None:
        logger.info('server_ip is None')
        quit()
    else:
        logger.info("Windows AP started with IP addr {}".format(server_ip))

    entry_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)  # Create a TCP/IP socket
    entry_address = (server_ip, ENTRY_PORT)
    entry_socket.bind(entry_address)  # Bind the socket to the port
    entry_socket.listen(200)  # Listen for incoming connections
    last_time = time.time()

    while last_time < end_time:
        current_time = time.time()
        remaining = end_time - current_time
        hours_remaining = str(int(remaining / 3600)).zfill(2)
        mins_remaining = str(int(remaining/60)%60).zfill(2)
        secs_remaining = str(int(remaining % 60)).zfill(2)
        logger.info("{}:{}:{} remaining in the experiement".format(hours_remaining, mins_remaining, secs_remaining))
        logger.info('waiting for a connection')
        connection, client_address = entry_socket.accept()

        logger.info(f'*** connection from {client_address} ***')
        data = connection.recv(100000)

        logger.info(data)
        logger.info("It's been {} seconds since last connection".format(current_time - last_time))
        parse_mcu_msg(data, client_address)
        last_time = current_time


def run_experiment():
    setup()
    end_time = time.time() + (60 * TEST_LEN)
    if WINDOWS:
        windows(end_time)
    else:
        linux(end_time)


def cleanup():
    # deletes folder timestamp data and files inside it
    folder = os.path.join(os.getcwd(), "timestamp_data")
    files = os.listdir(folder)
    for file in files:
        filepath = os.path.join(folder, file)
        os.remove(filepath)
    os.remove(folder)

if __name__ == '__main__':
    run_experiment()
    #cleanup()