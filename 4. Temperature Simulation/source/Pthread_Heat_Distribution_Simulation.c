/* Paralleled heat distribution simulation via Pthread static assignment */

#include "const.h"
#include "models.h"
#include "display.h"
#include <pthread.h>
#include <stdint.h>

#define chunk_height (int)(x % NUM_PTHREADS == 0 ? x / NUM_PTHREADS : x / NUM_PTHREADS + 1)

TemperatureField *field, *tempField;

/* Neighbor offset: Right, Up, Left, Bsottom */
int dx[4] = { 0, -1, 0, 1 };
int dy[4] = { 1, 0, -1, 0 };

/* default preference */
int NUM_PTHREADS = 4;  // number of pthreads
int x = 100;  // number of rows
int y = 100;  // number of columns
int MAX_ITERATIONS = 1000;  // max iterations
int X11_ENABLE = 0;  // 1 for enabling display

enum status { WORKING, FREE };
enum boolean {FALSE, TRUE};
enum status current_status = FREE;
enum boolean simulating = TRUE;
int remain_pthreads = 4;

pthread_mutex_t mutex;
pthread_cond_t all_finished;  // raise if all pthreads finish their simulation
pthread_cond_t active;  // raise if need to activate all the threads

void temperature_iterate(int chunk_idx)
{
	int i, j, d;
	for (i = chunk_idx; i < chunk_idx + chunk_height; i++)
		if (legal(i, field->x))
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

void wait_child_threads()
{
	/* wait child threads until all of them finish their work */
	pthread_mutex_lock(&mutex);
	if (remain_pthreads > 0)
	{
		pthread_cond_wait(&all_finished, &mutex);
	}
	pthread_mutex_unlock(&mutex);
}

void new_simulate()
{
	/* activate all pthreads for new simulation */
	pthread_mutex_lock(&mutex);
	remain_pthreads = NUM_PTHREADS;  // reset the number of remaining threads to NUM_PTHREADS
	pthread_cond_broadcast(&active);
	pthread_mutex_unlock(&mutex);
}

static void *simulate(void *idx)
{
	/* pthread routine */
	int chunk_idx = (intptr_t)idx;
	while (simulating)
	{
		pthread_mutex_lock(&mutex);
		if (--remain_pthreads == 0)
		{
			pthread_cond_signal(&all_finished);
		}
		pthread_cond_wait(&active, &mutex);
		pthread_mutex_unlock(&mutex);
		if (current_status == WORKING) temperature_iterate(chunk_idx);
	}
	//printf("Exit!\n");
	pthread_exit(NULL);
}

int main(int argc, char **argv)
{
	/* measure time interval */
	struct timespec start, end;
	double total_time;

	int i, iteration;

	/* modify preference */
	if (argc < 6)
	{
		printf("Hint: Execute with %s NUM_PTHREADS x y MAX_ITERATIONS X11_ENABLE\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	sscanf(argv[1], "%d", &NUM_PTHREADS);
	sscanf(argv[2], "%d", &x);
	sscanf(argv[3], "%d", &y);
	sscanf(argv[4], "%d", &MAX_ITERATIONS);
	sscanf(argv[5], "%d", &X11_ENABLE);

	field = (TemperatureField*)malloc(sizeof(TemperatureField));
	tempField = (TemperatureField*)malloc(sizeof(TemperatureField));
	newField(field, x, y, 0, 0);
	newField(tempField, x, y, 0, 0);
	initField(field);

	if (X11_ENABLE)
	{
		XWindow_Init(field);
	}

	remain_pthreads = NUM_PTHREADS;

	/* create pthreads */
	pthread_mutex_init(&mutex, NULL);
	pthread_cond_init(&active, NULL);
	pthread_cond_init(&all_finished, NULL);
	pthread_t *threads = (pthread_t*)malloc(sizeof(pthread_t) * NUM_PTHREADS);
	
	int rc;

	current_status = FREE;

	/* set start timestamp */
	clock_gettime(CLOCK_MONOTONIC, &start);

	for (i = 0; i < NUM_PTHREADS; i++)
	{
		int chunk_idx = i * chunk_height;
		rc = pthread_create(&threads[i], NULL, &simulate, (void*)(intptr_t)chunk_idx);
		if (rc) {
			printf("ERROR: return code from pthread_create() is %d", rc);
			exit(EXIT_FAILURE);
		}
	}

	for (iteration = 0; iteration < MAX_ITERATIONS; iteration++)
	{
		wait_child_threads();
		pthread_mutex_lock(&mutex);
		current_status = WORKING;
		pthread_mutex_unlock(&mutex);

		new_simulate();
		wait_child_threads();

		// maintainFireplace(tempField);
		maintainField(tempField, x);
		if (X11_ENABLE)
		{
			if (iteration % X_REFRESH_RATE == 0)
				XRedraw(tempField, iteration);
		}

		field = tempField;
	}

	pthread_mutex_lock(&mutex);
	pthread_cond_broadcast(&active);
	current_status = FREE;
	simulating = FALSE;
	//printf("simulating is set to 0!\n");
	pthread_mutex_unlock(&mutex);

	/* set end timestamp */
	clock_gettime(CLOCK_MONOTONIC, &end);

	for (i = 0; i < NUM_PTHREADS; i++)
	{
		rc = pthread_join(threads[i], NULL);
		if (rc) {
			printf("Error: unable to join thread, %d\n", rc);
			exit(EXIT_FAILURE);
		}
	}
	//printf("Pthread joined!\n");

	total_time = end.tv_sec - start.tv_sec + (end.tv_nsec - start.tv_nsec) / 1000000000.0;
	printf("The total running time is %fs.\n", total_time);

	/* save execution time log */
	FILE *log_file_p;
	char file_name[100];
	sprintf(file_name, "Pthread_%d_threads_in_%dx%d_img_to_%d_iterations.txt", NUM_PTHREADS, y, x, MAX_ITERATIONS);
	log_file_p = fopen(file_name, "a");
	fprintf(log_file_p, "%f\n", total_time);
	fclose(log_file_p);

	pthread_mutex_destroy(&mutex);
	pthread_cond_destroy(&all_finished);
	pthread_cond_destroy(&active);

	pthread_exit(NULL);
	return EXIT_SUCCESS;
}