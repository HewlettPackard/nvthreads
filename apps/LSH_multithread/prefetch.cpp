#include <xmmintrin.h>
#define PAGESIZE 512000
#define NUMPERPAGE 512 // # of elements to fit a page

main() {
double a[PAGESIZE], b[PAGESIZE], temp;
//for (int kk=0;kk<PAGESIZE;kk++) {
//	a[kk]=kk;
//}
for (int kk=0; kk<PAGESIZE; kk+=NUMPERPAGE) {
	temp = a[kk+NUMPERPAGE]; // TLB priming
	for (int j=kk+16; j<kk+NUMPERPAGE; j+=16) {
		_mm_prefetch((char*)&a[j], _MM_HINT_NTA);
	}
	for (int j=kk; j<kk+NUMPERPAGE; j+=16) {
_mm_stream_ps((float*)&b[j],
_mm_load_ps((float*)&a[j]));
/*
_mm_stream_ps((float*)&b[j+2],
_mm_load_ps((float*)&a[j+2]));
_mm_stream_ps((float*)&b[j+4],
_mm_load_ps((float*)&a[j+4]));
_mm_stream_ps((float*)&b[j+6],
_mm_load_ps((float*)&a[j+6]));
_mm_stream_ps((float*)&b[j+8],
_mm_load_ps((float*)&a[j+8]));
_mm_stream_ps((float*)&b[j+10],
_mm_load_ps((float*)&a[j+10]));
_mm_stream_ps((float*)&b[j+12],
_mm_load_ps((float*)&a[j+12]));
_mm_stream_ps((float*)&b[j+14],
_mm_load_ps((float*)&a[j+14]));
*/
}
}
_mm_sfence();
//for (int kk=0;kk<PAGESIZE;kk++) {
//	printf("%f ",b[kk]);
//}
}
