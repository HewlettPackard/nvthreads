#!/usr/bin/python

import os
import sys
import subprocess
import re
import multiprocessing as mp
import time

# activate all cores
allcores = mp.cpu_count()
for cpu in range(1,allcores):
	cmd = 'echo 1 > /sys/devices/system/cpu/cpu'+str(cpu)+'/online'
	os.system(cmd);

cores = 12
all_configs = ['dthread']
btype = 'phoenix'
if btype == 'phoenix':
	all_benchmarks = ['kmeans', 'reverse_index', 'string_match', 'word_count', 'histogram', 'linear_regression', 'matrix_multiply', 'pca']
elif btype == 'parsec':
	all_benchmarks = ['blackscholes', 'canneal', 'dedup', 'ferret', 'streamcluster', 'swaptions']
elif btype == 'all':
	all_benchmarks = ['kmeans', 'reverse_index', 'string_match', 'word_count', 'histogram', 'linear_regression', 'matrix_multiply', 'pca', 'blackscholes', 'canneal', 'dedup', 'ferret', 'streamcluster', 'swaptions']

all_benchmarks.sort()
#all_benchmarks.append('dedup')
#all_benchmarks.append('ferret')
#all_configs = ['pthread', 'dthread']
runs = 3

for b in all_benchmarks:
	print b


if len(sys.argv) == 1:
	print 'Usage: '+sys.argv[0]+' <benchmark names> <config names> <runs>'
	print 'Configs:'
	for c in all_configs:
		print '  '+c
	print 'Benchmarks:'
	for b in all_benchmarks:
		print '  '+b
	sys.exit(1)

benchmarks = []
configs = []

for p in sys.argv:
	if p in all_benchmarks:
		benchmarks.append(p)
	elif p in all_configs:
		configs.append(p)
	elif re.match('^[0-9]+$', p):
		runs = int(p)
	elif p == '8core':
		cores = 8
	elif p == '4core':
		cores = 4
	elif p == '2core':
		cores = 2

if len(benchmarks) == 0:
	benchmarks = all_benchmarks

if len(configs) == 0:
	configs = all_configs


for cpu in range(1,cores):
	cmd = 'echo 1 > /sys/devices/system/cpu/cpu'+str(cpu)+'/online'		
	os.system(cmd)
for cpu in range(cores, allcores):
	cmd = 'echo 0 > /sys/devices/system/cpu/cpu'+str(cpu)+'/online'
	os.system(cmd)

if runs < 4:
        print 'Warning: with fewer than 4 runs per benchmark, all runs are averaged. Request at least 4 runs to discard the min and max runs from the average.'

print 'cores: '+str(cores)+', runs: '+str(runs)
data = {}
try:
	for benchmark in benchmarks:
		data[benchmark] = {}
		for config in configs:
			data[benchmark][config] = []
	
			for n in range(0, runs):
				print 'Running '+str(n)+': '+benchmark+'.'+config
				os.chdir('tests/'+benchmark)
				
				start_time = os.times()[4]
				#print 'make: make'+' eval-'+config+' NCORES='+str(cores)
				p = subprocess.Popen(['make', 'eval-'+config, 'NCORES='+str(cores)], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
				#output = p.stdout.read()
				#print 'cmd: '+output
				p.wait()

				time = os.times()[4] - start_time
				data[benchmark][config].append(time)
				os.chdir('../..')

			mean = (sum(data[benchmark][config])-max(data[benchmark][config])-min(data[benchmark][config]))/(runs-2)
			print benchmark+'.'+config+': '+str(mean)

except:
	print 'Aborted!'
	
print 'benchmark',
for config in configs:
	print '\t'+config,
print

for benchmark in benchmarks:
	print benchmark,
	for config in configs:
		if benchmark in data and config in data[benchmark] and len(data[benchmark][config]) == runs:
			if len(data[benchmark][config]) >= 4:
				mean = (sum(data[benchmark][config])-max(data[benchmark][config])-min(data[benchmark][config]))/(runs-2)
			else:
				mean = sum(data[benchmark][config])/runs
			print '\t'+str(mean),
		else:
			print '\tNOT RUN',
	print

# activate all cores
for cpu in range(1,allcores):
	cmd = 'echo 1 > /sys/devices/system/cpu/cpu'+str(cpu)+'/online'
	os.system(cmd);

