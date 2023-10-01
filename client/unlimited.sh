#!/bin/bash

# 無限ループ
while :
do
    # cURLコマンドの実行
    curl -X POST -H "Content-Type: application/octet-stream" --data-binary "@./test/test1.txt" http://localhost:3000/
    
    # オプショナル: スリープ時間を設定して、次のリクエストまでの間隔を作成します。
    # sleep 1  # 1秒待つ
done
