import socket
from windows_ap import WindowsSoftAP

ENTRY_PORT = 10000

def main():
    ap = WindowsSoftAP()
    server_ip = ap.get_ipv4()
    if server_ip is None:
        print('server_ip is None')
        quit()

    entry_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)  # Create a TCP/IP socket
    entry_address = (server_ip, ENTRY_PORT)
    entry_socket.bind(entry_address)  # Bind the socket to the port
    entry_socket.listen(2)  # Listen for incoming connections

    while 1:
        print('waiting for a connection')
        connection, client_address = entry_socket.accept()

        print(f'*** connection from {client_address} ***')
        data = connection.recv(100000)

        print(data)

if __name__ == '__main__':
    main()
