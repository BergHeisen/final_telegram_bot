import yt_dlp
import logging
from functools import cached_property
from enum import Enum, auto
from os import getenv
import pathlib
from threading import Lock
import threading
from collections import defaultdict


class State(Enum):
    UNINITIALIZED = auto()
    VALID = auto()
    INVALID = auto()


DOWNLOAD_FOLDER = getenv("DOWNLOAD_FOLDER", "./videos")
MAX_FILESIZE = getenv("MAX_FILESIZE", None)


class YoutubeVideo:
    videos = {}
    __mutexRepo = defaultdict(lambda: Lock())
    __yt = yt_dlp.YoutubeDL()
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
            return YoutubeVideo.__yt.extract_info(self.__url, download=False)
        except yt_dlp.utils.DownloadError:
            self.state = State.INVALID
            return None

    def download(self, audioOnly: bool, resolution: str):
        key = self.get_info[
            "id"] + resolution if audioOnly else self.get_info["id"] + "audio_only"
        mutex = self.__mutexRepo[key]
        with mutex:
            logging.debug(f"THREAD {threading.get_ident()}: Acquired lock for initialisation of id: {self.get_info['id']} with"
                          f"audio_only: {audioOnly} and resolution: {resolution} ")
            opts = (
                {
                    "format-sort": [f"res={resolution}", "ext"],
                    "outtmpl": f"{DOWNLOAD_FOLDER}/%(id)s_{resolution}.%(ext)s",
                    "overwrites": False,
                    "nocheckcertificate": True,
                    "merge_output_format": "mp4",
                } if not audioOnly else {
                    "format": "bestaudio",
                    "nocheckcertificate": True,
                    "outtmpl": f"{DOWNLOAD_FOLDER}/%(id)s_{resolution}.%(ext)s",
                    "final_ext": "mp3",
                    "postprocessors": [
                        {
                            "key": "FFmpegExtractAudio",
                            "preferredcodec": "mp3",
                        }
                    ],
                }
            )

            if MAX_FILESIZE is not None:
                from yt_dlp import FileDownloader
                opts["max_filesize"] = FileDownloader.parse_bytes(MAX_FILESIZE)
                opts["error_on_too_large"] = True

            try:
                with yt_dlp.YoutubeDL(opts) as ydl:
                    info = ydl.extract_info(self.__url, download=True)
                    return {"path": info["requested_downloads"][0]["filepath"], "title": info["title"]}, ""

            except yt_dlp.utils.FileTooLarge:
                return None, "Requested File is too large too download"

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
