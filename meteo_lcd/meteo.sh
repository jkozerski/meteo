#!/bin/bash
 
### BEGIN INIT INFO
# Default-Start:     2 3 4 5
# Default-Stop:      0 1 6  
# Short-Description: Start/stop foo
### END INIT INFO
 
dir=/home/pi/meteo
user=pi
#config=/home/foo/thin.yml
 
case "$1" in
 
    'start')
        cd $dir
        sudo -u $user rm nohup.out 2>/dev/null ; nohup stdbuf -oL python ./update_meteo.py 2>&1 & echo $! > nohup.pid
        ;;
 
    'stop')
        cd $dir
        sudo -u $user kill -9 $(cat nohup.pid)
        ;;
 
    *)
        echo "Usage: $0 { start | stop }"
        ;;
 
esac
