#!/system/bin/sh
echo "start $1 -p $2 $3 $4"
nohup $1 -p $2 $3 $4 &>/dev/null &
echo "done"
