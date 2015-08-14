#!/usr/bin/env python

import os

threads = [1,2,4,8,12]
lines = 948464
cols = 82166
input = '/scratch/bruegner/adj_slashdot_snap'
output = 'build/examples/pagerank-mnemosyne/output.txt'
exe = 'build/examples/pagerank-mnemosyne/pagerank-mnemosyne'
iteration = 10
runs = 10
#target = open(stats, 'w')
def run():
	for th in threads:
		print 'th: '+str(th)
#		stats = 'build/examples/pagerank-mnemosyne/stats_'+str(th)+'.csv'
		nthread = str(th)
		cmd = exe + ' ' + input + ' ' + str(iteration)+ ' '+ str(th) + ' ' + str(lines) + ' ' + str(cols) + ' ' + output
		n= 0
		while n < runs:
			os.system('rm -rf /tmp/*')
			print '------- #threads: '+ str(th)+' Run '+ str(n) + '-------'
			print cmd
			rv = os.system(cmd);
			if rv != 0:
				print 'Error, rerun'
				continue
			n = n + 1

run()
