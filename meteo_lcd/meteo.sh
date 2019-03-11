#!/bin/bash
 
### BEGIN INIT INFO
# Provides:          update_meteo
# Required-Start:    $remote_fs $syslog
# Required-Stop:     $remote_fs $syslog
# Default-Start:     2 3 4 5
# Default-Stop:      0 1 6  
# Short-Description: Start/stop update_meteo script
### END INIT INFO
 
dir=/home/pi/meteo
user=pi
 
case "$1" in
 
    'start')
        cd $dir
        nohup stdbuf -oL python ./update_meteo.py > nohup.out 2>&1 & echo $! > nohup.pid
        ;;
 
    'stop')
        cd $dir
        kill $(cat nohup.pid)
        ;;
 
    *)
        echo "Usage: $0 { start | stop }"
        ;;
 
esac
