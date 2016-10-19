#!/usr/bin/env python

#---Tokyo cabinet---


import os
import getopt
import re
import sys
import subprocess
import multiprocessing
import time
from subprocess import check_output
from multiprocessing import Process

print (sys.version)
print '[TokyoCabinet-eval] ----------------------------------------------------Welcome-------------------------------------------------------'


MAX_failed = 20
start_time = 0
disk = 0
pagedensity = 0
memory_overhead = 0

pwd = os.getcwd()


# Thread libraries
all_configs = ['nvthread']

# Record size
all_recsizes = [8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096]

# Number of records
all_nrecords = [100]

# Number of threads
all_threads = [1, 2, 4, 8]

# write percentage
all_writepercent = [25, 50, 75, 100]

# benchmarks
benchmarks = ['tcbmtsimple']

# Number of cores on machine (hyper threading)
ht_cores = 8

# Physical cores counts
all_cores = 4

# Get input arguments
configs = []
cores = all_cores
configs = all_configs
#threads = all_threads

#nvthreads log location
log_path = '/mnt/ramdisk/nvthreads/'

#database file path
hd_database_path = './'
ssd_database_path = '/mnt/ssd/'
shm_database_path = '/dev/shm/'
nvmfs_database_path = '/mnt/ramdisk/'

#pthreads vs nvthreads database file path
pthread_database_path = shm_database_path
nvthread_database_path = nvmfs_database_path

data = {}

def create_database(nrecord, keysize):
	backend = ['HD', 'SSD', 'TMPFS', 'NVMFS']
	nrecords = 100000
	for b in backend:
		cmd = './tcbcreateworkload ' + b + ' ' + str(nrecord) + ' ' + str(keysize)
		print 'cmd: ' + cmd
		rv = os.system(cmd)

def cleanup():
	os.system('rm -rf ' + log_path + '*')
	os.system('rm /tmp/nvlib.crash')

def sim(runs, thread, nrecord, recbufsize, writepercent):
	print '[TokyoCabinet-eval] #runs per setup: ' + str(runs)
	for bench in benchmarks:
		data[bench] = {}
		for config in all_configs:
			data[bench][config] = []
			run = 0
			if config == 'pthread':
				database = pthread_database_path + 'workload-' + str(nrecord) + 'records-' + str(recbufsize) + 'bytes.bdb'
			else:
				database = nvthread_database_path + 'workload-' + str(nrecord) + 'records-' + str(recbufsize) + 'bytes.bdb'
			while run < runs:
				cleanup()
				start_time = os.times()[4]
				cmd = './' + bench + '_' + config + ' ' + database + ' ' + str(thread) + ' ' + str(nrecord) + ' ' + str(recbufsize) + ' ' + str(writepercent)
				print 'Run: ' + str(run) + ', cmd: ' + cmd
				rv = os.system(cmd)
				time = os.times()[4] - start_time
				if rv != 0:
					print 'Error'
					continue
				data[bench][config].append(float(time))
				run = run + 1
	return data

# print config
def printConfig(runs, thread, nrecord, recbufsize, writepercent):
	print '[TokyoCabinet-eval] #runs per setup: ' + str(runs)
	print '[TokyoCabinet-eval] #threads: ' + str(thread)
	print '[TokyoCabinet-eval] #records: ' + str(nrecord)
	print '[TokyoCabinet-eval] record size: ' + str(recbufsize) + ' bytes'
	print '[TokyoCabinet-eval] writepercent: ' + str(writepercent) + '%'

# Print results
def printStats(runs, thread, nrecord, recbufsize, writepercent, data):
	print '[TokyoCabinet-eval] #runs per setup: ' + str(runs)
	print '[TokyoCabinet-eval] #threads: ' + str(thread)
	print '[TokyoCabinet-eval] #records: ' + str(nrecord)
	print '[TokyoCabinet-eval] record size: ' + str(recbufsize) + ' bytes'
	print '[TokyoCabinet-eval] writepercent: ' + str(writepercent) + '%'
	print '[TokyoCabinet-eval] ----------------------------Stats--------------------------------'
	print '[TokyoCabinet-eval] benchmark',
	for config in configs:
		print '\t' + config,
	print
	for bench in benchmarks:
		print '[TokyoCabinet-eval] ' + bench,
		for config in configs:
			if bench in data and config in data[bench] and len(data[bench][config]) == runs:
				if len(data[bench][config]) >= 4:
					mean = (sum(data[bench][config]) - max(data[bench][config]) - min(data[bench][config])) / (runs - 2)
				else:
					mean = sum(data[bench][config]) / runs
				print ('\t' + str(mean) + '\t'),
			else:
				print ('\tNOT RUN'),
		print
	print '[TokyoCabinet-eval] -----------------------------------------------------------------'
#	writeToFile(delay)

# Set number of online cores (disable hyper-threading)
def setCPU():
	alive_cores = ht_cores
	cores_count = 1 # core 0 is always alive
	for cpu in range(1, ht_cores):
		if cpu % 2 == 0 and cores_count < cores:
			cmd = 'sudo echo 1 > /sys/devices/system/cpu/cpu' + str(cpu) + '/online'
			cores_count = cores_count + 1
		else:
			cmd = 'sudo echo 0 > /sys/devices/system/cpu/cpu' + str(cpu) + '/online'
		os.system(cmd);
	alive_cores = multiprocessing.cpu_count()
	print '[TokyoCabinet-eval] Set CPU done, alive #cores: ' + str(alive_cores)

# Bring all cores back to live (no hyper-threading)
def restoreCPU():
	for cpu in range(1, ht_cores):
		cmd = 'sudo echo 1 > /sys/devices/system/cpu/cpu' + str(cpu) + '/online'
		os.system(cmd);
	time.sleep(2)           # wait for cpus to be back online
	alive_cores = multiprocessing.cpu_count()
	print '[TokyoCabinet-eval] Restored CPU, alive #cores: ' + str(alive_cores)

def main(argv):
	try:
		opts, args = getopt.getopt(argv, "hr:t:c:s:w:", ["runs=","thread=","nrecord","recbufsize","writepercent"])
	except getopt.GetoptError:
		print '[TokyoCabinet-eval] Usage: eval_tokyo.py -r<#runs> -t<#threads> -c<#records> -s<buffersize> -w<write%>'
		sys.exit(2)
	if len(sys.argv) < 6:
		print '[TokyoCabinet-eval] Usage: eval_tokyo.py -r<#runs> -t<#threads> -c<#records> -s<buffersize> -w<write%>'
		sys.exit()
	for opt, arg in opts:
		if opt == '-h':
			print '[TokyoCabinet-eval] Usage: eval_tokyo.py -r<#runs> -t<#threads> -c<#records> -s<buffersize> -w<write%>'
			sys.exit()
		elif opt in ("-r", "--runs"):
			runs = int(arg)
		elif opt in ("-t", "--thread"):
			thread = int(arg)
		elif opt in ("-c", "--nrecord"):
			nrecord = int(arg)
		elif opt in ("-s", "--recbufsize"):
			recbufsize = int(arg)
		elif opt in ("-w", "--writepercent"):
			writepercent = int(arg)
	printConfig(runs, thread, nrecord, recbufsize, writepercent);

	start_time = time.time()
#	restoreCPU()
#	setCPU()
	create_database(nrecord, recbufsize)
	data = sim(runs, thread, nrecord, recbufsize, writepercent)
	printStats(runs, thread, nrecord, recbufsize, writepercent, data)
#	restoreCPU()
	elapsed_time = time.time() - start_time
	print '[TokyoCabinet-eval] Finished, time: ' + str(elapsed_time) + ' seconds.'


main(sys.argv[1:])
