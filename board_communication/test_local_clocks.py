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

# whether or not to save data into text files
SAVE = True
#whether or not we are only saving beacon timestamps (for testing what percent of beacons we miss)
BEACON_TEST = True

ROOT_FOLDER = ""

def str_date():
    # returns the date as a string formatted as <year>-<month>-<day> hours:minutes:seconds
    a = str(datetime.datetime.now())
    return a


def setup():
    global ROOT_FOLDER, logger, BEACON_TEST

    # if data folder does not exist, create it
    if not BEACON_TEST:
        data_folder = os.path.join(os.getcwd(), "timestamp_data")
    else:
        data_folder = os.path.join(os.getcwd(), "beacon_recv_test")
    if not os.path.exists(data_folder):
        os.mkdir(data_folder)

    # create experiment folder
    date = str_date()
    formatted_date = str(date).replace(" ", "_").split(".")[0].replace(":", ";")
    ROOT_FOLDER = os.path.join(data_folder, formatted_date)
    os.mkdir(ROOT_FOLDER)

    # setup logging
    logger = logging.getLogger("experiment_log")
    logger.setLevel(logging.INFO)
    logfile = os.path.join(ROOT_FOLDER, "laptop.log")
    logging.basicConfig(filename=logfile, format='%(message)s')
    logging.getLogger("experiment_log").addHandler(logging.StreamHandler())


def create_files(uid, store_folder = None, load_cell = None, beacon_test = False):
    global ROOT_FOLDER
    # uid: client IP address
    ip_last3 = uid.split(".")[-1]
    filename = ip_last3 + ".txt"
    if store_folder is None:
        board_file = os.path.join(ROOT_FOLDER, filename)
    else:
        board_file = os.path.join(store_folder, filename)
    if not os.path.exists(board_file):
        f = open(board_file, 'a')
        if load_cell is not None:
            f.write("beacon_timestamp,local_timestamp,load_cell_reading")
        if beacon_test:
            f.write("beacon_timestamp")
        else:
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


def linux(end_time, linux_hotspot = LINUX_HOTSPOT, beacon_test = BEACON_TEST):
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
            data = connection.recv(1460)
            for i in range(100):
                data += connection.recv(1460)

            logger.info(data)
            logger.info("It's been {} seconds since last connection".format(current_time - last_time))
            parse_mcu_msg(data, client_address, beacon_test)
            last_time = current_time
    except Exception as e:
        logger.info(e)
    finally:
        if not linux_hotspot:
            ap.stop()


def parse_mcu_msg(data, uid, beacon_test = False, store_folder = None):
    # store_folder is an option used to recreate the board txt files from an experiment, no printing will occur
    ip_addr = uid[0]
    data = str(data)
    data = data.replace("b'", "")
    data = data.replace("'", "")
    data = data.split("|")
    logger.info("Length of data recieved:\t{}".format(len(data)))
    try:
        for reading in data:
            load_cell_reading = None
            local_ts = None
            reading = reading.split(",")

            elements = len(reading)

            if elements<1:
                continue
            if elements >= 1:
                beacon_ts = reading[0]
                if int(beacon_ts) == 3200171710:
                    continue
            if elements >=2:
                local_ts = reading[1]
                if int (local_ts) == 3200171710:
                    continue
            #check length of readings
            if elements == 3:
                load_cell_reading = reading[2]
            if store_folder is None:
                logger.info("Recieved from board {}:\n\tBeacon timestamp:\t{}\n\tLocal timestamp:\t{}".format(ip_addr,
                                                                                                              beacon_ts,
                                                                                                    local_ts))
            store_data(ip_addr, beacon_ts, local_ts = local_ts, load_cell = load_cell_reading, store_folder = store_folder)
    except Exception as e:
        if store_folder is None:
            logger.info("Unexpected data format, exception:")
            logger.info(e)
        else:
            print("unexpected data format")


def store_data(uid, beacon_timestamp, local_ts = None, load_cell = None, store_folder = None):
    if local_ts is None:
        beacon_test = True
    else:
        beacon_test = False
    board_file = create_files(uid, store_folder = store_folder, load_cell = load_cell, beacon_test = beacon_test)
    f = open(os.path.join(os.getcwd(), "timestamp_data", board_file), 'a')
    if load_cell is not None and local_ts is not None:
        f.write('\n' + str(beacon_timestamp) + ',' + str(local_ts) + ',' + str(load_cell))
    if local_ts is not None:
        f.write('\n' + str(beacon_timestamp) + ',' + str(local_ts))
    else:
        f.write('\n' + str(beacon_timestamp))
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


def cleanup(folder):
    # deletes root folder and files inside it
    files = os.listdir(folder)
    for file in files:
        filepath = os.path.join(folder, file)
        os.remove(filepath)
    os.rmdir(folder)

if __name__ == '__main__':
    run_experiment()
    if not SAVE:
        cleanup(ROOT_FOLDER)