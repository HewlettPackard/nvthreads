#!/usr/bin/env python

import os
import re
import sys
import subprocess

print (sys.version)

#input_size = 'simlarge'
input_size = 'native'
runs = 1

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

cores = 12

def sim():
	# Let's run benchmarks (simlarge)
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
				inp = pwd+'/pkgs/apps/blackscholes/run/'
				if input_size == 'simlarge':
					inp += 'in_64K.txt'
				elif input_size == 'native':
					inp += 'in_10M.txt'
				outp = pwd+'/pkgs/apps/blackscholes/run/prices.txt'
				cmd = exe + ' ' + args + ' ' + inp + ' ' + outp

			elif bench == 'canneal':
				exe = pwd+'/pkgs/kernels/'+bench+'/obj/amd64-linux.gcc/'+bench+suffix
				args = str(cores) + ' 15000 2000 '
				inp = pwd+'/pkgs/kernels/canneal/run/'
				if input_size == 'simlarge':
					inp += '400000.nets 128'
				elif input_size == 'native':
					inp += '2500000.nets 6000'
				cmd = exe + ' ' + args + ' ' + inp + ' '

			elif bench == 'dedup':
				exe = pwd+'/pkgs/kernels/'+bench+'/obj/amd64-linux.gcc/'+bench+suffix
				args = ' -c -p -v -t ' + str(cores) 
				inp = ' -i '+pwd+'/pkgs/kernels/dedup/run/'
				if input_size == 'simlarge':
					inp += 'media.dat'
				elif input_size == 'native':
					inp += 'FC-6-x86_64-disc1.iso'
				outp = ' -o '+pwd+'/pkgs/kernels/dedup/run/output.dat.ddp'
				cmd = exe + ' ' + args + ' ' + inp + ' ' + outp

			elif bench == 'ferret':
				if config != 'pthread':
					suffix = '-pthreads-'+config
				exe = pwd+'/pkgs/apps/'+bench+'/inst/amd64-linux.gcc/bin/'+bench+suffix
				args = pwd+'/pkgs/apps/ferret/run/corel lsh '
				inp = pwd+'/pkgs/apps/ferret/run/queries/'
				if input_size == 'simlarge':
					inp += ' 10 20 ' + str(cores) 
				elif input_size == 'native':
					inp += ' 50 20 ' + str(cores)
				outp = pwd+'/pkgs/apps/ferret/run/output.txt'
				cmd = exe + ' ' + args + ' ' + inp + ' ' + outp

			elif bench == 'streamcluster':
				exe = pwd+'/pkgs/kernels/streamcluster/obj/amd64-linux.gcc/streamcluster'
				if input_size == 'simlarge':
					args = ' 10 20 128 16384 16384 1000 none '
				elif input_size == 'native':
					args = ' 10 20 128 1000000 200000 5000 none '
				outp = pwd+'/pkgs/kernels/streamcluster/run/output.txt ' + str(cores)
				cmd = exe + ' ' + args + ' ' + outp 

			elif bench == 'swaptions':
				exe = pwd+'/pkgs/apps/swaptions/obj/amd64-linux.gcc/swaptions'
				if input_size == 'simlarge':
					args = ' -ns 64 -sm 40000 -nt ' + str(cores) 
				elif input_size == 'native':
					args = ' -ns 128 -sm 1000000 -nt ' + str(cores)
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
	print 'benchmark',
	for config in configs :
		print '\t' + config,   
	print
	for bench in benchmarks:
		print (bench),
		for config in configs:
			if bench in data and config in data[bench] and len(data[bench][config]) == runs:
				if len(data[bench][config]) >= 4:
					mean = (sum(data[bench][config])-max(data[bench][config])-min(data[bench][config]))/(runs-2)
				else:
					mean = sum(data[bench][config])/runs
				print ('\t'+str(mean)),
			else:
				print ('\tNOT RUN'),
		print 

# Generate input files using parsec tool
def generateInputs():
	for bench in benchmarks:
		cmd = pwd+'/bin/parsecmgmt -a run -p '+bench+' -i '+input_size+' -n '+str(cores)
		os.system(cmd)

def main():
#	generateInputs()
	data = sim()
#	printStats(data)

main()
