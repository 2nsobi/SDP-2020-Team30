import socket
import sys
from windows_ap import WindowsSoftAP
import util
from server import Server
from pylive import live_plotter_xy
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
    max_period = 20
    x_vecs = []
    y_vecs = []
    for _ in range(len(server.boards)):
        x_vecs.append(np.zeros(max_period))
        y_vecs.append(np.zeros(max_period))
    lines = []
    update_period = 0.1

    while True:
        for i, board_ip in enumerate(server.boards):
            if len(server.boards[board_ip]["stream_data"]) > 0:
                x, y = server.boards[board_ip]["stream_data"].popleft()
                x_vecs[i] = np.append(x_vecs[i][1:], x)
                y_vecs[i] = np.append(y_vecs[i][1:], y)
            else:
                x_vecs[i] = np.append(x_vecs[i][1:], 0)
                y_vecs[i] = np.append(y_vecs[i][1:], 0)

        lines = live_plotter_xy(x_vecs, y_vecs, lines, len(server.boards), x_label='x',
                                y_label='sine(x)',
                                title='What a Referee Might be Seeing',
                                pause_time=update_period)


if __name__ == "__main__":
    main()
