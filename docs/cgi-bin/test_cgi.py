#!/usr/bin/env python3

import datetime
import time
import os
import sys

method = os.environ["REQUEST_METHOD"]
if method == "POST":
	length = int(os.environ["CONTENT_LENGTH"])
	body = sys.stdin.read(length)
	

print("HTTP/1.1 200 OK")
print("Content-type: text/html\r\n\r\n")
print("<html>")
#time.sleep(3)
print("<head>")
print("</head>")
print("<body>")
print("<h1> Hello World! </h1>")
print(datetime.datetime.strftime(datetime.datetime.now(), "<h1>  %H:%M:%S </h1>"))
print("<h1> Method: {} </h1>".format(method))
if method == "POST":
	print("<h1> Body: {} </h1>".format(body))
print("</body>")
print("</html>")
