/* Sequential heat distribution simulation, modified from TA's code */

#include "const.h"
#include "models.h"
#include "display.h"

#define legal(x, n) ( (x)>=0 && (x)<(n) )

TemperatureField *field;
TemperatureField *tempField, *swapField;

/* Neighbor offset: Right, Up, Left, Bsottom */
int dx[4] = { 0, -1, 0, 1 };
int dy[4] = { 1, 0, -1, 0 };

/* default preference */
int x = 100;  // number of rows
int y = 100;  // number of columns
int iteration = 1000;  // max iterations
int X11_ENABLE = 0;  // 1 for enabling display

void temperature_iterate()
{
	int i, j, d;
	for (i = 0; i < field->x; i++)
		for (j = 0; j < field->y; ++j)
		{
			int cnt = 0;
			tempField->t[i][j] = 0;
			for (d = 0; d < 4; ++d)
				if (legal(i + dx[d], field->x) && legal(j + dy[d], field->y))
				{
					tempField->t[i][j] += field->t[i + dx[d]][j + dy[d]];
					++cnt;
				}
			tempField->t[i][j] /= cnt;
			tempField->t[i][field->y - j] = tempField->t[i][j];
		}
}

int main(int argc, char **argv)
{
	/* measure time interval */
	struct timespec start, end;
	double total_time;

	if (argc < 5)
	{
		printf("Usage: %s x y iteration X11_ENABLE\n", argv[0]);
		return(EXIT_FAILURE);
	}
	sscanf(argv[1], "%d", &x);
	sscanf(argv[2], "%d", &y);
	sscanf(argv[3], "%d", &iteration);
	sscanf(argv[4], "%d", &X11_ENABLE);

	field = malloc(sizeof(TemperatureField));
	tempField = malloc(sizeof(TemperatureField));
	newField(field, x, y, 0, 0);
	newField(tempField, x, y, 0, 0);
	initField(field);

	if (X11_ENABLE)
	{
		XWindow_Init(field);
	}

	int i, iter;

	/* set start timestamp */
	clock_gettime(CLOCK_MONOTONIC, &start);

	for (iter = 0; iter < iteration; iter++)
	{
		temperature_iterate();
		// maintainFireplace(field);
		maintainField(field, x);
		if (X11_ENABLE)
		{
			if (iter % X_REFRESH_RATE == 0)
				XRedraw(tempField, iter);
		}
		field = tempField;
	}

	/* set end timestamp */
	clock_gettime(CLOCK_MONOTONIC, &end);

	total_time = end.tv_sec - start.tv_sec + (end.tv_nsec - start.tv_nsec) / 1000000000.0;
	printf("The total running time is %fs.\n", total_time);

	/* save execution time log */
	FILE *log_file_p;
	char file_name[100];
	sprintf(file_name, "Sequential_in_%dx%d_img_to_%d_iterations.txt", y, x, iteration);
	log_file_p = fopen(file_name, "a");
	fprintf(log_file_p, "%f\n", total_time);
	fclose(log_file_p);

	return EXIT_SUCCESS;
}
