#rsync -auv --rsh='ssh -p 9999 ' --exclude-from rsync_exclude ../nvthreads terry@localhost:/home/terry/workspace/
rsync -auv --exclude-from rsync_exclude ../nvthreads $1@$2:$3
OUT=$?
if [ $OUT -eq 0 ];then
	echo "[rsync completed]"
else
	echo "[rsync error] status: $OUT"
fi


