FROM python:3.10-alpine3.16

WORKDIR /app
COPY requirements.txt .
RUN pip3 install -r requirements.txt

COPY . . 
