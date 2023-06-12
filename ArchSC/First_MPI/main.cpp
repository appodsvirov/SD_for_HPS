#include <stdio.h>
#include "mpi.h"

int main(int argc, char **argv)
{	
	const int MAX = 20;
	int rank, size;
	int n, ibeg, iend;

	MPI_Init(&argc, &argv);
	double start = MPI_Wtime();
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	n = (MAX - 1) / size + 1;
	ibeg = rank * n + 1;
	iend = (rank + 1) * n;
	for(int i = ibeg; i <= ((iend > MAX) ? MAX : iend); i++)
	{
		printf("Process: %d, %d^2=%d\n", rank, i, i*i);
	}
	double end = MPI_Wtime();
	MPI_Finalize();

	if (rank == 0)
		printf("Время работы программы в секундах:%lf", start - end);
	return 0;
}
