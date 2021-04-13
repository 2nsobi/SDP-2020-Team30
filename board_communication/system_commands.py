import socket
import logging
import time
from board_communication.windows_ap import WindowsSoftAP
from board_communication.system_integration import assemble_data_for_plot
from board_communication.parse_and_plot import plot_tcp_data
from .Logger import Logger

AP_SSID = 'true_base_ap'
AP_KEY = 'sdp-2021'
ENTRY_PORT = 10000
DATA_WAIT_TIMEOUT = 7  # seconds

entry_socket = None
logger = Logger.get_instance()
base_station_ap = None
base_station_initialized = False


def initialize_base_station():
    global entry_socket, logger, base_station_ap, base_station_initialized

    if base_station_initialized:
        logger.warn('Base station already initialized')
        return

    base_station_ap = WindowsSoftAP(ssid=AP_SSID, key=AP_KEY)
    base_station_ap.start_ap()
    ip = base_station_ap.get_ipv4()
    if ip is None:
        logger.error('Error: server_ip is None')
        return
    else:
        print(f"Windows AP started with IP addr {ip}")

    entry_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)  # Create a TCP/IP socket
    entry_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    entry_socket.bind((ip, ENTRY_PORT))  # Bind the socket to the port
    entry_socket.listen(2)  # Listen for incoming connections
    entry_socket.settimeout(DATA_WAIT_TIMEOUT)

    base_station_initialized = True


def get_data_from_boards():
    global entry_socket, base_station_ap

    modules = set(['wrist', 'base'])
    readings = {"wrist": {}, "base": {}}

    base_station_ap.stop_ap()
    time.sleep(3.5)
    base_station_ap.start_ap()

    while len(modules) > 0:
        try:
            print(f'waiting for a connection from modules: {modules}')
            connection, client_address = entry_socket.accept()

            print(f'*** connection from {client_address} ***')
            data = connection.recv(10000000)
            for i in range(10):
                data += connection.recv(10000000)

            print(f'Data: {data}')
            name, data = assemble_data_for_plot(data)
            print(data.keys())
            readings[name] = data
            print(readings)

            modules.remove(name)
        except Exception as e:
            logger.error(f'Failed to get data back from both boards, error:\n{e}')
            return

    print("Contents of readings: ----------------------")
    for key in readings:
        print(key)
        print(readings[key].keys())
    print("-----------------------------------------")
    return plot_tcp_data(readings["wrist"], readings["base"], output=False)


if __name__ == '__main__':
    initialize_base_station()
    time.sleep(5)
    res = get_data_from_boards()
    if res is not None:
        x1, y1, x2, y2 = res

        print(len(x1), len(y1))
        print(len(x2), len(y2))
