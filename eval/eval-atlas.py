#!/usr/bin/env python

#----PARSEC----
#blackscholes: 
#canneal: 
#ferret: dthread
#dedup:
#steamcluster: 
#swaptions: 


import os
import re
import sys
import subprocess
import multiprocessing
import time
import csv
from subprocess import check_output
from multiprocessing import Process

print (sys.version)
print '[Atlas-eval] ----------------------------------------------------Welcome-------------------------------------------------------'
print '[Atlas-eval] Usage: ./eval.py action[build/run] benchmark input-size[simlarge/native] thread-type[p/d/nvthread] runs ncore[1/2/4/8/12core] delays[10/25/50/100/1000(ns)]'

log_path = '/mnt/ramdisk/'
MAX_failed = 20
runs = 1
start_time = 0
disk = 0
pagedensity = 0
memory_overhead = 0
#input_size = 'simsmall'
input_size = 'simlarge'
#input_size = 'native'

pwd = os.getcwd()

datasets_dir = '/mnt/ssd/terry/workspace/nvthreads/eval/datasets'
phoenix_exe_dir = '/mnt/ssd/terry/workspace/Atlas-github/multicore-tests/bench/phoenix-2.0-atlas-all-persist/tests'
parsec_exe_dir = '/mnt/ssd/terry/workspace/Atlas-github/multicore-tests/bench/parsec-3.0-atlas/pkgs'

# Define becnhmarks directories
phoenix_benchmarks = ['histogram', 'kmeans', 'linear_regression', 'matrix_multiply', 'pca', 'reverseindex', 'string_match', 'word_count']
#phoenix_benchmarks = ['histogram', 'kmeans', 'linear_regression', 'matrix_multiply', 'pca', 'string_match']
parsec_benchmarks = ['blackscholes', 'canneal', 'dedup', 'ferret', 'streamcluster', 'swaptions']
#parsec_benchmarks = ['blackscholes', 'canneal', 'dedup', 'streamcluster', 'swaptions']
#parsec_benchmarks = ['ferret', 'streamcluster', 'swaptions']

# Benchmarks available to run
all_benchmarks = phoenix_benchmarks + parsec_benchmarks

# Thread libraries
all_configs = ['atlas']

# All input sizes
all_inputs = ['simlarge', 'native']

# All delays to simulate
all_delays = [200, 250, 500, 550, 700, 750, 1000, 5000, 8000, 10000, 20000, 30000, 40000, 50000]

# Number of cores on machine (hyper threading)
ht_cores = 24

# Physical cores counts
all_cores = 12

# Get input arguments
nvm_delays = []
benchmarks = []
configs = []
uninstall = 0
build = 0
geninput = 0
run = 1
delay = 0
cores = all_cores
for p in sys.argv:
	if p == 'phoenix':
		benchmarks = phoenix_benchmarks
	elif p == 'parsec':
		benchmarks = parsec_benchmarks
	elif p in all_benchmarks:
		benchmarks.append(p)
	elif p in all_configs:
		configs.append(p)
	elif re.match('^[0-9]+$', p):
		tmp = int(p)
		if tmp < 25:
			runs = tmp
		else:
			nvm_delays.append(tmp)
	elif p == '1core':
		cores = 1
	elif p == '2core':
		cores = 2
	elif p == '4core':
		cores = 4
	elif p == '8core':
		cores = 8
	elif p == '12core':
		cores = 12
	elif p == 'build':
		build = 1
		run = 0
	elif p in all_inputs:
		input_size = p
	elif p == 'disk':
		disk = 1
		log_path = '/mnt/ssd/terry/tmp/'
	elif p == 'pagedensity':
		pagedensity = 1

if len(benchmarks) == 0:
	benchmarks = all_benchmarks
if len(configs) == 0:
	configs = all_configs

data = {}

def memloop (cmd, progname):
	memcmd = 'sudo ./ps_mem.py | grep ' + progname 
	print 'memcmd: ' + str(memcmd)
	while 1:
		os.system(memcmd)
		time.sleep(1)
	print 'memloop done, return'
	return

def mysleep (sec):
	time.sleep(sec)

def sim(delay):
	print '[Atlas-eval] Input: '+input_size
	print '[Atlas-eval] #cores in use: ' + str(cores)
	print '[Atlas-eval] #runs per setup: ' + str(runs)
	if delay != 0:
		print '[Atlas-eval] write delay: ' + str(delay) + ' ns'
	print '[Atlas-eval] Running benchmarks: ',
	for b in benchmarks:
		print b + ' ',
	print 
	print '[Atlas-eval] Using threads: ',
	for c in configs:
		print c + ' ',
	print
	# Let's run benchmarks (simlarge)
	for bench in benchmarks:
		data[bench] = {}
		print '[Atlas-eval] ----------------' + bench + '----------------'
		for config in configs:
			data[bench][config] = []
			args = str()
			print 'running ' + config + '.' + bench
			#------------phoenix------------------
			exe = phoenix_exe_dir+'/'+bench+'/'+bench+'-pthread'
			exeps = bench + '-' + config + '.out'
			if bench == 'histogram':
				inp = datasets_dir+'/histogram_datafiles/large.bmp'
				cmd = exe + ' ' + inp
			elif bench == 'kmeans':
				args = ' -d 3 -c 1000 -p 100000 -s 1000'
				cmd = exe + ' ' + args
			elif bench == 'linear_regression':
				inp = datasets_dir + '/linear_regression_datafiles/key_file_500MB.txt'
				cmd = exe + ' ' + inp
			elif bench == 'matrix_multiply':
				args = ' 2000 2000'
				cmd = exe + ' ' + args
			elif bench == 'pca':
				args = '  -r 4000 -c 4000 -s 100'
				cmd = exe + ' ' + args
			elif bench == 'reverseindex':
				inp = datasets_dir + '/reverse_index_datafiles'
				cmd = exe + ' ' + inp
			elif bench == 'string_match':
				inp = datasets_dir + '/string_match_datafiles/key_file_500MB.txt'
				cmd = exe + ' ' + inp
			elif bench == 'word_count':
				inp = datasets_dir + '/word_count_datafiles/word_100MB.txt'
				cmd = exe + ' ' + inp
			#------------parsec-------------------
			elif bench == 'blackscholes':
				exe = parsec_exe_dir+'/apps/blackscholes/inst/amd64-linux.gcc/bin/'+bench
				args = str(cores)
				if input_size == 'simlarge':
					inp = datasets_dir + '/parsec-3.0-sim/parsec-3.0/pkgs/apps/blackscholes/inputs/in_64K.txt'
				elif input_size == 'native':
					inp = datasets_dir + '/parsec-3.0-native/parsec-3.0/pkgs/apps/blackscholes/inputs/in_10M.txt'
				outp = parsec_exe_dir + '/apps/blackscholes/prices.txt'
				cmd = exe + ' ' + args + ' ' + inp + ' ' + outp
			elif bench == 'canneal':
				nthreads_per_run = cores - 1
				if nthreads_per_run <= 0:
					nthreads_per_run = 1
				exe = parsec_exe_dir + '/kernels/canneal/inst/amd64-linux.gcc/bin/'+bench
				args = str(nthreads_per_run) + ' 15000 2000 '
				if input_size == 'simlarge':
					inp = datasets_dir + '/parsec-3.0-sim/parsec-3.0/pkgs/kernels/canneal/inputs/400000.nets 128'
				elif input_size == 'native':
					inp = datasets_dir + '/parsec-3.0-native/parsec-3.0/pkgs/kernels/canneal/inputs/2500000.nets 6000'
				cmd = exe + ' ' + args + ' ' + inp + ' '
			# dedup input file path could overflow: use relative path
			elif bench == 'dedup':
				if cores == 12:
					nthreads_per_stage = 2
				else:
					nthreads_per_stage = 1
				exe = parsec_exe_dir + '/kernels/dedup/inst/amd64-linux.gcc/bin/'+bench
				args = ' -c -p -f -t ' + str(nthreads_per_stage) 
				if input_size == 'simlarge':
					inp = ' -i ' + datasets_dir + '/parsec-3.0-sim/parsec-3.0/pkgs/kernels/dedup/inputs/media.dat'
				elif input_size == 'native':
					inp = ' -i '+ datasets_dir + '/parsec-3.0-native/parsec-3.0/pkgs/kernels/dedup/inputs/FC-6-x86_64-disc1.iso'
				outp = ' -o ' + parsec_exe_dir + '/kernels/dedup/output.dat.ddp'
				cmd = exe + ' ' + args + ' ' + inp + ' ' + outp
			# dedup input file path could overflow: use relative path
			elif bench == 'ferret':
				os.environ['LD_LIBRARY_PATH'] = '/usr/local/lib/' #must install libjpeg7 here
				if cores == 12:
					nthreads_per_stage = 2
				else:
					nthreads_per_stage = 1
				exe = parsec_exe_dir + '/apps/ferret/inst/amd64-linux.gcc/bin/'+bench
				if input_size == 'simlarge':
					args = datasets_dir + '/parsec-3.0-sim/parsec-3.0/pkgs/apps/ferret/inputs/corel lsh '
					inp = datasets_dir + '/parsec-3.0-sim/parsec-3.0/pkgs/apps/ferret/inputs/queries'
					inp += ' 10 20 ' + str(nthreads_per_stage)
				elif input_size == 'native':
					args = datasets_dir + '/parsec-3.0-native/parsec-3.0/pkgs/apps/ferret/inputs/corel lsh '
					inp = datasets_dir + '/parsec-3.0-native/parsec-3.0/pkgs/apps/ferret/inputs/queries'
					inp += ' 50 20 ' + str(nthreads_per_stage)
				outp = parsec_exe_dir + '/apps/ferret/output.txt'
				cmd = exe + ' ' + args + ' ' + inp + ' ' + outp
			elif bench == 'streamcluster':
				exe = parsec_exe_dir + '/kernels/streamcluster/inst/amd64-linux.gcc/bin/'+bench
				if input_size == 'simlarge':
					args = ' 10 20 128 16384 16384 1000 none '
				elif input_size == 'native':
					args = ' 10 20 128 1000000 200000 5000 none '
				outp = parsec_exe_dir + '/kernels/streamcluster/output.txt ' + str(cores)
				cmd = exe + ' ' + args + ' ' + outp 
			elif bench == 'swaptions':
				exe = parsec_exe_dir + '/apps/swaptions/inst/amd64-linux.gcc/bin/'+bench
				if input_size == 'simlarge':
					args = ' -ns 64 -sm 40000 -nt ' + str(cores) 
				elif input_size == 'native':
					args = ' -ns 128 -sm 1000000 -nt ' + str(cores)
				cmd = exe + ' ' + args

#			print '[Atlas-eval] Executing: '+cmd
#			continue

			# Execute command
			n = 0
			failed = 0
			while n < runs:
				# Run and measure time
				print '[Atlas-eval] Executing: '+cmd
				start_time = os.times()[4]

# measure memory overhead, don't care about runtime (not accurate when measuring memory overhead)
				if (memory_overhead == 1):
					p = Process(target=memloop, args=(cmd, exeps))
					p.start()
					mysleep(1)

				rv = os.system(cmd)
# wait for memory overhead thread
				if (memory_overhead == 1):
					p.terminate()

				if rv != 0 :
					print 'Error, rerun'
					killcmd = 'ps ax | grep ' + exe + ' | awk \'{print $1}\' | xargs kill -9'
					os.system(killcmd)
					failed = failed + 1
					if failed >= MAX_failed:
						notif = 'Failed ' + str(failed) + ' times, continue? [y/n] '
#						cont = raw_input(notif)
						if config == 'nvthread':
							os.system('find '+ log_path + ' -name "MemLog*" -print0 | xargs -0 rm ')
							os.system('find '+ log_path + ' -name "varmap*" -print0 | xargs -0 rm ')
							os.system('rm _running; rm _crashed')
#						if cont == str('y'):
#							failed = 0
#						else:
						printStats(data)
						exit()
					if config == 'nvthread':
						os.system('du -h ' + log_path)
						os.system('find ' + log_path + ' | xargs rm -rf')
					continue
				time = os.times()[4] - start_time
				print ('\n\n'+bench + '.' + config + '[' + str(n) + ']: ' + str(time)+'\n\n')
				data[bench][config].append(float(time))
				n=n+1
				if config == 'nvthread':
					os.system('du -h ' + log_path)
					os.system('find ' + log_path + ' | xargs rm -rf')
					os.system('mv /tmp/pagedensity.csv pagedensity/'+bench+'_pagedensity.csv')

		print ''
	return data

# Write results to file
def writeToFile(delay):
	filepath = './stats/'
	if disk != 0:
		filepath = filepath + 'disk_'
	filename = filepath+input_size+'_'+str(cores)+'cores_'+str(runs)+'runs_'+str(delay)+'ns.csv'
	target = open(filename, 'w')
	target.write('Input,'+input_size+'\n')
	target.write('#cores used,' + str(cores)+'\n')
	target.write('#runs per setup,' + str(runs)+'\n')
	target.write('write delay,' + str(delay) + ' ns\n')
	target.write('\n')
	target.write('benchmark,')
	for config in configs :
		target.write('\t' + config + ',')
	target.write('\n')
	for bench in benchmarks:
		target.write(bench + ',')
		for config in configs:
			if bench in data and config in data[bench] and len(data[bench][config]) == runs:
				if len(data[bench][config]) >= 4:
					mean = (sum(data[bench][config])-max(data[bench][config])-min(data[bench][config]))/(runs-2)
				else:
					mean = sum(data[bench][config])/runs
				target.write(str(mean)+',')
			else:
				target.write( 'NOT RUN,')
		target.write('\n')

	elapsed_time = time.time() - start_time
	target.write('\n\nFinished in : ' + str(elapsed_time) + ' seconds\n')

# Print results
def printStats(data, delay):
	print '[Atlas-eval] Input: '+input_size
	print '[Atlas-eval] #cores used: ' + str(cores)
	print '[Atlas-eval] #runs per setup: ' + str(runs)
	if delay != 0:
		print '[Atlas-eval] write delay: ' + str(delay) + ' ns'
	print '[Atlas-eval] ----------------------------Stats--------------------------------'
	print '[Atlas-eval] benchmark',
	for config in configs :
		print '\t' + config,   
	print
	for bench in benchmarks:
		print '[Atlas-eval] '+ bench,
		for config in configs:
			if bench in data and config in data[bench] and len(data[bench][config]) == runs:
				if len(data[bench][config]) >= 4:
					mean = (sum(data[bench][config])-max(data[bench][config])-min(data[bench][config]))/(runs-2)
				else:
					mean = sum(data[bench][config])/runs
				print ('\t'+str(mean)+'\t'),
			else:
				print ('\tNOT RUN'),
		print 
	print '[Atlas-eval] -----------------------------------------------------------------'
	writeToFile(delay)

def buildBenchmark(benchmark):
		os.chdir('./tests/'+benchmark)
		os.system('mkdir obj')
		rv = os.system('make clean; make')
		os.chdir('../../')
		return rv

def buildAll():
	success = {}
	for bench in benchmarks:
		print '[Atlas-eval] Building benchmark: '+bench
		rv = buildBenchmark(bench)
		if rv == 0 :
			success[bench] = 1
		else:
			success[bench] = 0
		print '[Atlas-eval] Built '+bench
	print '[Atlas-eval] ----------------------------Finish building benchmarks:----------------------------'
	for bench in benchmarks :
		print '[Atlas-eval] ' + bench + ': ',
		if success[bench] == 1 :
			print 'succeeded'
		else:
			print 'failed'
	print '[Atlas-eval] -----------------------------------------------------------------'

# Set number of online cores (disable hyper-threading)
def setCPU():
	alive_cores = ht_cores
	cores_count = 1 # core 0 is always alive
	for cpu in range(1,ht_cores):
		if cpu%2 == 0 and cores_count < cores:
			cmd = 'sudo echo 1 > /sys/devices/system/cpu/cpu'+str(cpu)+'/online'
			cores_count = cores_count + 1
		else :
			cmd = 'sudo echo 0 > /sys/devices/system/cpu/cpu'+str(cpu)+'/online'
		os.system(cmd);
	alive_cores = multiprocessing.cpu_count()
	print '[Atlas-eval] Set CPU done, alive #cores: ' + str(alive_cores)

# Bring all cores back to live (no hyper-threading)
def restoreCPU():
	for cpu in range(1,ht_cores):
		cmd = 'sudo echo 1 > /sys/devices/system/cpu/cpu'+str(cpu)+'/online'
		os.system(cmd);
	time.sleep(2)			# wait for cpus to be back online
	alive_cores = multiprocessing.cpu_count()
	print '[Atlas-eval] Restored CPU, alive #cores: ' + str(alive_cores)

# Mount nvmfs
mountPoint = '/mnt/ramdisk'
def mountNVMfs(delay):
	cmd = 'sudo mount -t nvmfs -o rd_delay_ns_fixed=0,wr_delay_ns_fixed='+str(delay)+',rd_delay_ns_per_kb=0,wr_delay_ns_per_kb=0,cpu_freq_mhz=2666,size=1000000m nvmfs ' + mountPoint
	os.system(cmd)
	print '[Atlas-eval] Mounted NVM filesystem with write delay '+str(delay)+' ns at ' + mountPoint

# Umount nvmfs
def umountNVMfs():
	cmd = 'sudo umount '+mountPoint
	os.system(cmd)
	print '[Atlas-eval] Umounted NVM filesystem at '+mountPoint



def main():

	if not nvm_delays:
		nvm_delays.append(0)
	nvm_delays.sort();

	for d in nvm_delays:
		delay = d;
		start_time = time.time()
		if delay != 0:
			mountNVMfs(delay)

		if build == 1:
			buildAll()
		if run == 1:
			restoreCPU()
			setCPU()
			data = sim(delay)
			printStats(data, delay)
			restoreCPU()
		elapsed_time = time.time() - start_time
		print '[Atlas-eval] Finished, time: ' + str(elapsed_time) + ' seconds.'

		if delay != 0:
			umountNVMfs()

main()
