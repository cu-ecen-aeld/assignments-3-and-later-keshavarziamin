#! /bin/bash

case "$1" in
start)
    echo "starting aesdsocket"
    start-stop-daemon -S -n aesdsocket -d usr/bin/aesdsocket
    ;;
stop)
    echo "stoping aesdsocket"
    start-stop-daemon -K -n aesdsocket 
    ;;
*)
    echo "Usage: $0 {start|stop}"
    exit 1
    ;;
esac
exit 0
