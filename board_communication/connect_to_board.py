import socket
import sys
from windows_ap import WindowsSoftAP
from util import util
from server import Server
from util.pylive import live_plotter_xy
import time
import numpy as np


def main():
    # setup and start windows "hostednetwork" soft AP
    ap = WindowsSoftAP()
    ap.start_ap()
    server_ip = ap.get_ipv4()

    # Create and start server for communicating with boards
    server = Server(server_ip, board_count=1)

    # start_server() returns socket for server that is already listening for incoming connections
    server_entry_socket = server.start_server()

    retval = server.wait_for_all_boards_to_connect()

    if retval < 0:
        # ap.stop_ap()
        return 0

    """
    For testing clock drift
    """
    # while True:
    #     msg = input('Enter msg to send to all boards: ')
    #
    #     server.send_msg_to_all_boards(msg)
    #
    #     if msg == 'exit':
    #         # make sure to send "exit" msg to send_msg_to_all_boards() before exiting so that all child threads are
    #         # closed as well
    #         return 0
    #
    #     for i, board_ip in enumerate(server.boards):
    #         print(f'[board {i}] response from board with ip {board_ip}:\n{server.boards[board_ip]["last_response"]}')
    #     print()

    """
    For plotting data stream
    """
    max_period = 50
    x_vecs = []
    y_vecs = []
    y_labels = []
    y_ranges = []
    for i in range(len(server.boards)):
        y_labels.extend([f'accel. x\nboard {i}', f'accel. y\nboard {i}',
                         f'accel. z\nboard {i}', f'accel. magnitude\nboard {i}'])
        y_ranges.extend([(-12, 12), (-12, 12), (-12, 12), (0, 12)])  # x, y, z, magnitude
        for _ in range(4):
            x_vecs.append(np.zeros(max_period))
            y_vecs.append(np.zeros(max_period))
    lines = []
    update_period = 0.1

    while True:
        for i, board_ip in enumerate(server.boards):
            if len(server.boards[board_ip]["stream_data"]) > 0:
                # idx, x, y, z, magnitude = server.boards[board_ip]["stream_data"].popleft()    # FIFO board reading
                idx, x, y, z, magnitude = server.boards[board_ip]["stream_data"].pop()  # LIFO board reading
                x = x / 1000
                y = y / 1000
                z = z / 1000
                magnitude = magnitude / 1000
                x_vecs[4 * i] = np.append(x_vecs[4 * i][1:], idx)
                x_vecs[4 * i + 1] = np.append(x_vecs[4 * i + 1][1:], idx)
                x_vecs[4 * i + 2] = np.append(x_vecs[4 * i + 2][1:], idx)
                x_vecs[4 * i + 3] = np.append(x_vecs[4 * i + 3][1:], idx)

                y_vecs[4 * i] = np.append(y_vecs[4 * i][1:], x)
                y_vecs[4 * i + 1] = np.append(y_vecs[4 * i + 1][1:], y)
                y_vecs[4 * i + 2] = np.append(y_vecs[4 * i + 2][1:], z)
                y_vecs[4 * i + 3] = np.append(y_vecs[4 * i + 3][1:], magnitude)
            else:
                x_vecs[4 * i] = np.append(x_vecs[4 * i][1:], 0)
                x_vecs[4 * i + 1] = np.append(x_vecs[4 * i + 1][1:], 0)
                x_vecs[4 * i + 2] = np.append(x_vecs[4 * i + 2][1:], 0)
                x_vecs[4 * i + 3] = np.append(x_vecs[4 * i + 3][1:], 0)

                y_vecs[4 * i] = np.append(y_vecs[4 * i][1:], 0)
                y_vecs[4 * i + 1] = np.append(y_vecs[4 * i + 1][1:], 0)
                y_vecs[4 * i + 2] = np.append(y_vecs[4 * i + 2][1:], 0)
                y_vecs[4 * i + 3] = np.append(y_vecs[4 * i + 3][1:], 0)

        lines = live_plotter_xy(x_vecs, y_vecs, lines, 4 * len(server.boards), x_label='board time (i)',
                                y_labels=y_labels,
                                title='Board Accelerometer G-Forces vs. Board Time',
                                pause_time=update_period,
                                dynamic_y=False,
                                y_ranges=y_ranges)


if __name__ == "__main__":
    main()
