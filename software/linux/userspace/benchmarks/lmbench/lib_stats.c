#include <math.h>
#include "bench.h"

#define BOOTSTRAP_COUNT	200

int
int_compare(const void *a, const void *b)
{
	if (*(int*)a < *(int*)b) return -1;
	if (*(int*)a > *(int*)b) return 1;
	return 0;
}

int
uint64_compare(const void *a, const void *b)
{
	if (*(uint64*)a < *(uint64*)b) return -1;
	if (*(uint64*)a > *(uint64*)b) return  1;
	return 0;
}

int
double_compare(const void *a, const void *b)
{
	if (*(double*)a < *(double*)b) return -1;
	if (*(double*)a > *(double*)b) return 1;
	return 0;
}

int
int_median(int *values, int size)
{
	qsort(values, size, sizeof(int), int_compare);

	if (size == 0) return 0.;

	if (size % 2) {
	    return values[size/2];
	}

	return (values[size/2 - 1] + values[size/2]) / 2;
}

uint64
uint64_median(uint64 *values, int size)
{
	qsort(values, size, sizeof(uint64), uint64_compare);

	if (size == 0) return 0.;

	if (size % 2) {
	    return values[size/2];
	}

	return (values[size/2 - 1] + values[size/2]) / 2;
}

double
double_median(double *values, int size)
{
	qsort(values, size, sizeof(double), double_compare);

	if (size == 0) return 0.;

	if (size % 2) {
	    return values[size/2];
	}

	return (values[size/2 - 1] + values[size/2]) / 2.0;
}

int
int_mean(int *values, int size)
{
	int	i;
	int	sum = 0;

	for (i = 0; i < size; ++i)
		sum += values[i];

	return sum / size;
}

uint64
uint64_mean(uint64 *values, int size)
{
	int	i;
	uint64	sum = 0;

	for (i = 0; i < size; ++i)
		sum += values[i];

	return sum / size;
}

double
double_mean(double *values, int size)
{
	int	i;
	double	sum = 0.0;

	for (i = 0; i < size; ++i)
		sum += values[i];

	return sum / (double)size;
}

int
int_min(int *values, int size)
{
	int	i;
	int	min = values[0];

	for (i = 1; i < size; ++i)
		if (values[i] < min) min = values[i];

	return min;
}

uint64
uint64_min(uint64 *values, int size)
{
	int	i;
	uint64	min = values[0];

	for (i = 1; i < size; ++i)
		if (values[i] < min) min = values[i];

	return min;
}

double
double_min(double *values, int size)
{
	int	i;
	double	min = values[0];

	for (i = 1; i < size; ++i)
		if (values[i] < min) min = values[i];

	return min;
}

int
int_max(int *values, int size)
{
	int	i;
	int	max = values[0];

	for (i = 1; i < size; ++i)
		if (values[i] > max) max = values[i];

	return max;
}

uint64
uint64_max(uint64 *values, int size)
{
	int	i;
	uint64	max = values[0];

	for (i = 1; i < size; ++i)
		if (values[i] > max) max = values[i];

	return max;
}

double
double_max(double *values, int size)
{
	int	i;
	double	max = values[0];

	for (i = 1; i < size; ++i)
		if (values[i] > max) max = values[i];

	return max;
}

double	int_variance(int *values, int size)
{
	int	i;
	double	sum = 0.0;
	int	mean = int_mean(values, size);

	for (i = 0; i < size; ++i)
		sum += (double)((values[i] - mean) * (values[i] - mean));

	return sum / (double)(size - 1);
}

double	uint64_variance(uint64 *values, int size)
{
	int	i;
	double	sum = 0.0;
	uint64	mean = uint64_mean(values, size);

	for (i = 0; i < size; ++i)
		sum += (double)((values[i] - mean) * (values[i] - mean));
	return sum / (double)(size - 1);
}

double	double_variance(double *values, int size)
{
	int	i;
	double	sum = 0.0;
	double	mean = double_mean(values, size);

	for (i = 0; i < size; ++i)
		sum += (double)((values[i] - mean) * (values[i] - mean));

	return sum / (double)(size - 1);
}

double	int_moment(int moment, int *values, int size)
{
	int	i, j;
	double	sum = 0.0;
	int	mean = int_mean(values, size);

	for (i = 0; i < size; ++i) {
		double diff = values[i] - mean;
		double m = diff;
		for (j = 1; j < moment; ++j)
			m *= diff;
		sum += m;
	}

	return sum / (double)size;
}

double	uint64_moment(int moment, uint64 *values, int size)
{
	int	i, j;
	double	sum = 0.0;
	uint64	mean = uint64_mean(values, size);

	for (i = 0; i < size; ++i) {
		double diff = values[i] - mean;
		double m = diff;
		for (j = 1; j < moment; ++j)
			m *= diff;
		sum += m;
	}

	return sum / (double)size;
}

double	double_moment(int moment, double *values, int size)
{
	int	i, j;
	double	sum = 0.0;
	double	mean = double_mean(values, size);

	for (i = 0; i < size; ++i) {
		double diff = values[i] - mean;
		double m = diff;
		for (j = 1; j < moment; ++j)
			m *= diff;
		sum += m;
	}

	return sum / (double)size;
}

double	int_stderr(int *values, int size)
{
	return sqrt(int_variance(values, size));
}

double	uint64_stderr(uint64 *values, int size)
{
	return sqrt(uint64_variance(values, size));
}

double	double_stderr(double *values, int size)
{
	return sqrt(double_variance(values, size));
}

double	int_skew(int *values, int size)
{
	double	sigma = int_stderr(values, size);
	double	moment3 = int_moment(3, values, size);

	return moment3 / (sigma * sigma * sigma);
}

double	uint64_skew(uint64 *values, int size)
{
	double	sigma = uint64_stderr(values, size);
	double	moment3 = uint64_moment(3, values, size);

	return moment3 / (sigma * sigma * sigma);
}

double	double_skew(double *values, int size)
{
	double	sigma = double_stderr(values, size);
	double	moment3 = double_moment(3, values, size);

	return moment3 / (sigma * sigma * sigma);
}

double	int_kurtosis(int *values, int size)
{
	double	variance = int_variance(values, size);
	double	moment4 = int_moment(4, values, size);

	return moment4 / (variance * variance) - 3;
}

double	uint64_kurtosis(uint64 *values, int size)
{
	double	variance = uint64_variance(values, size);
	double	moment4 = uint64_moment(4, values, size);

	return moment4 / (variance * variance) - 3;
}

double	double_kurtosis(double *values, int size)
{
	double	variance = double_variance(values, size);
	double	moment4 = double_moment(4, values, size);

	return moment4 / (variance * variance) - 3;
}


double	int_bootstrap_stderr(int *values, int size, int_stat f)
{
	int	i, j;
	int    *samples = (int*)malloc(size * sizeof(int));
	double *s = (double*)malloc(BOOTSTRAP_COUNT * sizeof(double));
	double	s_sum = 0;
	double	sum = 0;

	for (i = 0; i < BOOTSTRAP_COUNT; ++i) {
		for (j = 0; j < size; ++j)
			samples[j] = values[rand() % size];
		s[i] = (double)(*f)(samples, size);
		s_sum += s[i];
	}
	s_sum /= (double)BOOTSTRAP_COUNT;
	
	for (i = 0; i < BOOTSTRAP_COUNT; ++i) 
		sum += (s[i] - s_sum) * (s[i] - s_sum);

	sum /= (double)(BOOTSTRAP_COUNT - 1);

	free(samples);
	free(s);

	return sqrt(sum);
}

double	uint64_bootstrap_stderr(uint64 *values, int size, uint64_stat f)
{
	int	i, j;
	uint64 *samples = (uint64*)malloc(size * sizeof(uint64));
	double *s = (double*)malloc(BOOTSTRAP_COUNT * sizeof(double));
	double	s_sum;
	double	sum;

	for (i = 0, s_sum = 0.0; i < BOOTSTRAP_COUNT; ++i) {
		for (j = 0; j < size; ++j) 
			samples[j] = values[rand() % size];
		s[i] = (double)(*f)(samples, size);
		s_sum += s[i];
	}
	s_sum /= (double)BOOTSTRAP_COUNT;
	
	for (i = 0, sum = 0.0; i < BOOTSTRAP_COUNT; ++i) 
		sum += (s[i] - s_sum) * (s[i] - s_sum);

	free(samples);
	free(s);

	return sqrt(sum/(double)(BOOTSTRAP_COUNT - 1));
}

double	double_bootstrap_stderr(double *values, int size, double_stat f)
{
	int	i, j;
	double *samples = (double*)malloc(size * sizeof(double));
	double *s = (double*)malloc(BOOTSTRAP_COUNT * sizeof(double));
	double	s_sum = 0;
	double	sum = 0;

	for (i = 0; i < BOOTSTRAP_COUNT; ++i) {
		for (j = 0; j < size; ++j) 
			samples[j] = values[rand() % size];
		s[i] = (*f)(samples, size);
		s_sum += (double)s[i];
	}
	s_sum /= (double)BOOTSTRAP_COUNT;
	
	for (i = 0; i < BOOTSTRAP_COUNT; ++i) 
		sum += (s[i] - s_sum) * (s[i] - s_sum);

	sum /= (double)(BOOTSTRAP_COUNT - 1);

	free(samples);
	free(s);

	return sqrt(sum);
}


void
regression(double *x, double *y, double *sig, int n,
	   double *a, double *b, double *sig_a, double *sig_b, 
	   double *chi2)
{
	int	i;
	double	S = 0.0, Sx = 0.0, Sy = 0.0, Stt = 0.0, Sx_S;

	for (i = 0; i < n; ++i) {
		double	weight = 1.0 / (sig ? sig[i] * sig[i] : 1.0);
		S += weight;
		Sx += weight * x[i];
		Sy += weight * y[i];
	}

	*b = 0.0;
	Sx_S = Sx / S;
	for (i = 0; i < n; ++i) {
		double t_i = (x[i] - Sx_S) / (sig ? sig[i] : 1.0);
		Stt += t_i * t_i;
		*b  += t_i * y[i] / (sig ? sig[i] : 1.0);
	}

	*b /= Stt;
	*a = (Sy - *b * Sx) / S;
	*sig_a = sqrt((1.0 + (Sx * Sx) / (S * Stt)) / S);
	*sig_b = sqrt(1.0 / Stt);

	*chi2 = 0.0;
	for (i = 0; i < n; ++i) {
		double merit = (y[i] - ((*a) + (*b) * x[i])) / (sig ? sig[i] : 1.0);
		*chi2 += merit * merit;
	}
	if (sig == NULL) {
	  *sig_a *= sqrt((*chi2) / (n - 2));
	  *sig_b *= sqrt((*chi2) / (n - 2));
	}
}

