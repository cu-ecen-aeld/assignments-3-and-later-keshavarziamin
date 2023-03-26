#! /bin/bash

case "$1" in
start)
    echo "starting aesdsocket"
    start-stop-daemon -S -n aesdsocket -d
    ;;
stop)
    echo "stoping aesdsocket"
    start-stop-daemon -K -n aesdsocket usr/bin/aesdsocket
    ;;
*)
    echo "Usage: $0 {start|stop}"
    exit 1
    ;;
esac
exit 0
