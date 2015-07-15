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

cores = allcores
#all_configs = ['pthread', 'dthread', 'nvthread']
all_configs = ['pthread', 'dthread']
btype = 'apps'
if btype == 'apps':
	all_benchmarks = ['barnes', 'fmm', 'ocean_cp', 'ocean_ncp', 'radiosity', 'raytrace', 'volrend', 'water_nsquared', 'water_spatial']
#	all_benchmarks = ['barnes']
elif btype == 'kernels':
	all_benchmarks = ['cholesky', 'fft', 'lu_cb', 'lu_ncb', 'radix']

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
pwd = os.getcwd()
for benchmark in benchmarks:
	data[benchmark] = {}
	os.chdir('./splash2/apps/'+benchmark+'/run')
	for config in configs:
		data[benchmark][config] = []
		if config == 'pthread':
			suffix = ''
		else:
			suffix = '-'+config
		executable = pwd+'/splash2/apps/'+benchmark+'/obj/amd64-linux.gcc/'+benchmark+suffix 

		if benchmark == 'barnes':
			input_file = pwd+'/splash2/apps/'+benchmark+'/run/input_' + str(cores)
			cmd = executable + ' ' + str(cores) + ' < ' + input_file
		 # fmm: only works for at most 4 processors
		elif benchmark == 'fmm':
			input_file = pwd+'/splash2/apps/'+benchmark+'/run/input_' + '4'
			cmd = executable + ' ' + '4' + ' < ' + input_file
		 # ocean_cp: only works for power of 2 processes
		elif benchmark == 'ocean_cp':
			cmd = executable + ' -n258 -p' + str(8) + ' -e1e-07 -r20000 -t28800:'
		elif benchmark == 'ocean_ncp':
			cmd = executable + ' -n258 -p' + str(8) + ' -e1e-07 -r20000 -t28800:'
		elif benchmark == 'radiosity':
			cmd = executable + ' -batch -room -p '+ str(cores)
		elif benchmark == 'raytrace':
			input_file = 'teapot.env'
			cmd = executable + ' -s -p'+ str(cores) + input_file
		elif benchmark == 'volrend':
			input_file =  'head-scaleddown4'
			cmd = executable + ' ' + str(cores) + ' ' + input_file
		elif benchmark == 'water_nsquared':
			input_file = 'input_12'
			cmd = executable + ' < ' + input_file
		elif benchmark == 'water_spatial':
			input_file = 'input_12'
			cmd = executable + ' < ' + input_file
		else:
			print 'unknown benchmark: '+benchmark+', abort'
			exit()
		print 'cmd: '+cmd

		n = 0
		while n < runs:
			print '------------'+benchmark+'-'+config+'. run '+str(n)+'-----------------'
			stdout = []			
			start_time = os.times()[4]
			rv = os.system(cmd);
			if rv != 0:
				print 'Error, rerun'
				continue

			time = os.times()[4] - start_time
			print ('\n\n'+benchmark + '.' + config + '[' + str(n) + ']: ' + str(time)+'\n\n')
			data[benchmark][config].append(float(time))
			n = n+1
	os.chdir('../../../..')

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


