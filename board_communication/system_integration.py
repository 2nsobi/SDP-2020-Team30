import socket
import time
import os
from PyAccessPoint import pyaccesspoint
import datetime
import logging
import math
from board_communication.parse_and_plot import plot_tcp_data, plot_one_board_data

WINDOWS = True
ENTRY_PORT = 10000

#whether or not to save experiment
SAVE = False
LINUX_HOTSPOT = True

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
    formatted_date = str(date).replace(" ", "_").split(".")[0].replace(":", ";")
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
    access_point = pyaccesspoint.AccessPoint()
    access_point.start()

    if access_point.is_running():
        logger.info("Hostapd is running")
    else:
        logger.info("Hostapd failed to start")
        exit(-1)
    return access_point


def linux(plot = True, plot_1 = False):
    # plot: indicates whether or not to plot
    # plot_1: indicates whether or not to plot only 1 board's output (cannot be true if plot is false)
    global ENTRY_PORT

    readings = {"wrist":{}, "base":{}}

    try:
        if LINUX_HOTSPOT:
            server_ip = "10.42.0.1"
            print("Server running at {}".format(server_ip))
        else:
            ap = setup_ap()
            server_ip = ap.ip
        ENTRY_PORT = 10000

        entry_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)  # Create a TCP/IP socket
        entry_address = (server_ip, ENTRY_PORT)
        entry_socket.bind(entry_address)  # Bind the socket to the port
        entry_socket.listen(2)  # Listen for incoming connections
        last_time = time.time()


        for i in range(2):

            logger.info('waiting for a connection')
            connection, client_address = entry_socket.accept()

            logger.info(f'*** connection from {client_address} ***')
            data = connection.recv(100000)
            for i in range(10):
                data += connection.recv(10000000)

            logger.info(data)
            if plot:
                name, data = assemble_data_for_plot(data)
                readings[name] = data
                if plot_1:
                    plot_one_board_data(data)
                    return
            else:
                parse_mcu_msg(data, client_address)
    except Exception as e:
        logger.info(e)
        logger.info("Exiting")
        return

    plot_tcp_data(readings["wrist"], readings["base"])



def assemble_data_for_plot(data):
    # from wrist module, readings will be in the format:
    # "|<beacon_ts>,<local_ts>,<accel_x>,<accel_y>,<accel_z>|"
    # from base module, readings will be in the format:
    # "|<beacon_ts>,<local_ts>,<load_cell>|"
    # uid is a tuple (connected_ip_address, connected_socket)
    # returns a tuple ("wrist" or "base", dictionary (created below))

    base_data = False
    dict = {"adc":[], "local_ts": [], "beacon_ts": []}
    data = str(data)
    data = data.replace("b'", "")
    data = data.replace("'", "")
    data = data.split("|")
    try:
        for reading in data:
            reading = reading.split(",")
            if len(reading) < 2:
                continue
            beacon_ts = int(reading[0])
            local_ts = int(reading[1])
            if int(beacon_ts) == 3200171710 or int(local_ts) == 3200171710 or int(beacon_ts) == 0 or int(local_ts) == 0:
                continue
            if len(reading) == 3:  # from the base module
                load_cell = float(reading[2])
                dict["beacon_ts"].append(beacon_ts)
                dict["local_ts"].append(local_ts)
                dict["adc"].append(load_cell)
                base_data = True
            elif len(reading) == 5: #from the wrist module

                #base data should not be true, this means that we got an array of length 3 and an array of length 5
                if base_data:
                    raise Exception

                x = float(reading[2])
                y = float(reading[3])
                z = float(reading[4])
                accel = math.sqrt(math.pow(x, 2) + math.pow(y, 2) + math.pow(z, 2))
                dict["beacon_ts"].append(beacon_ts)
                dict["local_ts"].append(local_ts)
                dict["adc"].append(accel)
    except Exception as e:
        logger.info("Unexpected data format, exception:")
        logger.info(e)
        logger.info(reading)

    if len(dict["beacon_ts"]) == len(dict["local_ts"]) == len(dict["adc"]):
        name = ""
        if base_data:
            name = "base"
        else:
            name = "wrist"
        return name, dict
    else:
        print("Lengths of the arrays are unequal")
        exit(-1)




def parse_mcu_msg(data, uid):
    # from wrist module, readings will be in the format:
    # "|<beacon_ts>,<local_ts>,<accel_x>,<accel_y>,<accel_z>|"
    # from base module, readings will be in the format:
    # "|<beacon_ts>,<local_ts>,<load_cell>|"
    # uid is a tuple (connected_ip_address, connected_socket)

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
            if len(reading) == 3:   # from the base module
                load_cell = reading[3]
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



def run_experiment():
    setup()
    linux()


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
    if SAVE:
        cleanup()