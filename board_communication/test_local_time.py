import time
from windows_ap import WindowsSoftAP
from server import Server
import time
from PyAccessPoint import pyaccesspoint
import socket
import os

# time of test in minutes
TEST_LEN = 0.1
# how often to sample in seconds
SAMPLE_RATE = 60

def create_files(uid):
    filename = uid.split(".")[-1]
    filename = filename + ".txt"
    board_file = os.path.join(os.getcwd(), "timestamp_data", filename)
    if not os.path.exists(board_file):
        f = open(board_file, 'a')
        f.write("beacon_timestamp,local_timestamp")
        f.close()
    return board_file


def setup_ap():

    access_point = pyaccesspoint.AccessPoint()
    access_point.start()

    if access_point.is_running():
        print("Hostapd is running")
    else:
        print("Hostapd failed to start")
        exit(-1)
    return access_point

def main():
    #setup AP and make connections
    ap = setup_ap()
    server_ip = ap.ip
    end_time = time.time() + (60 * TEST_LEN)
    while (time.time() < end_time):
        time.sleep(60)
        ap.stop()
        
    # Create and start server for communicating with boards
    server = Server(server_ip, board_count=2)

    # start_server() returns socket for server that is already listening for incoming connections
    server_entry_socket = server.start_server()

    ap.stop()
    end_time = time.time() + (60 * TEST_LEN)
    while(time.time() < end_time):
        retval = server.wait_for_all_boards_to_connect(task='test_local_drift')
        for i, board_ip in enumerate(server.boards):
            print(i, board_ip)
            print(f'[board {i}] response from board with ip {board_ip} '
                  f'(server: {server.boards[board_ip]["last_response_server"]}):'
                  f'\n{server.boards[board_ip]["last_response"]}')
        pass


def test_socket():
    try:
        ap = setup_ap()
        server_ip = ap.ip
        ENTRY_PORT = 10000


        entry_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)  # Create a TCP/IP socket
        entry_address = (server_ip, ENTRY_PORT)
        entry_socket.bind(entry_address)  # Bind the socket to the port
        entry_socket.listen(2)  # Listen for incoming connections

        while 1:
            time.sleep(3)
            print('waiting for a connection')
            connection, client_address = entry_socket.accept()

            print(f'*** connection from {client_address} ***')
            data = connection.recv(100000)
            # data = "100,200,1,2,3|100,200,1,2,3|100,200,1,2,3"
            # client_address = "123.456.789.102"
            parse_MCU_byte(data, client_address)
            break
    except Exception as e:
        print(e)
    finally:
        ap.stop()


def parse_MCU_byte(data, uid):
    data = str(data)
    data = data.split("|")
    for reading in data:
        reading = reading.split(",")
        beacon_ts = reading[0]
        local_ts = reading[1]
        print("Recieved from board {}:\n\tBeacon timestamp:\t{}\n\tLocal timestamp:\t{}".format(uid, beacon_ts,
                                                                                                local_ts))
        store_data(uid, beacon_ts, local_ts)


def store_data(uid, beacon_timestamp, local_time):
    board_file = create_files(uid)
    f = open(os.path.join(os.getcwd(), "timestamp_data", board_file), 'a')
    f.write('\n' + str(beacon_timestamp) + ',' + str(local_time))
    f.close()

if __name__ == "__main__":
    test_socket()
    #main()
    # create_files()
    # store_data(1, 100, 200)
    # store_data(2, 100, 201)