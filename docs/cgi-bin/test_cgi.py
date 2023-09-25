#!/usr/bin/env python3

import os

pathInfo = os.environ["PATH_INFO"]
print("Content-Type: text/html")
print()
print("<html><body>")
print("<p>Hello, world!</p>")
print("<p>path Info : "+ pathInfo + "</p>")
print("</body></html>")
