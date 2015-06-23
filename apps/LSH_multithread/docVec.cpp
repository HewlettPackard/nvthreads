#include <fvec.h>

float dotVec(float x[], float y[]) {
	F32vec4 *pX = (F32vec4 *) x;
	F32vec4 *pY = (F32vec4 *) y;
	F32vec4 val = pX[0] * pY[0];
	return val[0] + val[1] + val[2] + val[3];
}
