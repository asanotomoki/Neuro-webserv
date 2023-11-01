#!/usr/bin/env python3

import datetime
import time

import sys
import os

# CONTENT_LENGTHからデータの長さを取得
request_method = os.environ['REQUEST_METHOD']
if request_method == "POST":
	body = sys.stdin.read(int(os.environ['CONTENT_LENGTH']))

path_info = os.environ['PATH_INFO']
print("HTTP/1.1 200 OK")
print("Content-type: text/html\r\n\r\n")

time.sleep(1)
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


print("<p> path info" + path_info + "</p>")
print("</body>")
print("</html>")
# 無限ループ
while True:
	print("<p> " + datetime.datetime.strftime(datetime.datetime.now(), "%H:%M:%S") + "</p>")
	
