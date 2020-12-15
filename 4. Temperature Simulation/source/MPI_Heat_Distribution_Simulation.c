/* Paralleled heat distribution simulation via MPI static assignment */

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include "mpi.h"
#include "const.h"
#include "display.h"
#include "models.h"

const int ROOT = 0;

TemperatureField *field, *tempField;

/* default preference */
int NUM_PROCESSES = 4;  // default amount of processes
int x = 100;  // number of rows
int y = 100;  // number of columns
int MAX_ITERATIONS = 1000;  // max iterations
int X11_ENABLE = 0;  // 1 for enabling display

/* Neighbor offset: Right, Up, Left, Bsottom */
int dx[4] = { 0, -1, 0, 1 };
int dy[4] = { 1, 0, -1, 0 };

int chunk_height;

void temperature_iterate(int chunk_idx)
{
	int i, j, d;
	for (i = chunk_idx; i < chunk_idx + chunk_height; i++)
		if (legal(i, x))
			for (j = 0; j < field->y / 2 + 1; j++)  // make use of symmetric
			{
				int cnt = 0;
				tempField->t[i][j] = 0;
				for (d = 0; d < 4; ++d)  // go through its neighbours
					if (legal(i + dx[d], field->x) && legal(j + dy[d], field->y))
					{
						tempField->t[i][j] += field->t[i + dx[d]][j + dy[d]];
						++cnt;
					}
				tempField->t[i][j] /= cnt;
				tempField->t[i][field->y - j] = tempField->t[i][j];  // copy the temperature from left half to right half
			}
}

int main(int argc, char **argv)
{
	/* measure time interval */
	struct timespec start, end;
	double total_time;

	int rank, chunk_idx, iteration;

	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &NUM_PROCESSES);

	/* modify preference */
	if (rank == ROOT && argc < 5)
	{
		printf("Hint: Execute with mpiexec -oversubscribe -np NUM_PROCESSES %s x y MAX_ITERATIONS X11_ENABLE\n", argv[0]);
		MPI_Finalize();
		exit(EXIT_FAILURE);
	}
	sscanf(argv[1], "%d", &x);
	sscanf(argv[2], "%d", &y);
	sscanf(argv[3], "%d", &MAX_ITERATIONS);
	sscanf(argv[4], "%d", &X11_ENABLE);

	field = (TemperatureField*)malloc(sizeof(TemperatureField));
	tempField = (TemperatureField*)malloc(sizeof(TemperatureField));
	chunk_height = x % NUM_PROCESSES == 0 ? x / NUM_PROCESSES : x / NUM_PROCESSES + 1;  // calculate the workload of each process
	
	newField(field, chunk_height * NUM_PROCESSES, y, 0, 0);
	newField(tempField, chunk_height * NUM_PROCESSES, y, 0, 0);
	initField(field);

	if (rank == ROOT)
	{
		if (X11_ENABLE)
		{
			XWindow_Init(field);
		}

		/* set start timestamp */
		clock_gettime(CLOCK_MONOTONIC, &start);
	}

	chunk_idx = rank * chunk_height;  // calculate the chunk index
	for (iteration = 0; iteration < MAX_ITERATIONS; iteration++)
	{
		temperature_iterate(chunk_idx);
		/* scatter the whole field to all processes and gather their computation result to update the whole field */
		MPI_Allgather(&(tempField->t[chunk_idx][0]), 2 * chunk_height * y, MPI_FLOAT, &(field->t[0][0]), 2 * chunk_height * y, MPI_FLOAT, MPI_COMM_WORLD);
		// maintainFireplace(field);
		maintainField(field, x);
		if (rank == ROOT && X11_ENABLE)
		{
			if(iteration % X_REFRESH_RATE == 0)
				XRedraw(field, iteration);
		}
	}

	if (rank == ROOT)
	{
		/* set end timestamp */
		clock_gettime(CLOCK_MONOTONIC, &end);

		total_time = end.tv_sec - start.tv_sec + (end.tv_nsec - start.tv_nsec) / 1000000000.0;
		printf("The total running time is %fs.\n", total_time);

		/* save execution time log */
		FILE *log_file_p;
		char file_name[100];
		sprintf(file_name, "MPI_%d_processes_in_%dx%d_img_to_%d_iterations.txt", NUM_PROCESSES, y, x, MAX_ITERATIONS);
		log_file_p = fopen(file_name, "a");
		fprintf(log_file_p, "%f\n", total_time);
		fclose(log_file_p);
	}

	/* Program Finished */
	MPI_Finalize();
	return EXIT_SUCCESS;
}
