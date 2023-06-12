
#include <stdio.h>
#include "mpi.h"

double f(double x) { return (2*x*x - 0.5 * x + 3); }

int main(int argc, char** argv)
{
	int rank, size, n = 1000000000;
	double sum, part_sum, interval, time, first = 0, second = 4;
	MPI_Status st;
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	if (rank == 0)
	{
		time = MPI_Wtime();
	}

	MPI_Barrier(MPI_COMM_WORLD);
	sum = 0;
	interval = (second - first) / n;
	for (int i = rank; i < n; i += size)
	{
		sum += (double)( 0.5 * interval * (f(first + interval * i) + f(first + interval * (i+1))));
	}
	MPI_Reduce(&part_sum, &sum, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
	if (rank == 0)
	{
		time = MPI_Wtime() - time;
		printf("Значение определенного интеграла = %lf\nВремя работы = %lf секунд\n", sum, time);
	}
	MPI_Finalize();
	return 0;
}
