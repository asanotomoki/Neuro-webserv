# localRedirect.py
# # localRedirect.py - redirect to a new URL on the same server

import os, sys
import hashlib

print("Location: /form")
print("Host: server1")
print("Content-type: text/html\r\n\r\n")
request_method = os.environ['REQUEST_METHOD']
if request_method == "POST":
	body = sys.stdin.read(int(os.environ['CONTENT_LENGTH']))
	# bodyはusername=xxx&password=yyyのような形式で送られてくる
	user_name = body.split("&")[0].split("=")[1]
	password = body.split("&")[1].split("=")[1]
	hash_pass = hashlib.sha256(password.encode()).hexdigest()
	print("user_name=" + user_name)
	print("password=" + hash_pass)