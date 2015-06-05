Dthreads: Efficient Deterministic Multithreading
------------------------------------------------

Tongping Liu, Charlie Curtsinger, Emery D. Berger  
<tonyliu@cs.umass.edu>  
<charlie@cs.umass.edu>  
<emery@cs.umass.edu>  

<http://plasma.cs.umass.edu>

Copyright (C) 2011-2012 University of Massachusetts Amherst


### Building Dthreads ###

You can build the dthreads library (`libdthread.so`) by running `make`.
That library is for 64-bit systems (the default); the 32-bit version
is called `libdthread32.so`.

*NOTE*: if your system does not support the SSE3 instruction set, then
remove `SSE_SUPPORT` from the Makefile.


### Using Dthreads ###

Dthreads currently only supports Linux/x86 platforms.

1. Compile your program to object files (here, we use just one, `target.o`).

2. Link to the dthreads library. There are two options (neither of which
   is particular to dthreads).

  (a) Dynamic linking: this approach requires no environment variables,
      but the dthreads library needs to be in a fixed, known location.
      Place the dthreads library in a directory (`DTHREAD_DIR`).
      
      Then compile your program as follows:

	% g++ target.o -rdynamic DTHREAD_DIR/libdthread.so -ldl -o target  

  (b) Ordinary dynamic linking: this approach is more flexible (you can
      change the location of the dthreads library), but you must also
      set the `LD_LIBRARY_PATH` environment variable.

	% g++ target.o -LDTHREAD_DIR -ldthread -dl -o target  
	% export LD_LIBRARY_PATH=DTHREAD_DIR:$LD_LIBRARY_PATH  

### Citing Dthreads ###

If you use dthreads, we would appreciate hearing about it. To cite
dthreads, please refer to the following paper, included as
doc/dthreads-sosp11.pdf.

@inproceedings{dthreads,  
 author = {Liu, Tongping and Curtsinger, Charlie and Berger, Emery D. },  
 title = {Dthreads: Efficient Deterministic Multithreading},  
 booktitle = {Proceedings of the 23rd ACM Symposium on Operating Systems Principles},   
 series = {SOSP '11},  
 year = {2011},  
 location = {Cascais, Portugal},  
 numpages = {10},  
 acmid = {1640096},  
 publisher = {ACM},  
}   

### Acknowledgment and Disclaimer ###

This material is based upon work supported by the National Science
Foundation under Grant No. 0910883.

Any opinions, findings and conclusions or recomendations expressed in
this material are those of the author(s) and do not necessarily
reflect the views of the National Science Foundation (NSF).