from codecs import unicode_escape_encode
import socket
import os
import logging
import struct
import threading
from dataclasses_json import dataclass_json
from dataclasses import dataclass
from typing import Dict, List
import json
import yt


VIDEO_INFORMATION_KEYS = "original_url title uploader id formats thumbnails description extractor ext".split(
    " ")

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


@dataclass
@dataclass_json
class Request:
    operation: str
    url: str


@dataclass_json
@dataclass
class InfoResponse:
    success: bool
    error_msg: str
    info: str


@dataclass_json
@dataclass
class FileResponse:
    success: bool
    error_msg: str
    file_path: str
    title: str


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


def copy_value_from_keys(obj: Dict, keys: List):
    result = {}
    for key in keys:
        result[key] = obj.get(key)
    return result


def handleConnection(connection, client_address):
    try:
        while True:
            raw_message_length = recvall(connection, 4)
            if raw_message_length is not None:

                message_length = struct.unpack(">I", raw_message_length)[0]

                logging.debug(
                    f"Received Connection that wants to send message with length: {message_length}")
                message = recvall(connection, message_length)

                if message is not None:

                    logging.debug(
                        f"Message that client ip: {client_address} sent was: {message}")
                    message_decoded = message.decode("utf-8")

                    if message_decoded == "closeConnection":
                        connection.close()
                        break

                    else:
                        request = json.loads(message_decoded)

                        if (request["operation"] == "getInfo"):
                            info = yt.get_video_information(request["url"])
                            video_info = {
                                "success": info is not None,
                                "error_msg": "" if info is not None else "Invalid URL",
                                "info": copy_value_from_keys(info, VIDEO_INFORMATION_KEYS) if info is not None else None
                            }
                            reply = json.dumps(video_info, ensure_ascii=False)
                            encoded = reply.encode("utf8")
                            reply_length = struct.pack(">I", len(encoded))
                            connection.send(reply_length)
                            connection.sendall(encoded)

                            logging.debug("Finished sending")
                        if (request["operation"] == "download"):
                            file, errorMessage = yt.download_video(
                                request["url"], request["resolution"], request["audio_only"])
                            reply = FileResponse(True, "", file["path"], file["title"]).to_json(
                            ) if file is not None else FileResponse(False, errorMessage, "", "").to_json()
                            reply_length = struct.pack(">I", len(reply))
                            connection.send(reply_length)
                            connection.send(reply.encode())
    except Exception as e:
        connection.close()
        raise e


sock.listen()
try:
    while True:
        connection, client_address = sock.accept()
        threading.Thread(target=handleConnection, args=(
            connection, client_address)).start()

except Exception as e:
    sock.close()
    raise e
