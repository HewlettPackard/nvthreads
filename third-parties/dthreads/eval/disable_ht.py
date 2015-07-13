#!/usr/bin/python

import os
import sys
import subprocess
import re
import multiprocessing as mp
import time


allcores = mp.cpu_count()
for cpu in range(1,allcores):
	if cpu%2 == 0 :
		cmd = 'echo 1 > /sys/devices/system/cpu/cpu'+str(cpu)+'/online'
	else:
		cmd = 'echo 0 > /sys/devices/system/cpu/cpu'+str(cpu)+'/online'		   
	os.system(cmd);
sys.exit()

