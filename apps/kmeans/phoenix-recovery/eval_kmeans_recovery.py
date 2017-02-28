#!/usr/bin/python
import os
num_means = 1000
dimension = 3
grid_size = 1000
num_points = {1000000, 10000000, 20000000, 30000000}
#num_points = {1000000}
#start_at = {10}
start_at = {10, 50, 75, 150}
threads = {'nvthread'}
#threads = {'pthread'}
#runs = 10
runs = 1
os.system('rm /tmp/nvlib.crash')

for npoints in sorted(num_points):
   for thread in sorted(threads, reverse=False):
      for start in sorted(start_at):                  
         run = 1
         while run <= runs:
            print '<<<<<<<<<<<<<<<<<<<<< [start] Run ' + str(run) + ' ' + thread + ' p=' + str(npoints) + ', start_at=' + str(start) + ' >>>>>>>>>>>>>>>>>>>>>>'      
            cmd = './kmeans-' + thread + '.o' +  ' -p ' + str(npoints) + ' -s ' + str(grid_size) + ' -c ' + str(num_means)
            if start != 0:
               crash_cmd = cmd + ' -a ' + str(start)
               recovery_cmd = cmd
               # match number of iterations for 1M, 10M, 30M             
#              recovery_cmd = recovery_cmd + ' -a 154 '

               print '(crash at iteration ' + str(start) + '): ' + crash_cmd
               os.system(crash_cmd)
      
               # Pthread recovery flag
               if thread == 'pthread':
                  recovery_cmd = recovery_cmd + ' -r 1'
               print '(recovery from ' + str(start) + '): ' + recovery_cmd
               os.system(recovery_cmd)
            else:               
               if npoints == 20000000:
                  cmd = cmd + ' -a 154'   #20Mmatch number of iterations for 1M, 10M, 30M
               print '(no crash) : ' + cmd;
               os.system(cmd)

            print '<<<<<<<<<<<<<<<<<<<<< [end] ' + thread + ' p=' + str(npoints) + ', start_at=' + str(start) + ' >>>>>>>>>>>>>>>>>>>>>>>>>>>>\n\n'      
            os.system('rm -rf /mnt/ramdisk/nvthreads/*')
            os.system('rm -rf /mnt/ssd2/tmp/*')
            os.system('rm /tmp/nvlib.crash')
            run = run + 1

