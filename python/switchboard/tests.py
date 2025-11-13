import argparse  # https://www.w3schools.com/python/ref_module_argparse.asp
import asyncio  # https://www.w3schools.com/python/ref_module_asyncio.asp
import atexit  # https://www.w3schools.com/python/ref_module_atexit.asp
import configparser  # https://www.w3schools.com/python/ref_module_configparser.asp
from dataclasses import (
    dataclass,
)  # https://www.w3schools.com/python/ref_module_dataclasses.asp
from enum import Enum  # https://www.w3schools.com/python/ref_module_enum.asp
import getopt  # https://www.w3schools.com/python/ref_module_getopt.asp
import hashlib  # ttps://www.w3schools.com/python/ref_module_hashlib.asp
import hmac  # https://www.w3schools.com/python/ref_module_hmac.asp
import hashlib  # https://www.w3schools.com/python/ref_module_hmac.asp
import json  # https://www.w3schools.com/python/ref_module_json.asp
import logging  # https://www.w3schools.com/python/ref_module_logging.asp
import os  # https://www.w3schools.com/python/ref_module_os.asp
from pprint import pprint  # https://www.w3schools.com/python/ref_module_pprint.asp
import queue  # https://www.w3schools.com/python/ref_module_queue.asp
import re  # https://www.w3schools.com/python/ref_module_re.asp
import sched  # https://www.w3schools.com/python/ref_module_sched.asp
import time  # https://www.w3schools.com/python/ref_module_time.asp
import secrets  # https://www.w3schools.com/python/ref_module_secrets.asp
import shelve  # https://www.w3schools.com/python/ref_module_shelve.asp
import signal  # https://www.w3schools.com/python/ref_module_signal.asp
import sys  # https://www.w3schools.com/python/ref_module_sys.asp
import site  # https://www.w3schools.com/python/ref_module_site.asp
import socket  # https://www.w3schools.com/python/ref_module_socket.asp
import socketserver  # https://www.w3schools.com/python/ref_module_socketserver.asp
import sqlite3  # https://www.w3schools.com/python/ref_module_sqlite3.asp
import ssl  # https://www.w3schools.com/python/ref_module_ssl.asp
import struct  # https://www.w3schools.com/python/ref_module_struct.asp
import threading  # https://www.w3schools.com/python/ref_module_threading.asp
import tkinter as tk  # https://www.w3schools.com/python/ref_module_tkinter.asp
import uuid  # https://www.w3schools.com/python/ref_module_uuid.asp
import http  # https://www.w3schools.com/python/ref_module_http.asp


print(hasattr(http, "HTTPStatus"))

id1 = uuid.uuid4()
id2 = uuid.uuid4()
print(f"UUID 1: {id1}")
print(f"UUID 2: {id2}")
print(f"Are they equal? {id1 == id2}")

print("tkinter is available")
print("Standard GUI toolkit for Python")


def task():
    print("Thread running for Linus")


thread = threading.Thread(target=task)
print(f"Thread created: {thread.name}")

packed = struct.pack("ii", 42, 100)
print(f"Packed bytes: {packed}")
unpacked = struct.unpack("ii", packed)
print(f"Unpacked values: {unpacked}")

print(f"OpenSSL version: {ssl.OPENSSL_VERSION}")
print(f"Has SNI: {ssl.HAS_SNI}")

conn = sqlite3.connect(":memory:")
cursor = conn.cursor()
cursor.execute("CREATE TABLE users (name TEXT, age INTEGER)")
cursor.execute("INSERT INTO users VALUES ('Tobias', 28)")
conn.commit()

cursor.execute("SELECT * FROM users")
result = cursor.fetchone()
print(f"User: {result[0]}, Age: {result[1]}")


class MyHandler(socketserver.BaseRequestHandler):
    def handle(self):
        print("Request received from Linus")


print("Server handler created")

hostname = socket.gethostname()
ip = socket.gethostbyname(hostname)
print(f"Hostname: {hostname}")
print(f"IP: {ip}")

print(f"User site packages: {site.USER_SITE}")
print(f"User base: {site.USER_BASE}")


def handler(signum, frame):
    print("Signal received, exiting gracefully...")
    sys.exit(0)


signal.signal(signal.SIGINT, handler)
print("Press Ctrl+C to trigger signal")

print(f"Python version: {sys.version}")
print(f"Platform: {sys.platform}")

token = secrets.token_hex(16)
print(f"Secure token: {token}")
scheduler = sched.scheduler(time.time, time.sleep)

start = time.time()
print(f"Start time: {start}")
time.sleep(1)
end = time.time()
print(f"Elapsed: {end - start:.2f} seconds")


def greet():
    print("Hello from Tobias!")


scheduler.enter(2, 1, greet)
scheduler.run()


text = "Contact Emil at emil@example.com or Tobias at tobias@test.com"
emails = re.findall(r"\S+@\S+", text)
print(emails)

q = queue.Queue()
q.put("task1")
print(q.get())

data = {"users": [{"name": "Emil", "tags": ["admin", "editor"]}]}
pprint(data)

print(os.name)

logger = logging.getLogger("demo")
print(logger.name)

data = {"name": "Emil", "age": 30}
s = json.dumps(data, sort_keys=True)
print(s)
print(json.loads(s)["name"])

sig = hmac.new(b"key", b"Linus", hashlib.sha256).hexdigest()
print(sig)

s = hashlib.sha256(b"Emil").hexdigest()
print(s)


opts, args = getopt.getopt(["-a", "-b", "Linus"], "ab:")
print(dict(sorted(opts)))
print(args)


class Color(Enum):
    RED = 1
    GREEN = 2
    BLUE = 3


print(Color.RED.name)
print(Color.RED.value)


@dataclass
class Point:
    x: int
    y: int


p = Point(1, 2)
print(p)


ini = """[user]\nname = Linus\nport = 8080\n"""
cfg = configparser.ConfigParser()
cfg.read_string(ini)
print(cfg.get("user", "name"))
print(cfg.getint("user", "port"))


def bye():
    print("Goodbye!")


atexit.register(bye)


async def main():
    print("hello")
    await asyncio.sleep(0)
    print("world")


asyncio.run(main())

parser = argparse.ArgumentParser()
parser.add_argument("name")
args = parser.parse_args(["Tobias"])
print(args.name)
