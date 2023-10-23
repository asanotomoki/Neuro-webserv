#!/usr/bin/env python3

import datetime
import time

import sys
import os

# CONTENT_LENGTHからデータの長さを取得
request_method = os.environ['REQUEST_METHOD']

if request_method == "POST":
	content_length = int(os.environ['CONTENT_LENGTH'])
	body = sys.stdin.read(content_length)

print("HTTP/1.1 200 OK")
print("Content-Length: 100")
print("Content-type: text/html\r\n\r\n")


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
