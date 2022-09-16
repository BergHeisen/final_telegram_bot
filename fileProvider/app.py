import flask
from flask import Flask
from markupsafe import escape
import sqlite3
from os import getenv
import os.path
import logging

app = Flask(__name__)
repository_path = f"{getenv('DOWNLOAD_FOLDER', '../')}/repository.db"
db = sqlite3.connect(repository_path, check_same_thread=False)
logging.basicConfig(
    level=logging.DEBUG,
    format="%(asctime)s [%(levelname)s] %(message)s",
    handlers=[
        logging.StreamHandler()
    ]
)

logger = logging.getLogger()
def get_extension(filename):
    return os.path.splitext(filename)[1]

def get_file_or_none(id: str):
    query = f"SELECT filePath, title FROM Videos WHERE Videos.id = '{id}'"
    logger.info(f"With DB: {repository_path} executing query: {query}")
    result = db.execute(query).fetchone()
    logger.debug(f"With DB: {repository_path} executed query: {query} result: {result}")
    if result is None:
        logger.debug(f"With DB: {repository_path} executed query: {query} found nothing")
        return None, None
    file, title = result
    return file, title
    

@app.get("/video/<fileId>") 
def getVideo(fileId):
    id = escape(fileId)
    file, title= get_file_or_none(id)
    if file is None:
        return flask.make_response("", 404)
    return flask.send_file(file, as_attachment= True, download_name=f"{title}.{get_extension(file)}")

