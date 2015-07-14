### Readme for Kmeans and Datagen ###

-- 1. Intro --------------------------------------------------------------------------------------------

Run Make to build the datagenerator ('datagen') and the (multithreaded) kmeans: 'kmeans-mt-pt' for the pthreads-library, 'kmeans-mt-dt' for dthreads.
Make sure that you built dthreads first! The libdthread.so should be located in the directory third-parties/dthreads/lib, which is guaranteed if you execute Make in the dthreads/src-directory.

To get information about the usage of datagen and kmeans-mt-(pt/dt) run them without parameters.

Find the datagenerator including an explanation here: github.com/hbrgnr/datagenerator.

-- 2. Generating data --------------------------------------------------------------------------------------------

To use the datagenerator, run ./datagen with the following parameters:

./datagen <int|double> <rows> <columns> <offset> <range> <seed> <output-file>

<int|double>		write 'int' to generate int-, 'double' to generate double-data
<rows>			specifies the number of rows
<columns>		specifies the number of columns
<offset> and <range>	numbers are generated within the range from (offset - range) to (offset + range)
<seed>			specify a seed for the generator
<output-file>		path to the output file

-- 3. Running Kmeans --------------------------------------------------------------------------------------------

Execute ./kmeans-mt-(pt/dt) with the following parameters:

./kmeans-mt-(pt/dt) <x-input-file> <centers-input-file> <threads> <iterations> <sync-mode (2 is recommended)> <log-mode (1 is recommended)> [<mapping-output-file>]

<x-input-file>		path to the dataset
<centers-input-file>	path to the centers-file, IMPORTANT: assert(x.colums == centers.columns)
<threads>		no. of threads: if set to -1, the REAL singlethreaded version is executed (in both kmeans-mt-pt and -dt)
<iterations>		no. of iterations
<sync-mode>		specifies how the labels are written into the global array (2 is recommended: each thread writes to the global array before exit using locks)
<log-mode>		specifies what is written to the screen (1 is recommended: logging just time and errors)
<mapping-output-file>	(optional) path to the output file that maps x-input to calculated center; centers will be written on screen if not specified

Known issues: implement centers-output-file
