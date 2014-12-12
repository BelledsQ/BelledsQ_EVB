#!/bin/sh
while true
do
        counter=`ps | grep rfgateway | grep -v grep|wc -l`
        if [ $counter == 0 ]; then
                echo "rfgateway not run"
                rfgateway &
        fi
		sleep 5
done
~
