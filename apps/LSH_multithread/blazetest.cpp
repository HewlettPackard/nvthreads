

#include <armadillo>
#include <blaze/math/DynamicMatrix.h>

#include <sys/time.h>
#include "Util.h"

using blaze::DynamicMatrix;

using namespace arma;

int main(int nargs, char **args)
{
	int x = atoi(args[1]);
	int y = atoi(args[2]);
	int z = atoi(args[3]);

	Mat<float> nn(x,y);
	Mat<float> mm(y,z);
	nn.randu(x,y);
	mm.randu(y,z);
        DynamicMatrix<float> a(x,y,nn.memptr());
        DynamicMatrix<float> b(y,z,mm.memptr());


	timespec before, after;
	clock_gettime(CLOCK_MONOTONIC, &before);

	DynamicMatrix<float> c = a*b;

	clock_gettime(CLOCK_MONOTONIC, &after);
        cout << "time taken (nsec)=" << diff(before, after).tv_nsec << endl;

}
