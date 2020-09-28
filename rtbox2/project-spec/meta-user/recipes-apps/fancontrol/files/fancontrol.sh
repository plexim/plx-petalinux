#!/bin/sh
DAEMON=/usr/bin/fancontrol
NAME=fancontrol
DESC="Fan Control"

test -f $DAEMON || exit 0

set -e

case "$1" in
    start)
        echo -n "starting $DESC: $NAME... "
        start-stop-daemon -S -b -a $DAEMON -n $NAME
        echo "done."
        ;;
    stop)
        echo -n "stopping $DESC: $NAME... "
        $DAEMON -K -n $NAME
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


