FROM python:3.10-alpine3.16

RUN apk add git g++

WORKDIR /app
COPY requirements.txt .

RUN pip3 install -r requirements.txt
COPY . . 

CMD ["python3", "main.py"]
