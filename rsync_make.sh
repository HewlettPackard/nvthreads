rsync -auv --rsh='ssh ' --exclude-from rsync_exclude ../nvthreads hsuchi@120-1.bfc.labs.hpecorp.net:/mnt/ssd/terry/workspace
OUT=$?
if [ $OUT -eq 0 ];then
	echo "[rsync completed]"
else
	echo "[rsync error] status: $OUT"
fi

ssh -f  hsuchi@120-1.bfc.labs.hpecorp.net  "cd /mnt/ssd/terry/workspace/nvthreads/src; make -j 12; wait $!"
OUT=$?
if [ $OUT -eq 0 ];then
	echo "[Make] Remote received commad."
else
	echo "[Make] error with $OUT."
fi

