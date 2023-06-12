#include <iostream>
#include <semaphore.h>
#include <random>
#include <cstdlib>
#include <ctime>
#include <unistd.h>
#include "mpi.h"



using namespace std;

//constexpr const int N = 5;  // количество философов
int state;
MPI_Status st;
sem_t semaphore;

int inline left(int rank, int size)
{
	return (rank + size - 1) % size;
}

int inline right(int rank, int size)
{
	return (rank + 1) % size;
}

int my_rand(int rank, int size, int min, int max)
{
	srand((unsigned)time(NULL) + rank * size);
	return rand() % min + max - min;
}

bool test(int rank, int size)
{
	int buf[3]{ 0 }, answer1[2]{ 0 }, answer2[2]{ 0 };
	buf[0] = 0; //проверить состояние левого соседа
	buf[1] = rank;
	buf[2] = left(rank, size);
	MPI_Send(&buf, 3, MPI_INT, 0, 0, MPI_COMM_WORLD);
	MPI_Recv(answer1, 2, MPI_INT, 0, 0, MPI_COMM_WORLD, &st);
	if (answer1[0] == 1) //медиатор остановлен
	{
		return false;
	}

	buf[0] = 0; //проверить состояние правого соседа
	buf[1] = rank;
	buf[2] = right(rank, size);
	MPI_Send(&buf, 3, MPI_INT, 0, 0, MPI_COMM_WORLD);
	MPI_Recv(answer2, 2, MPI_INT, 0, 0, MPI_COMM_WORLD, &st);
	if (answer1[0] == 1) //медиатор остановлен
	{
		return false;
	}

	if (answer1[1] != (int)(2) && answer2[1] != (int)(2))
	{
		buf[0] = 1; //изменить состояние на медиаторе
		buf[1] = rank;
		buf[2] = (int)(2);
		MPI_Send(&buf, 3, MPI_INT, 0, 0, MPI_COMM_WORLD);
		MPI_Recv(answer1, 2, MPI_INT, 0, 0, MPI_COMM_WORLD, &st); //для проверки на конец медиатора
		if (answer1[0] == 1) //медиатор остановлен
		{
			return false;
		}

		//разблокировать процесс с id rank
		sem_post(&semaphore);
	}

	else
	{
		//блокировать процесс с id rank
		sem_wait(&semaphore);
	}

	return true;
}

void mediator(int size)
{
	/* 
	Философ -> Медиатор
		buf[0] - вид запроса: 0 - сообщить состояние, 1 - изменить состояиие
		buf[1] - id получателя пакета
			если buf[0] = 0, то buf[2]  - id соседа,
			если buf[0] = 1, то buf[2] - текущее состояние отправителя

	Медиатор -> Философ
		buf[0] - вид запроса: 0 - ответ на запрос, 1 - прекращение работы медиатора
		buf[1] - состояние философа
	*/
	int philosophers[5] = {0 };
	int buf[3]{ 0 }, answer[2]{ 0 };
	for (double time = MPI_Wtime(); MPI_Wtime() - time < 3;)
	{
		MPI_Recv(buf, 3, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &st);
		if (buf[0] == 0)
		{
			answer[0] = 0;
			answer[1] = (int)(philosophers[buf[2]]);
			MPI_Send(&answer, 2, MPI_INT, st.MPI_SOURCE, 0, MPI_COMM_WORLD);
		}

		else if (buf[0] == 1)
		{
			answer[0] = 0;
			philosophers[buf[1]] = (buf[2]);
			MPI_Send(&answer, 2, MPI_INT, st.MPI_SOURCE, 0, MPI_COMM_WORLD); //ок, статус обновлен, медиатор жив
		}

	}

	MPI_Recv(buf, 3, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &st); //неблокирующий?
	answer[0] = 1;
	for (int j = 1; j != size; j++)
	{
		MPI_Send(&answer, 2, MPI_INT, j, 0, MPI_COMM_WORLD);  //неблокирующий?
	}

	cout << "Конец медиатора\n";
}

void think(int rank, int size)
{
	int duration = my_rand(rank, size, 400, 800);
	cout << rank << " thinks " << duration << "ms\n";
	sleep(duration / 1000);
}

bool take_forks(int rank, int size) //проверить состояние правого соседа
{
	state = 1;
	cout << "\t\t" << rank << " is State::HUNGRY\n";
	int buf[3] = { 1, rank, 1 }, answer[2]{0}; //изменить состояние на медиаторе
	MPI_Send(&buf, 3, MPI_INT, 0, 0, MPI_COMM_WORLD);
	MPI_Recv(answer, 2, MPI_INT, 0, 0, MPI_COMM_WORLD, &st); //для проверки на конец медиатора
	if (answer[0] == 1) //медиатор остановлен
	{
		return false;
	}

	if (test(rank, size) == false) //проверка левого и правого соседей
	{
		return false;
	}
	
	return true;
}

void eat(int rank, int size)
{
	size_t duration = my_rand(rank, size, 400, 800);
	cout << "\t\t\t\t" << rank << " eats " << duration << "ms\n";
	sleep(duration / 1000);
}

bool put_forks(int rank, int size)
{
	state = 0;
	int buf[3] = { 1, rank, 0 }, answer[2]{ 0 }; //изменить состояние на медиаторе
	MPI_Send(&buf, 3, MPI_INT, 0, 0, MPI_COMM_WORLD);
	MPI_Recv(answer, 2, MPI_INT, 0, 0, MPI_COMM_WORLD, &st); //для проверки на конец медиатора
	if (answer[0] == 1)
	{
		return false; //медиатор остановлен
	}

	if (test(left(rank, size), size) == false) //проверка левого и правого соседей от имени левого и правого соседей
	{
		return false;
	}

	if (test(right(rank, size), size) == false)
	{
		return false;
	}

	return true;
}

void philosopher(int rank, int size)
{
	while (true)
	{
		think(rank, size);						// состояние философа: думает
		if (take_forks(rank, size) == false)    // философ берет 2 вилки или блокируется (ожидает)
		{
			return;
		}

		eat(rank, size);					    // философ ест спагетти
		if (put_forks(rank, size) == false)		// философ кладет вилки обратно на стол
		{
			return;
		}
		        
	}

}

int main(int argc, char** argv)
{
	int rank, size;
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	/*sem_init(&semaphore, 0, 1);
	if (rank == 0)
	{
		mediator(size);
	}

	else
	{
		philosopher(rank, size);
	}

	sem_destroy(&semaphore);*/


	cout << my_rand(rank, size, 400, 800);
	MPI_Finalize();
	return 0;
}
#include <iostream>
#include <semaphore.h>
#include <random>
#include <cstdlib>
#include <ctime>
#include <unistd.h>
#include "mpi.h"

using namespace std;

//constexpr const int N = 5;  // количество философов
int state;
MPI_Status st;
sem_t semaphore;

int inline left(int rank, int size)
{
	return (rank + size - 1) % size;
}

int inline right(int rank, int size)
{
	return (rank + 1) % size;
}

int my_rand(int rank, int size, int min, int max)
{
	srand((unsigned)time(NULL) + rank * size);
	return rand() % min + max - min;
}

bool test(int rank, int size)
{
	int buf[3]{ 0 }, answer1[2]{ 0 }, answer2[2]{ 0 };
	buf[0] = 0; //проверить состояние левого соседа
	buf[1] = rank;
	buf[2] = left(rank, size);
	MPI_Send(&buf, 3, MPI_INT, 0, 0, MPI_COMM_WORLD);
	MPI_Recv(answer1, 2, MPI_INT, 0, 0, MPI_COMM_WORLD, &st);
	if (answer1[0] == 1) //медиатор остановлен
	{
		return false;
	}

	buf[0] = 0; //проверить состояние правого соседа
	buf[1] = rank;
	buf[2] = right(rank, size);
	MPI_Send(&buf, 3, MPI_INT, 0, 0, MPI_COMM_WORLD);
	MPI_Recv(answer2, 2, MPI_INT, 0, 0, MPI_COMM_WORLD, &st);
	if (answer1[0] == 1) //медиатор остановлен
	{
		return false;
	}

	if (answer1[1] != (int)(2) && answer2[1] != (int)(2))
	{
		buf[0] = 1; //изменить состояние на медиаторе
		buf[1] = rank;
		buf[2] = (int)(2);
		MPI_Send(&buf, 3, MPI_INT, 0, 0, MPI_COMM_WORLD);
		MPI_Recv(answer1, 2, MPI_INT, 0, 0, MPI_COMM_WORLD, &st); //для проверки на конец медиатора
		if (answer1[0] == 1) //медиатор остановлен
		{
			return false;
		}

		//разблокировать процесс с id rank
		sem_post(&semaphore);
	}

	else
	{
		//блокировать процесс с id rank
		sem_wait(&semaphore);
	}

	return true;
}

void mediator(int size)
{
	/* 
	Философ -> Медиатор
		buf[0] - вид запроса: 0 - сообщить состояние, 1 - изменить состояиие
		buf[1] - id получателя пакета
			если buf[0] = 0, то buf[2]  - id соседа,
			если buf[0] = 1, то buf[2] - текущее состояние отправителя

	Медиатор -> Философ
		buf[0] - вид запроса: 0 - ответ на запрос, 1 - прекращение работы медиатора
		buf[1] - состояние философа
	*/
	int philosophers[5] = {0 };
	int buf[3]{ 0 }, answer[2]{ 0 };
	for (double time = MPI_Wtime(); MPI_Wtime() - time < 3;)
	{
		MPI_Recv(buf, 3, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &st);
		if (buf[0] == 0)
		{
			answer[0] = 0;
			answer[1] = (int)(philosophers[buf[2]]);
			MPI_Send(&answer, 2, MPI_INT, st.MPI_SOURCE, 0, MPI_COMM_WORLD);
		}

		else if (buf[0] == 1)
		{
			answer[0] = 0;
			philosophers[buf[1]] = (buf[2]);
			MPI_Send(&answer, 2, MPI_INT, st.MPI_SOURCE, 0, MPI_COMM_WORLD); //ок, статус обновлен, медиатор жив
		}

	}

	MPI_Recv(buf, 3, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &st); //неблокирующий?
	answer[0] = 1;
	for (int j = 1; j != size; j++)
	{
		MPI_Send(&answer, 2, MPI_INT, j, 0, MPI_COMM_WORLD);  //неблокирующий?
	}

	cout << "Конец медиатора\n";
}

void think(int rank, int size)
{
	int duration = my_rand(rank, size, 400, 800);
	cout << rank << " thinks " << duration << "ms\n";
	sleep(duration / 1000);
}

bool take_forks(int rank, int size) //проверить состояние правого соседа
{
	state = 1;
	cout << "\t\t" << rank << " is State::HUNGRY\n";
	int buf[3] = { 1, rank, 1 }, answer[2]{0}; //изменить состояние на медиаторе
	MPI_Send(&buf, 3, MPI_INT, 0, 0, MPI_COMM_WORLD);
	MPI_Recv(answer, 2, MPI_INT, 0, 0, MPI_COMM_WORLD, &st); //для проверки на конец медиатора
	if (answer[0] == 1) //медиатор остановлен
	{
		return false;
	}

	if (test(rank, size) == false) //проверка левого и правого соседей
	{
		return false;
	}
	
	return true;
}

void eat(int rank, int size)
{
	size_t duration = my_rand(rank, size, 400, 800);
	cout << "\t\t\t\t" << rank << " eats " << duration << "ms\n";
	sleep(duration / 1000);
}

bool put_forks(int rank, int size)
{
	state = 0;
	int buf[3] = { 1, rank, 0 }, answer[2]{ 0 }; //изменить состояние на медиаторе
	MPI_Send(&buf, 3, MPI_INT, 0, 0, MPI_COMM_WORLD);
	MPI_Recv(answer, 2, MPI_INT, 0, 0, MPI_COMM_WORLD, &st); //для проверки на конец медиатора
	if (answer[0] == 1)
	{
		return false; //медиатор остановлен
	}

	if (test(left(rank, size), size) == false) //проверка левого и правого соседей от имени левого и правого соседей
	{
		return false;
	}

	if (test(right(rank, size), size) == false)
	{
		return false;
	}

	return true;
}

void philosopher(int rank, int size)
{
	while (true)
	{
		think(rank, size);						// состояние философа: думает
		if (take_forks(rank, size) == false)    // философ берет 2 вилки или блокируется (ожидает)
		{
			return;
		}

		eat(rank, size);					    // философ ест спагетти
		if (put_forks(rank, size) == false)		// философ кладет вилки обратно на стол
		{
			return;
		}
		        
	}

}

int main(int argc, char** argv)
{
	int rank, size;
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	/*sem_init(&semaphore, 0, 1);
	if (rank == 0)
	{
		mediator(size);
	}

	else
	{
		philosopher(rank, size);
	}

	sem_destroy(&semaphore);*/


	cout << my_rand(rank, size, 400, 800);
	MPI_Finalize();
	return 0;
}
