import socket
import sys
from windows_ap import WindowsSoftAP
import util
from server import Server


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

    while True:
        msg = input('Enter msg to send to all boards: ')

        server.send_msg_to_all_boards(msg)

        if msg == 'exit':
            # make sure to send "exit" msg to send_msg_to_all_boards() before exiting so that all child threads are
            # closed as well
            return 0

        for i, board_ip in enumerate(server.boards):
            print(f'[board {i}] response from board with ip {board_ip}:\n{server.boards[board_ip]["last_response"]}')
        print()


if __name__ == "__main__":
    main()
