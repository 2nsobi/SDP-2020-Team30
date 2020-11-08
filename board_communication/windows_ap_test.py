import socket
import sys
from windows_ap import WindowsSoftAP

# setup and start windows "hostednetwork" soft ap
ap = WindowsSoftAP()
ap.start_ap()
server_ip = ap.get_ipv4()

# Create a TCP/IP socket
sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

# Bind the socket to the port
server_address = (server_ip, 10000)
# server_address = (socket.gethostname(), 10000)
print(sys.stderr, f'starting up on {socket.gethostbyname(server_address[0])} port {server_address[1]}')
sock.bind(server_address)

# Listen for incoming connections
sock.listen(1)

while True:
    # Wait for a connection
    print(sys.stderr, 'waiting for a connection')
    connection, client_address = sock.accept()

    try:
        print(sys.stderr, 'connection from', client_address)

        # Receive the data in small chunks and retransmit it
        while True:
            data = connection.recv(15)
            print(sys.stderr, 'received packets, bytes: "%s", string: "%s"' % (data, str(data, encoding='utf-8')))
            if data:
                print(sys.stderr, 'sending data back to the client')
                connection.sendall(data)
            else:
                print(sys.stderr, 'no more data from', client_address)
                break

    finally:
        # Clean up the connection
        connection.close()
