#include <xmmintrin.h>
#define PAGESIZE 512000
#define NUMPERPAGE 512 // # of elements to fit a page

main() {
double a[PAGESIZE], b[PAGESIZE];
for (int kk=0; kk<PAGESIZE; kk++) {
	b[kk]=a[kk];
	}
}
