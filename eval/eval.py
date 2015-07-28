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

print (sys.version)
print '[NVthread-eval] ----------------------------------------------------Welcome-------------------------------------------------------'
print '[NVthread-eval] Usage: ./eval.py action[build/run] benchmark input-size[simlarge/native] thread-type[p/d/nvthread] runs ncore[1/2/4/8/12core]'

log_path = '/mnt/ramdisk/'
MAX_failed = 20
runs = 1
#input_size = 'simsmall'
#input_size = 'simlarge'
input_size = 'native'

pwd = os.getcwd()

# Define becnhmarks directories
#phoenix_benchmarks = ['histogram', 'kmeans', 'linear_regression', 'matrix_multiply', 'pca', 'reverse_index', 'string_match', 'word_count']
phoenix_benchmarks = ['histogram', 'kmeans', 'linear_regression', 'matrix_multiply', 'pca', 'string_match', 'word_count']
parsec_benchmarks = ['blackscholes', 'canneal', 'dedup', 'ferret', 'streamcluster', 'swaptions']
#parsec_benchmarks = ['ferret', 'streamcluster', 'swaptions']

# Benchmarks available to run
all_benchmarks = phoenix_benchmarks + parsec_benchmarks

# Thread libraries
all_configs = ['pthread', 'dthread', 'nvthread']

# All input sizes
all_inputs = ['simlarge', 'native']

# Number of cores on machine (hyper threading)
ht_cores = 24

# Physical cores counts
all_cores = 12

# Get input arguments
benchmarks = []
configs = []
uninstall = 0
build = 0
geninput = 0
run = 1
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
		runs = int(p)
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

if len(benchmarks) == 0:
	benchmarks = all_benchmarks
if len(configs) == 0:
	configs = all_configs

data = {}


def sim():
	print '[NVthread-eval] Input: '+input_size
	print '[NVthread-eval] #cores in use: ' + str(cores)
	print '[NVthread-eval] #runs per setup: ' + str(runs)
	print '[NVthread-eval] Running benchmarks: ',
	for b in benchmarks:
		print b + ' ',
	print 
	print '[NVthread-eval] Using threads: ',
	for c in configs:
		print c + ' ',
	print
	# Let's run benchmarks (simlarge)
	for bench in benchmarks:
		data[bench] = {}
		print '[NVthread-eval] ----------------' + bench + '----------------'
		for config in configs:
			data[bench][config] = []
			print 'running ' + config + '.' + bench
			#------------phoenix------------------
			exe = pwd+'/tests/'+bench+'/'+bench+'-'+config+'.out'
			if bench == 'histogram':
				inp = pwd+'/datasets/histogram_datafiles/large.bmp'
				cmd = exe + ' ' + inp
			elif bench == 'kmeans':
				args = ' -d 3 -c 1000 -p 100000 -s 1000'
				cmd = exe + ' ' + args
			elif bench == 'linear_regression':
				inp = pwd+'/datasets/linear_regression_datafiles/key_file_500MB.txt'
				cmd = exe + ' ' + inp
			elif bench == 'matrix_multiply':
				args = ' 2000 2000'
				cmd = exe + ' ' + args
			elif bench == 'pca':
				args = '  -r 4000 -c 4000 -s 100'
				cmd = exe + ' ' + args
			elif bench == 'reverse_index':
				inp = pwd+'/datasets/reverse_index_datafiles'
				cmd = exe + ' ' + inp
			elif bench == 'string_match':
				inp = pwd+'/datasets/string_match_datafiles/key_file_500MB.txt'
				cmd = exe + ' ' + inp
			elif bench == 'word_count':
				inp = pwd+'/datasets/word_count_datafiles/word_100MB.txt'
				cmd = exe + ' ' + inp
			#------------parsec-------------------
			elif bench == 'blackscholes':
				exe = pwd+'/tests/blackscholes/'+bench+'-'+config+'.out'
				args = str(cores)
				if input_size == 'simlarge':
					inp = pwd+'/datasets/parsec-3.0-sim/parsec-3.0/pkgs/apps/blackscholes/inputs/in_64K.txt'
				elif input_size == 'native':
					inp = pwd+'/datasets/parsec-3.0-native/parsec-3.0/pkgs/apps/blackscholes/inputs/in_10M.txt'
				outp = pwd+'/tests/blackscholes/prices.txt'
				cmd = exe + ' ' + args + ' ' + inp + ' ' + outp
			elif bench == 'canneal':
				nthreads_per_run = cores - 1
				if nthreads_per_run <= 0:
					nthreads_per_run = 1
				exe = pwd+'/tests/canneal/'+bench+'-'+config+'.out'
				args = str(nthreads_per_run) + ' 15000 2000 '
				if input_size == 'simlarge':
					inp = pwd+'/datasets/parsec-3.0-sim/parsec-3.0/pkgs/kernels/canneal/inputs/400000.nets 128'
				elif input_size == 'native':
					inp = pwd+'/datasets/parsec-3.0-native/parsec-3.0/pkgs/kernels/canneal/inputs/2500000.nets 6000'
				cmd = exe + ' ' + args + ' ' + inp + ' '
			# dedup input file path could overflow: use relative path
			elif bench == 'dedup':
				if cores == 12:
					nthreads_per_stage = 2
				else:
					nthreads_per_stage = 1
				exe = pwd+'/tests/dedup/'+bench+'-'+config+'.out'
				args = ' -c -p -f -t ' + str(nthreads_per_stage) 
				if input_size == 'simlarge':
					inp = ' -i '+'./datasets/parsec-3.0-sim/parsec-3.0/pkgs/kernels/dedup/inputs/media.dat'
				elif input_size == 'native':
					inp = ' -i '+'./datasets/parsec-3.0-native/parsec-3.0/pkgs/kernels/dedup/inputs/FC-6-x86_64-disc1.iso'
				outp = ' -o ./tests/dedup/output.dat.ddp'
				cmd = exe + ' ' + args + ' ' + inp + ' ' + outp
			# dedup input file path could overflow: use relative path
			elif bench == 'ferret':
				if cores == 12:
					nthreads_per_stage = 2
				else:
					nthreads_per_stage = 1
				exe = pwd+'/tests/'+bench+'/'+bench+'-'+config+'.out'
				if input_size == 'simlarge':
					args = 'datasets/parsec-3.0-sim/parsec-3.0/pkgs/apps/ferret/inputs/corel lsh '
					inp = 'datasets/parsec-3.0-sim/parsec-3.0/pkgs/apps/ferret/inputs/queries'
					inp += ' 10 20 ' + str(nthreads_per_stage)
				elif input_size == 'native':
					args = 'datasets/parsec-3.0-native/parsec-3.0/pkgs/apps/ferret/inputs/corel lsh '
					inp = 'datasets/parsec-3.0-native/parsec-3.0/pkgs/apps/ferret/inputs/queries'
					inp += ' 50 20 ' + str(nthreads_per_stage)
				outp = 'tests/ferret/output.txt'
				cmd = exe + ' ' + args + ' ' + inp + ' ' + outp
			elif bench == 'streamcluster':
				exe = pwd+'/tests/'+bench+'/'+bench+'-'+config+'.out'
				if input_size == 'simlarge':
					args = ' 10 20 128 16384 16384 1000 none '
				elif input_size == 'native':
					args = ' 10 20 128 1000000 200000 5000 none '
				outp = pwd+'/tests/streamcluster/output.txt ' + str(cores)
				cmd = exe + ' ' + args + ' ' + outp 
			elif bench == 'swaptions':
				exe = pwd+'/tests/'+bench+'/'+bench+'-'+config+'.out'
				if input_size == 'simlarge':
					args = ' -ns 64 -sm 40000 -nt ' + str(cores) 
				elif input_size == 'native':
					args = ' -ns 128 -sm 1000000 -nt ' + str(cores)
				cmd = exe + ' ' + args

#			print '[NVthread-eval] Executing: '+cmd
#			continue

			# Execute command
			n = 0
			failed = 0
			while n < runs:
				# Run and measure time
				print '[NVthread-eval] Executing: '+cmd
				start_time = os.times()[4]
				rv = os.system(cmd)
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
						os.system('find ' + log_path + ' | xargs rm')
					continue
				time = os.times()[4] - start_time
				print ('\n\n'+bench + '.' + config + '[' + str(n) + ']: ' + str(time)+'\n\n')
				data[bench][config].append(float(time))
				n=n+1
				if config == 'nvthread':
					os.system('du -h ' + log_path)
					os.system('find ' + log_path + ' | xargs rm')
		print ''
	return data

# Print results
def printStats(data):
	print '[NVthread-eval] Input: '+input_size
	print '[NVthread-eval] #cores used: ' + str(cores)
	print '[NVthread-eval] #runs per setup: ' + str(runs)
	print '[NVthread-eval] ----------------------------Stats--------------------------------'
	print '[NVthread-eval] benchmark',
	for config in configs :
		print '\t' + config,   
	print
	for bench in benchmarks:
		print '[NVthread-eval] '+ bench,
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
	print '[NVthread-eval] -----------------------------------------------------------------'

def buildBenchmark(benchmark):
		os.chdir('./tests/'+benchmark);
		rv = os.system('make clean; make');
		os.chdir('../../')
		return rv

def buildAll():
	success = {}
	for bench in benchmarks:
		print '[NVthread-eval] Building benchmark: '+bench
		rv = buildBenchmark(bench)
		if rv == 0 :
			success[bench] = 1
		else:
			success[bench] = 0
		print '[NVthread-eval] Built '+bench
	print '[NVthread-eval] ----------------------------Finish building benchmarks:----------------------------'
	for bench in benchmarks :
		print '[NVthread-eval] ' + bench + ': ',
		if success[bench] == 1 :
			print 'succeeded'
		else:
			print 'failed'
	print '[NVthread-eval] -----------------------------------------------------------------'

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
	print '[NVthread-eval] Set CPU done, alive #cores: ' + str(alive_cores)

# Bring all cores back to live (no hyper-threading)
def restoreCPU():
	for cpu in range(1,ht_cores):
		cmd = 'sudo echo 1 > /sys/devices/system/cpu/cpu'+str(cpu)+'/online'
		os.system(cmd);
	time.sleep(2)			# wait for cpus to be back online
	alive_cores = multiprocessing.cpu_count()
	print '[NVthread-eval] Restored CPU, alive #cores: ' + str(alive_cores)

def main():
	start_time = time.time()
	if build == 1:
		buildAll()
	if run == 1:
		restoreCPU()
		setCPU()
		data = sim()
		printStats(data)
		restoreCPU()
	elapsed_time = time.time() - start_time
	print '[NVthread-eval] Finished, time: ' + str(elapsed_time) + ' seconds.'

main()
