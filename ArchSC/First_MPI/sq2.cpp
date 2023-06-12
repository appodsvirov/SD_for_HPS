
#include <iostream>
#include "mpi.h"
using namespace std;

int main(int argc, char **argv)
{
	int rank;
	MPI_Status st;
	char buf[64];
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	if(rank == 0)
	{	
	    string s = "Hello from process 0";
		s.copy(buf, s.length() + 1);
		MPI_Send(buf, 64, MPI_CHAR, 1, 0, MPI_COMM_WORLD);
	}
	else
	{
		MPI_Recv(buf, 64, MPI_CHAR, 1, 0, MPI_COMM_WORLD, &st);
		cout << "Process #" << rank << " received" << buf << "\n";
	}
	
	MPI_Finalize();
	return 0;
}
