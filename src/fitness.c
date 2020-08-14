#include <math.h>

#define A 10	/* arbitrary constant, default for rastrigin */
#define n 50	/* number of dimensions */

/* define here the fitness function that will be used by all algorithms of parallel_evolution */
/* fitness function for generalized rastrigin function */
double objective_function(double *x)
{
	double sum = 0;
	int i;

	for (i = 0; i < n; ++i)
		sum += x[i] * x[i] - A * cos(2 * M_PI * x[i]);

	return A * n + sum;
}

double (*objective_function_p)(double*) = objective_function;
