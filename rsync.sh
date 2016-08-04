rsync -auv --rsh='ssh ' --exclude-from rsync_exclude ../nvthreads hsuchi@$1:/mnt/ssd/terry/workspace
OUT=$?
if [ $OUT -eq 0 ];then
	echo "[rsync completed]"
else
	echo "[rsync error] status: $OUT"
fi


