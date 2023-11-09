#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import os
import uuid

# 環境変数からCookieを取得する
cookie_string = os.environ.get('Cookie')
session_id = None
visit_count = 0

if cookie_string:
    # Cookie文字列を解析する簡易的な方法
    cookies = dict(cookie.split('=') for cookie in cookie_string.split('; ') if '=' in cookie)	
    visit_count = int(cookies.get("visit_count", 0))
    session_id = cookies.get("session_id")

# セッションIDがない場合は新たに生成
if not session_id:
    session_id = str(uuid.uuid4())
    visit_count = 1
    print(f"Set-Cookie: session_id={session_id}; visit_count={visit_count}; HttpOnly")  # HttpOnlyフラグを設定

# 訪問回数を更新
else:
    visit_count += 1
    print(f"Set-Cookie: visit_count={visit_count};")


# HTTPヘッダーを出力
print("Content-Type: text/html")
print("\r\n\r\n")  # ヘッダーの終了を意味する空行

# HTMLコンテンツを出力
print(f"<html>")
print(f"<head><title>Python CGI Session</title></head>")
print(f"<body>")
print(f"<h1>Python CGI Session</h1>")
print(f"<p>Visit: {visit_count}</p>")
print(f"<p>Session ID: {session_id}</p>")
print(f"</body>")
print(f"</html>")
