#/bin/bash
SERVER_NAME=$1
if [ -z "$SERVER_NAME" ]; then
	echo "Usage: $0 <server_name>"
	exit 1
fi
curl -X GET -H "Host: $SERVER_NAME" localhost:2000