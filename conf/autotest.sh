pid=$(ps -ef | grep asyncmsg | grep -v grep | awk '{print $2}')
interval=1
echo "pid $pid"

#remove input nodes
while(( 1 ))
do
echo "cp input.ini_less input.ini"
cp input.ini_less input.ini
kill -10 $pid
kill -17 $pid
sleep $interval

echo "cp input.ini_all input.ini"
cp input.ini_all input.ini
kill -10 $pid
kill -17 $pid
sleep $interval

echo "cp output.ini_all output.ini"
cp output.ini_all output.ini
kill -12 $pid
kill -17 $pid
sleep $interval
done
