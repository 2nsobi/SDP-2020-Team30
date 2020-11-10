import socket
import sys
from windows_ap import WindowsSoftAP
import util

def main():
    # setup and start windows "hostednetwork" soft AP
    ap = WindowsSoftAP()
    ap.start_ap()
    server_ip = ap.get_ipv4()

    # Create a TCP/IP socket
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

    # Bind the socket to the port
    server_address = (server_ip, 10000)     # (socket.gethostname(), 10000)
    print(sys.stderr, f'starting up on {socket.gethostbyname(server_address[0])} port {server_address[1]}')
    sock.bind(server_address)

    # Listen for incoming connections
    sock.listen(1)

    cc3220sf_ip_str = None

    while not cc3220sf_ip_str:
        # Wait for a connection
        print(sys.stderr, 'waiting for a connection')
        connection, client_address = sock.accept()

        try:
            print(sys.stderr, 'connection from', client_address)

            # Receive the data in small chunks and retransmit it
            while True:
                data = connection.recv(25)
                if data:
                    print(sys.stderr, 'received packets with data, bytes: "%s"' % data)
                    buf = str(data, encoding='utf-8')
                    if 'cc3220sf ipv4' in buf:
                        cc3220sf_ip_str = util.int_ip_to_ip_str(int(buf.strip().split(sep=':')[1]))
                        print(sys.stderr, 'extracted ip: "%s"' % cc3220sf_ip_str)
                else:
                    print(sys.stderr, 'no more data from', client_address)
                    break

        finally:
            # Clean up the connection
            connection.close()

    wait_for_my_input = input()

    # Create a TCP/IP socket
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

    # Connect the socket to the port where the server is listening
    server_address = ("192.168.137.216", 10000)  # (cc3220sf_ip_str, 10000)
    print(sys.stderr, 'connecting to %s port %s' % server_address)
    sock.connect(server_address)

    try:
        # Send data
        message = b'whats up'
        print(sys.stderr, 'sending "%s"' % message)
        sock.sendall(message)

    finally:
        print(sys.stderr, 'closing socket')
        sock.close()


if __name__ == "__main__":
    main()
