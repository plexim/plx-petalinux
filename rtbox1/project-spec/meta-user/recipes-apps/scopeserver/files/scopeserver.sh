#!/bin/sh
DAEMON=/usr/bin/scopeserver
NAME=scopeserver
DESC="Scope server"
ARGS="--detach"

test -f $DAEMON || exit 0

set -e

case "$1" in
    start)
        echo -n "starting $DESC: $NAME... "
        if [ ! -d $HTTPROOT ]; then
                echo "$HTTPROOT is missing."
                exit 1
        fi
        start-stop-daemon -S -b -n $NAME -a $DAEMON -- $ARGS
        echo "done."
        ;;
    stop)
        echo -n "stopping $DESC: $NAME... "
        start-stop-daemon -K -n $NAME
        echo "done."
        ;;
    restart)
        echo "restarting $DESC: $NAME... "
        $0 stop
        $0 start
        echo "done."
        ;;
    *)
        echo "Usage: $0 {start|stop|restart}"
        exit 1
        ;;
esac

exit 0


