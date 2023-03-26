#!/bin/bash
for i in 1 2 3 4 5 6 7 8 9 10; do
    echo "run nc ${i}"
    # sleep 2
    nc localhost 9000 &
done

lsof -i:9000