import socket
from util import util
import threading
import time
from collections import deque
import math
import struct

# make sure this matches the ENTRY_PORT global macro in the cc3220sf ap_connection.c code as well
ENTRY_PORT = 10000
MAX_BOARDS = 2
MAX_BIND_RETRIES = 5
MESSAGE_SIZE = 50

MULTICAST_GROUP_IP = "224.10.10.10"
MULTICAST_GROUP_PORT = 10007
MULTICAST_TTL = struct.pack('b', 12)

# global shared vars for multi-threading communication
str_to_send = None
thread_tasks_done = None
using_udp = None


class Server:
    def __init__(self, ipv4, board_count=MAX_BOARDS):
        if ipv4 is None:
            print('ipv4 can\'t be None')
        self.ipv4 = ipv4
        self.boards = {}
        self.entry_port = ENTRY_PORT
        self.broadcast_port = MULTICAST_GROUP_PORT
        self.last_used_port = None
        self.entry_address = None
        self.entry_socket = None
        self.broadcast_address = None
        self.broadcast_socket = None
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

        self.broadcast_address = (self.ipv4, self.broadcast_port)
        self.broadcast_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)  # Create the datagram socket
        # Set a timeout so the socket does not block indefinitely when trying to receive data
        # self.broadcast_socket.settimeout(0.2)
        self.broadcast_socket.settimeout(20)
        self.broadcast_socket.setsockopt(socket.IPPROTO_IP, socket.IP_MULTICAST_TTL, MULTICAST_TTL)

        print(f'*** server running, IP: {socket.gethostbyname(self.entry_address[0])}, entry'
              f' port: {self.entry_address[1]} ***')

        return self.entry_socket

    def wait_for_all_boards_to_connect(self, task='test_time_sync'):
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
                            socket_address = self.add_board(ipv4=cc3220sf_ip_str, task=task)
                            msg = bytes(str(socket_address[1]), encoding='utf-8')
                            connection.sendall(msg)
                            print(f'*** port sent to board as bytes {msg} ***')
            finally:
                # Clean up the connection
                connection.close()

        print(f'*** done finding all boards, boards connected to server: {len(self.boards)} ***')
        return 0

    def add_board(self, ipv4, task='test_time_sync'):
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
        if task == 'test_time_sync':
            thread = threading.Thread(target=wait_to_send_msgs,
                                      args=(socket_this_side, self.boards[ipv4],
                                            self.start_flag, self.end_flag, self.exit_flag, self.broadcast_socket))
        elif task == 'send_data':
            thread = threading.Thread(target=wait_and_plot_incoming_accel_data,
                                      args=(socket_this_side, self.boards[ipv4], self.exit_flag))

        thread.start()
        self.boards[ipv4]['thread'] = thread

        return socket_address

    def send_msg_to_all_boards_tcp(self, msg):
        global str_to_send, thread_tasks_done, using_udp

        str_to_send = msg
        thread_tasks_done = 0
        using_udp = False

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

    def send_msg_to_all_boards_udp(self, msg):
        global str_to_send, thread_tasks_done, using_udp

        thread_tasks_done = 0
        using_udp = True

        if msg == 'exit':
            self.exit_flag.set()
            self.start_flag.set()

            # close out of any active connections running in child threads
            if msg == 'exit':
                for ipv4 in self.boards:
                    self.boards[ipv4]['thread'].join()

            return 0

        else:
            sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)

            addy1 = ('192.168.137.255', 10012)
            # sock.bind(addy1)

            # addy2 = ('10.10.10.255', 10012)
            # sock.bind(addy2)

            # addy3 = ('', 10012)
            # sock.bind(addy3)

            sent_bytes = sock.sendto(bytes(msg, encoding='utf-8'), addy1)
            print(f'*** Successfully sent {sent_bytes} bytes on broadcast socket: {addy1} ***')

            # sent_bytes = sock.sendto(bytes(msg, encoding='utf-8'), addy2)
            # print(f'*** Successfully sent {sent_bytes} bytes on broadcast socket: {addy2} ***')

            # sent_bytes = sock.sendto(bytes(msg, encoding='utf-8'), addy3)
            # print(f'*** Successfully sent {sent_bytes} bytes on broadcast socket: {addy3} ***')

        self.end_flag.clear()
        self.start_flag.set()

        while thread_tasks_done != len(self.boards):
            time.sleep(0.1)

        self.start_flag.clear()
        self.end_flag.set()

        return 0

    def send_msg_to_all_boards_mac(self, msg):
        global str_to_send, thread_tasks_done, using_udp

        thread_tasks_done = 0
        using_udp = True

        if msg == 'exit':
            self.exit_flag.set()
            self.start_flag.set()

            # close out of any active connections running in child threads
            if msg == 'exit':
                for ipv4 in self.boards:
                    self.boards[ipv4]['thread'].join()

            return 0

        else:
            s = socket(socket.AF_PACKET, socket.SOCK_RAW)
            s.bind(("Wireless LAN adapter Wi-Fi", 0))

            src_addr = "\x70\xBC\x10\x69\x98\xB7"
            dst_addr = "\x40\x06\xA0\x97\x5E\xB6"
            payload = "1 - ayo"
            # checksum = "\x1a\x2b\x3c\x4d"
            ethertype = "\x08\x01"

            # s.send(dst_addr + src_addr + ethertype + payload + checksum)
            s.sendall(dst_addr + src_addr + ethertype + payload)
            s.close()

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


def wait_to_send_msgs(sock, coms_dict, start_flag, end_flag, exit_flag, broadcast_socket):
    global str_to_send, thread_tasks_done, using_udp
    while not exit_flag.is_set():
        # Wait for a connection
        connection, client_address = sock.accept()
        try:
            while not exit_flag.is_set():
                start_flag.wait()
                if exit_flag.is_set():
                    return 0

                if not using_udp:
                    my_msg = bytes(str_to_send, encoding='utf-8')
                    connection.sendall(my_msg)

                # wait for any response
                data = None
                while not data:
                    if not using_udp:
                        data = connection.recv(MESSAGE_SIZE)
                    else:
                        data, server = broadcast_socket.recvfrom(MESSAGE_SIZE)

                    if data:
                        coms_dict['last_response'] = util.strip_end_bytes(data)
                        coms_dict['last_response_server'] = server

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
