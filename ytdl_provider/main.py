import socket
import os
import logging
import struct
import threading
from functools import partial

logging.basicConfig(
    level=logging.DEBUG,
    format="%(asctime)s [%(levelname)s] %(message)s",
    handlers=[
        logging.StreamHandler()
    ]
)


logger = logging.getLogger()


port = os.getenv("PORT", 3436)

sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEPORT, 1)
server_address = ("127.0.0.1", port)
sock.bind(server_address)
logger.info(f"Listening on 0.0.0.0:{port}")

def recvall(sock, n):
    # Helper function to recv n bytes or return None if EOF is hit
    data = bytearray()
    while len(data) < n:
        packet = sock.recv(n - len(data))
        logging.debug(f"Received packet: {packet}")
        if not packet:
            return None
        data.extend(packet)
    return data
def handleConnection(connection, client_address):
    try:
        while True:
            raw_message_length = recvall(connection, 4)
            logging.debug(f"Received Connection that wants to send message with length: {raw_message_length}")
            message_length = struct.unpack("<I", raw_message_length)[0]
            logging.debug(f"Received Connection that wants to send message with length: {message_length}")
            connection.send(struct.pack("I", message_length))
            bytes_read = 0 
            message_bytes = [] 
            message = recvall(connection, message_length)
            logging.debug(f"Message that client ip: {client_address} sent was: {message}")
            reply = message.decode("utf-8") 
            if reply == "closeConnection":
                connection.close();
                break
            reply_length = struct.pack("I", len(reply))
            connection.send(reply_length)
            connection.send(reply.encode())
    except Exception as e:
        print(e)
        connection.close()

sock.listen()
try:
    while True:
        connection, client_address = sock.accept()
        threading.Thread(target=handleConnection, args=(connection, client_address)).start()

except Exception as e:
    sock.close()
    raise e


