#!/usr/bin/env python3

import datetime
import time

import sys
import os

# CONTENT_LENGTHからデータの長さを取得
request_method = os.environ['REQUEST_METHOD']
time.sleep(3)
if request_method == "POST":
	body = sys.stdin.read(int(os.environ['CONTENT_LENGTH']))

print("HTTP/1.1 200 OK")
print("Content-type: text/html\r\n\r\n")

time.sleep(10)
print("<html>")
print("<head>")

print("</head>")
print("<body>")
print("<h1> Hello World </h1>")
print(datetime.datetime.strftime(datetime.datetime.now(), "<h1>  %H:%M:%S </h1>"))
print("<p>" + request_method + "</p>")
print("<p>")
if request_method == "POST":
	print("body: " + body)
print("</p>")
print("</body>")
print("</html>")
