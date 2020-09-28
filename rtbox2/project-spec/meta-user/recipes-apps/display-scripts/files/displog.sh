#!/bin/sh

SOCKET_PATH=/tmp/display_log_socket

for i in $(seq 20)
do
        [ -S $SOCKET_PATH ] && { nc local:$SOCKET_PATH <&0; exit; }
        usleep 100000
done

echo "Socket $SOCKET_PATH not found. Is the scopeserver running?" 1>&2

