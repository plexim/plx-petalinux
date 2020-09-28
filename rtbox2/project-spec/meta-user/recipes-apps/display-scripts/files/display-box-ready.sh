#!/bin/sh
PROG=/usr/bin/box-ready-msg
NAME=box-ready-msg
                       
test -f $PROG || exit 0  
                         
set -e                   
            
case "$1" in
    start)                                       
        start-stop-daemon -S -b -n $NAME -a $PROG           
        ;;                                                  
    stop)                                                   
        start-stop-daemon -K -n $NAME                       
        ;;                                                  
    *)                                       
        echo "Usage: $0 {start|stop}"
        exit 1                               
        ;;                                   
esac

exit 0

