#!/usr/bin/env python

import subprocess32
import os
import sys
import re
import multiprocessing as mp
import time
import threading

print (sys.version)

os.system("rm /mnt/tmpfs/*")

# activate all cores
allcores = mp.cpu_count()

cores = 12
all_configs = ['pthread', 'dthread', 'nvthread']
#all_configs = ['pthread']
btype = 'phoenix'
if btype == 'phoenix':
	all_benchmarks = ['kmeans', 'reverse_index', 'string_match', 'word_count', 'histogram', 'linear_regression', 'matrix_multiply', 'pca']
elif btype == 'parsec':
	all_benchmarks = ['blackscholes', 'canneal', 'streamcluster', 'swaptions', 'dedup', 'ferret']
elif btype == 'all':
	all_benchmarks = ['kmeans', 'reverse_index', 'string_match', 'word_count', 'histogram', 'linear_regression', 'matrix_multiply', 'pca', 'blackscholes', 'canneal', 'dedup', 'ferret', 'streamcluster', 'swaptions']
elif btype == 'defined':
	all_benchmarks = ['streamcluster', 'swaptions', 'canneal', 'blackscholes', 'word_count', 'reverse_index']
runs = 10

benchmarks = []
configs = []

for p in sys.argv:
	if p in all_benchmarks:
		benchmarks.append(p)
	elif p in all_configs:
		configs.append(p)
	elif re.match('^[0-9]+$', p):
		runs = int(p)

if len(benchmarks) == 0:
	benchmarks = all_benchmarks

if len(configs) == 0:
	configs = all_configs


for b in benchmarks:
	print (b)
for cf in configs:
	print ('\t'+cf)


print ('cores: '+str(cores)+', runs: '+str(runs))
data = {}
minutes = 0.1
clock_timeout = minutes * 60

for benchmark in benchmarks:
	data[benchmark] = {}
	for config in configs:
		data[benchmark][config] = []
		n = 0
		while n < runs:
			stdout = []
			os.chdir('tests/'+benchmark)
			#cmd = ['make ', 'eval-'+str(config), ' NCORES=', str(cores)]
			cmd = 'make eval-' + str(config) + ' NCORES=' + str(cores)
            #print cmd
			start_time = os.times()[4]
			#proc = subprocess32.Popen(cmd, stdout=subprocess32.PIPE, stderr=subprocess32.STDOUT, shell=True)				
			#proc = subprocess32.Popen(cmd, shell=True)				
			os.system(cmd);
            #detect timeout
			#try:
				#proc.wait(timeout = clock_timeout)
			#except subprocess32.TimeoutExpired:
				#print '---------Run ' + str(n) + ' timed out!-------------'
				#proc.kill()
				#if config == 'nvthread':
					#os.system('rm /mnt/tmpfs/*')
					#os.system('rm _crashed; rm _running')
				#os.chdir('../..')
				#continue

			time = os.times()[4] - start_time			
			print (benchmark + '.' + config + '[' + str(n) + ']: ' + str(time))
			data[benchmark][config].append(float(time))
			os.chdir('../..')
			n = n+1

print 'benchmark',
for config in configs :
	print '\t' + config,   
print

for benchmark in benchmarks:
	print (benchmark),
	for config in configs:
		if benchmark in data and config in data[benchmark] and len(data[benchmark][config]) == runs:
			if len(data[benchmark][config]) >= 4:
				mean = (sum(data[benchmark][config])-max(data[benchmark][config])-min(data[benchmark][config]))/(runs-2)
			else:
				mean = sum(data[benchmark][config])/runs
			print ('\t'+str(mean)),
		else:
			print ('\tNOT RUN'),
	print 


