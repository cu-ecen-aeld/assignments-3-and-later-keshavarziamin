#!/bin/bash
PIDnc=$(pgrep nc)
kill $PIDnc
PID=$(pgrep aesdsocket)
kill $PID
