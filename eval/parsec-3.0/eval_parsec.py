#!/usr/bin/env python

import os
import re
import sys

print (sys.version)

pwd = os.getcwd()

# Benchmarks available to run
all_benchmarks = ['blackscholes', 'canneal', 'dedup', 'ferret', 'streamcluster', 'swaptions']

# Thread libraries
all_configs = ['pthread', 'dthread', 'nvthread']

# Get input arguments
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

print '[Parsec-eval] Running benchmarks: ',
for b in benchmarks:
	print b + ' ',
print 
print '[Parsec-eval] Using threads: ',
for c in configs:
	print c + ' ',
print
data = {}

def simlarge():
	# Let's run benchmarks (simlarge)
	cores = 12
	for bench in benchmarks:
		data[bench] = {}
		print '----------------' + bench + '----------------'
		for config in configs:
			data[bench][config] = []
			if config == 'pthread':
				suffix = ''
			else:
				suffix = '-'+config
			print 'running ' + config + '.' + bench
			if bench == 'blackscholes':
				exe = pwd+'/pkgs/apps/'+bench+'/obj/amd64-linux.gcc/'+bench+suffix
				args = str(cores)
				inp = pwd+'/pkgs/apps/blackscholes/run/in_64K.txt'
				outp = pwd+'/pkgs/apps/blackscholes/run/prices.txt'
				cmd = exe + ' ' + args + ' ' + inp + ' ' + outp
			elif bench == 'canneal':
				exe = pwd+'/pkgs/kernels/'+bench+'/obj/amd64-linux.gcc/'+bench+suffix
				args = ' 12 15000 2000 '
				inp = pwd+'/pkgs/kernels/canneal/run/400000.nets 128'
				cmd = exe + ' ' + args + ' ' + inp + ' '
			elif bench == 'dedup':
				exe = pwd+'/pkgs/kernels/'+bench+'/obj/amd64-linux.gcc/'+bench+suffix
				args = ' -c -p -v -t 12 '
				inp = ' -i '+pwd+'/pkgs/kernels/dedup/run/media.dat'
				outp = ' -o '+pwd+'/pkgs/kernels/dedup/run/output.dat.ddp'
				cmd = exe + ' ' + args + ' ' + inp + ' ' + outp
			elif bench == 'ferret':
				if config != 'pthread':
					suffix = '-pthreads-'+config
				exe = pwd+'/pkgs/apps/'+bench+'/inst/amd64-linux.gcc/bin/'+bench+suffix
				args = pwd+'/pkgs/apps/ferret/run/corel lsh '
				inp = pwd+'/pkgs/apps/ferret/run/queries/ 10 20 12 '
				outp = pwd+'/pkgs/apps/ferret/run/output.txt'
				cmd = exe + ' ' + args + ' ' + inp + ' ' + outp
			elif bench == 'streamcluster':
				exe = pwd+'/pkgs/kernels/streamcluster/obj/amd64-linux.gcc/streamcluster'
				args = ' 10 20 128 16384 16384 1000 none '
				outp = pwd+'/pkgs/kernels/streamcluster/run/output.txt 12'
				cmd = exe + ' ' + args + ' ' + outp 
			elif bench == 'swaptions':
				exe = pwd+'/pkgs/apps/swaptions/obj/amd64-linux.gcc/swaptions '
				args = ' -ns 64 -sm 40000 -nt 12 '
				cmd = exe + ' ' + args

			n = 0
			while n < runs:
				# Run and measure time
				print '[Parsec-eval] Executing: '+cmd
				start_time = os.times()[4]
				rv = os.system(cmd)
				if rv != 0 :
					print 'Error, rerun'
					continue
				time = os.times()[4] - start_time
				print ('\n\n'+bench + '.' + config + '[' + str(n) + ']: ' + str(time)+'\n\n')
				data[bench][config].append(float(time))
				n=n+1
		print ''
	return data

# Print results
def printStats(data):
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


def main():
	data = simlarge()
	printStats(data)
main()
