import yt_dlp
import logging
from functools import cached_property
from enum import Enum, auto
from os import getenv
import hashlib
import pathlib
from threading import Lock
import threading
from collections import defaultdict


class State(Enum):
    UNINITIALIZED = auto()
    VALID = auto()
    INVALID = auto()


DOWNLOAD_FOLDER = getenv("DOWNLOAD_FOLDER", "./videos")


class YoutubeVideo:
    videos = {}
    __mutexRepo = defaultdict(lambda: Lock())
    __url: str
    state: State
    __logger: logging.Logger

    def __init__(self, url: str):
        self.__url = url
        self.state = State.UNINITIALIZED
        self.__logger = logging.getLogger("YoutubeVideo")
        YoutubeVideo.videos[url] = self

    @cached_property
    def get_info(self):
        try:
            self.state = State.VALID
            with yt_dlp.YoutubeDL() as ydl:
                return ydl.extract_info(self.__url, download=False)
        except yt_dlp.utils.DownloadError:
            self.state = State.INVALID
            return None

    def download(self, audioOnly: bool, resolution: str):
        key = self.get_info[
            "id"] + resolution if audioOnly else self.get_info["id"] + "audio_only"
        mutex = self.__mutexRepo[key]
        parentFolderName = hashlib.sha1(self.__url.encode("utf8")).hexdigest()
        with mutex:
            logging.debug(f"THREAD {threading.get_ident()}: Acquired lock for initialisation of id: {self.get_info['id']} with"
                          f"audio_only: {audioOnly} and resolution: {resolution} ")
            opts = (
                {
                    "format_sort": [f"res:{resolution}", "ext:mp4:m4a"],
                    "outtmpl": f"{DOWNLOAD_FOLDER}/{parentFolderName}/%(id)s_{resolution}.%(ext)s",
                    "merge_output_format": "mp4",
                    "overwrites": False,
                    "nocheckcertificate": True,
                } if not audioOnly else {
                    "format": "bestaudio",
                    "nocheckcertificate": True,
                    "outtmpl": f"{DOWNLOAD_FOLDER}/{parentFolderName}/%(id)s_{resolution}.%(ext)s",
                    "final_ext": "mp3",
                    "postprocessors": [
                        {
                            "key": "FFmpegExtractAudio",
                            "preferredcodec": "mp3",
                        }
                    ],
                }
            )


            try:
                with yt_dlp.YoutubeDL(opts) as ydl:
                    info = ydl.extract_info(self.__url, download=True)
                    return {"path": info["requested_downloads"][0]["filepath"], "title": info["title"]}, ""

            except Exception as e:
                logging.error(
                    f"Error during download of url: {self.__url} res: ${resolution} audio_only: ${audioOnly}")
                logging.error(str(e))
                return None, "Unexpected Error occured during Download"


def get_video_information(url: str):
    yt = YoutubeVideo.videos.get(url)
    if yt is None:
        yt = YoutubeVideo(url)
    info = yt.get_info
    return info


def download_video(url: str, res: str, audioOnly: bool):
    yt = YoutubeVideo.videos.get(url)
    if yt is None:
        yt = YoutubeVideo(url)
    return yt.download(audioOnly, res)
