#!/usr/bin/env python
# Run this script at $MNEMOSYNE/usermode
import os
density= [5, 10, 25, 50, 75, 100] # % of page to write to
#configs = ['pt', 'icc']
runs = 10
#density= [75] # % of page to write to
configs = ['atlas']
#runs = 1
pwd = os.getcwd()
exetime = {}
rettime = {}
log_path_nv = '/mnt/ramdisk/'
#log_path_mnemosyne = '/tmp/'
#os.system('find ' + log_path_mnemosyne + ' | xargs rm')
#os.system('rm examples/microbench_output/*.csv')
def sim():
	timeout = 0
	for d in density:
		exetime[d] = {}
		rettime[d] = {}
		for config in configs:
			exetime[d][config] = []
			rettime[d][config] = []
			exe = pwd+'/examples/page_density-'+config+' '+str(d)
			n = 0;
			while n < runs:
				print '--------'+str(n)+' '+config+' '+ str(d) +'%----------'
				start_time = os.times()[4]
				rv = os.system(exe)
				time = os.times()[4] - start_time
				if config == 'nvt':
					os.system('du -h ' + log_path_nv)
					os.system('find ' + log_path_nv + ' | xargs rm')
				elif config == 'mnemosyne':
					os.system('du -h ' + log_path_mnemosyne)
					os.system('find ' + log_path_mnemosyne + ' | xargs rm')
				if rv != 0:
					print 'Error, try again'
#					printStats()
					continue

				if time > 600:
					print '-------time: '+str(time)+' is too slow, ignore results--------'
					timeout = timeout + 1
					continue
				else:
					exetime[d][config].append(float(time))
					rettime[d][config].append(float(rv))
					print config+'['+str(n)+'], '+str(d)+'% time: '+str(time)+', main returns '+str(rv)
				n=n+1
       
def printStats():
	print 'Execution time'
	print 'benchmark\t',
	for config in configs :
		print config + '\t',
	print
	for d in density:
		print str(d)+'%\t',
		for config in configs :
			mean = (sum(exetime[d][config]) - max(exetime[d][config]) - min(exetime[d][config])) / (runs - 2)
			print '\t'+str(mean)+'\t',
		print

def writeToFile():
	filepath = './stats/'
	filename = filepath + 'mnemosyne_pagedensity.csv'
	target = open(filename, 'w')
	target.write('benchmark,')
	for config in configs :
		target.write(config + ',')
	target.write('\n')
	for d in density:
		target.write(str(d)+'%,')
		for config in configs :
			mean = (sum(exetime[d][config]) - max(exetime[d][config]) - min(exetime[d][config])) / (runs - 2)
			target.write(','+str(mean)+',')
		target.write('\n')


def build():
	os.chdir('examples/microbench/')
	os.system('make')
	os.chdir('../../')

def main():
#   build()
	sim()
	printStats()
#	writeToFile()

main()
