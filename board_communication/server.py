import socket
from util import util
import threading
import time
from util.pylive import live_plotter_xy
import numpy as np
from collections import deque
import math

# make sure this matches the ENTRY_PORT global macro in the cc3220sf ap_connection.c code as well
ENTRY_PORT = 10000
MAX_BOARDS = 2
MAX_BIND_RETRIES = 5

str_to_send = None
thread_tasks_done = 0


class Server:
    def __init__(self, ipv4, board_count=MAX_BOARDS):
        self.ipv4 = ipv4
        self.boards = {}
        self.entry_port = ENTRY_PORT
        self.last_used_port = None
        self.entry_address = None
        self.entry_socket = None
        self.start_flag = threading.Event()
        self.end_flag = threading.Event()
        self.exit_flag = threading.Event()
        self.max_retries = MAX_BIND_RETRIES
        self.board_count = board_count

    def start_server(self):
        self.entry_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)  # Create a TCP/IP socket
        self.entry_address = (self.ipv4, self.entry_port)
        self.entry_socket.bind(self.entry_address)  # Bind the socket to the port
        self.entry_socket.listen(self.board_count)  # Listen for incoming connections
        self.last_used_port = self.entry_address[1]

        print(f'*** starting up server on {socket.gethostbyname(self.entry_address[0])}'
              f' port {self.entry_address[1]} ***')

        return self.entry_socket

    def wait_for_all_boards_to_connect(self):
        while len(self.boards) != self.board_count:
            # Wait for a connection
            print('*** waiting for a connection (use ctrl-c to cancel) ***')
            try:
                connection, client_address = self.entry_socket.accept()
            except KeyboardInterrupt:
                return -1

            try:
                print(f'*** connection from {client_address} ***')
                data = connection.recv(25)
                if data:
                    print('*** received packets with data, bytes: "%s" ***' % data)
                    buf = str(data, encoding='utf-8')
                    if 'cc3220sf ipv4' in buf:
                        cc3220sf_ip_str = util.int_ip_to_ip_str(int(buf.strip().split(sep=':')[1]))
                        print('*** extracted ip: "%s" ***' % cc3220sf_ip_str)

                        # security measure, might be useful?
                        if client_address[0] == cc3220sf_ip_str:
                            socket_address = self.add_board(ipv4=cc3220sf_ip_str)
                            msg = bytes(str(socket_address[1]), encoding='utf-8')
                            connection.sendall(msg)
                            print(f'*** port sent to board as bytes {msg} ***')
            finally:
                # Clean up the connection
                connection.close()

        print(f'*** done finding all boards, boards connected to server: {len(self.boards)} ***')
        return 0

    def add_board(self, ipv4):
        socket_this_side = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        socket_address = (self.ipv4, self.last_used_port + 1)

        not_bound = True
        retries = 0
        while not_bound and retries < self.max_retries:
            try:
                socket_this_side.bind(socket_address)

                socket_this_side.listen(1)  # Listen for incoming connections

                not_bound = False
            except Exception:
                print(f'unable to bind to socket address: {socket_address}, retrying...')
                socket_address = (socket_address[0], socket_address[1] + 1)
                retries += 1

        if retries >= self.max_retries:
            print(f'failed to bind to socket address: {socket_address}')
            return None

        self.last_used_port = socket_address[1]
        self.boards[ipv4] = {"ipv4": ipv4,
                             "socket_this_side": socket_this_side,
                             "port_this_side": socket_address[1],
                             "last_response": None,
                             "stream_data": deque()}

        # thread = threading.Thread(target=wait_to_send_msgs,
        #                           args=(socket_this_side, self.boards[ipv4],
        #                                 self.start_flag, self.end_flag, self.exit_flag))

        thread = threading.Thread(target=wait_and_plot_incoming_accel_data,
                                  args=(socket_this_side, self.boards[ipv4],
                                        self.exit_flag))
        thread.start()
        self.boards[ipv4]['thread'] = thread

        return socket_address

    def send_msg_to_all_boards(self, msg):
        global str_to_send, thread_tasks_done

        str_to_send = msg
        thread_tasks_done = 0

        if msg == 'exit':
            self.exit_flag.set()
            self.start_flag.set()

            # close out of any active connections running in child threads
            if msg == 'exit':
                for ipv4 in self.boards:
                    self.boards[ipv4]['thread'].join()

            return 0

        self.end_flag.clear()
        self.start_flag.set()

        while thread_tasks_done != len(self.boards):
            time.sleep(0.1)

        self.start_flag.clear()
        self.end_flag.set()

        return 0

    def get_board_socket(self, ipv4):
        if ipv4 in self.boards:
            return self.boards[ipv4]['socket_this_side']
        print(f'*** No board w/ IPv4 {ipv4} exists within this Server instance ***')
        return None

    def __del__(self):
        # close entry socket of this server
        self.entry_socket.close()

        # close all sockets with connection to boards
        for ipv4 in self.boards:
            self.boards[ipv4]['socket_this_side'].close()


def wait_to_send_msgs(sock, coms_dict, start_flag, end_flag, exit_flag):
    global str_to_send, thread_tasks_done
    while not exit_flag.is_set():
        # Wait for a connection
        connection, client_address = sock.accept()

        try:
            while not exit_flag.is_set():
                start_flag.wait()

                if exit_flag.is_set():
                    return 0

                my_msg = bytes(str_to_send, encoding='utf-8')
                connection.sendall(my_msg)

                # wait for any response
                data = None
                while not data:
                    data = connection.recv(50)
                    if data:
                        coms_dict['last_response'] = util.strip_end_bytes(data)

                # increment threads task counter
                thread_tasks_done += 1

                # wait to start next loop iteration this will occur once all threads have completed their tasks
                # which is indicated by when thread_tasks_done == # of threads
                end_flag.wait()
        finally:
            # Clean up the connection
            connection.close()
    return 0


def wait_and_plot_incoming_accel_data(sock, coms_dict, exit_flag):
    max_buffer_size = 600000
    while not exit_flag.is_set():
        # Wait for a connection
        connection, client_address = sock.accept()
        try:
            while not exit_flag.is_set():
                data = connection.recv(60)
                if data:
                    i, x, y, z = util.strip_end_bytes(data).split(',')
                    i = int(i)
                    x = float(x)
                    y = float(y)
                    z = float(z)
                    magnitude = math.sqrt(x ** 2 + y ** 2 + z ** 2)
                    coms_dict['stream_data'].append((i, x, y, z, magnitude))
                    if len(coms_dict['stream_data']) >= max_buffer_size:
                        coms_dict['stream_data'].popleft()
        finally:
            # Clean up the connection
            connection.close()
    return 0
