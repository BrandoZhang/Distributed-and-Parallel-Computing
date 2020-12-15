/* Sequential Mandelbrot program */

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "mpi.h"

double MANDELBROT_SET_LEN_SQ = 4.0;

const int ROOT = 0;
const int TAG_COL_ID = 1;
const int TAG_RESULT = 2;
const int TAG_SRC = 3;
const int TAG_TERMINATE = 4;

int X_RESN = 800;  /* x resolution */
int Y_RESN = 800;  /* y resolution */
float X_MIN = -2.0, X_MAX = 2.0;  /* range of real axis */
float Y_MIN = -2.0, Y_MAX = 2.0;  /* range of image axis */
int MAX_CALCULATE_TIME = 100;

typedef struct complextype
{
	float real, imag;
} Compl;

typedef struct X_session_type
{
	Display * display;
	Window * win;
	GC * gc;
} XSession;

/* draw a particular point with foreground according to its k */
void draw_with_k(XSession * session, int row_id, int column_id, int k)
{
	 //XSetForeground(session->display, *session->gc, 0xFFFFFF / MAX_CALCULATE_TIME * (MAX_CALCULATE_TIME - k));  /* Mandelbrot Set is drawn with black */
	if (k == MAX_CALCULATE_TIME)
		XDrawPoint(session->display, *session->win, *session->gc, column_id, row_id);
}

int main(int argc, char **argv)
{
	int rank, NUM_PROCESSES;

	/* measure time interval */
	clock_t start, end;
	double total_time;

	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &NUM_PROCESSES);

	if (NUM_PROCESSES == 1)
	{  /* at least 1 process for drawing and 1 process for computing */
		perror("ValueError: Please run this program with at least 2 processes.\n");
		MPI_Finalize();
		exit(EXIT_FAILURE);
	}

	/* modify preference */
	if (rank == ROOT && argc < 8)
	{
		printf("Hint: Execute with mpiexec -oversubscribe -np NUM_PROCESSES %s X_RESN Y_RESN X_MIN X_MAX Y_MIN Y_MAX MAX_CALCULATE_TIME\n", argv[0]);
		MPI_Finalize();
		exit(EXIT_FAILURE);
	}
	sscanf(argv[1], "%d", &X_RESN);
	sscanf(argv[2], "%d", &Y_RESN);
	sscanf(argv[3], "%f", &X_MIN);
	sscanf(argv[4], "%f", &X_MAX);
	sscanf(argv[5], "%f", &Y_MIN);
	sscanf(argv[6], "%f", &Y_MAX);
	sscanf(argv[7], "%d", &MAX_CALCULATE_TIME);
	NUM_PROCESSES = NUM_PROCESSES < X_RESN ? NUM_PROCESSES : X_RESN;

	if (rank == ROOT)
	{
		printf("%dx%d x: [%f, %f] y: [%f, %f]\n", X_RESN, Y_RESN, X_MIN, X_MAX, Y_MIN, Y_MAX);
		if (X_MAX <= X_MIN || Y_MAX <= Y_MIN)
		{
			printf("ValueError: Please ensure that X_MAX should be greater than X_MIN while Y_MAX is greater than Y_MIN.\n");
			MPI_Finalize();
			exit(EXIT_FAILURE);
		}
		else if (NUM_PROCESSES - 1 > X_RESN)  // ROOT does not calculate k values
		{
			printf("ValueError: Please ensure that NUM_PROCESSES should be less than X resolution.\n");
			MPI_Finalize();
			exit(EXIT_FAILURE);
		}
	}

	/* data structure of the result_buf:
	 * -------------------------------------------------
	 * | column_id | val0 | val1 | ... | val(Y_RESN-1) |
	 * -------------------------------------------------
	 */
	int *result_buf = (int *)malloc(sizeof(int) * (Y_RESN + 1));

	if (rank == ROOT)  /* master process */
	{
		Window win;                        /* initialization for a window */
		unsigned int width, height,        /* window size */
			x, y,                          /* window position */
			border_width,                  /*border width in pixels */
			display_width, display_height, /* size of screen */
			screen;                        /* which screen */

		char *window_name = "Mandelbrot Set", *display_name = NULL;
		GC gc;  /* graphic context */
		unsigned long valuemask = 0;
		XGCValues values;
		Display *display;
		XSizeHints size_hints;
		XSession session;

		XSetWindowAttributes attr[1];

		/* connect to Xserver */

		if ((display = XOpenDisplay(display_name)) == NULL)
		{
			fprintf(stderr, "drawon: cannot connect to X server %s\n",
				XDisplayName(display_name));
			MPI_Finalize();
			exit(EXIT_FAILURE);
		}

		/* get screen size */

		screen = DefaultScreen(display);
		display_width = DisplayWidth(display, screen);
		display_height = DisplayHeight(display, screen);

		/* set window size */

		width = X_RESN;
		height = Y_RESN;

		/* set window position */

		x = 0;
		y = 0;

		/* create opaque window */

		border_width = 4;
		win = XCreateSimpleWindow(display, RootWindow(display, screen),
			x, y, width, height, border_width,
			BlackPixel(display, screen), WhitePixel(display, screen));

		size_hints.flags = USPosition | USSize;
		size_hints.x = x;
		size_hints.y = y;
		size_hints.width = width;
		size_hints.height = height;
		size_hints.min_width = 300;
		size_hints.min_height = 300;

		XSetNormalHints(display, win, &size_hints);
		XStoreName(display, win, window_name);

		/* create graphics context */

		gc = XCreateGC(display, win, valuemask, &values);

		XSetBackground(display, gc, WhitePixel(display, screen));
		XSetForeground(display, gc, BlackPixel(display, screen));
		XSetLineAttributes(display, gc, 1, LineSolid, CapRound, JoinRound);

		attr[0].backing_store = Always;
		attr[0].backing_planes = 1;
		attr[0].backing_pixel = BlackPixel(display, screen);

		XChangeWindowAttributes(display, win, CWBackingStore | CWBackingPlanes | CWBackingPixel, attr);

		XMapWindow(display, win);
		XSync(display, 0);
		session.display = display;
		session.win = &win;
		session.gc = &gc;

		int row_id, dest_process, column_id = 0, count = 0;
		MPI_Status status;

		start = clock();  /* set start timestamp*/
		for (dest_process = 1; dest_process < NUM_PROCESSES; dest_process++)
		{
			MPI_Send(&column_id, 1, MPI_INT, dest_process, TAG_COL_ID, MPI_COMM_WORLD);
			column_id++;
			count++;
		}
		do
		{
			MPI_Recv(&result_buf[0], Y_RESN + 1, MPI_INT, MPI_ANY_SOURCE, TAG_RESULT, MPI_COMM_WORLD, &status);  /* result from slave processes */
			count--;
			if (column_id < Y_RESN)
			{
				MPI_Send(&column_id, 1, MPI_INT, status.MPI_SOURCE, TAG_COL_ID, MPI_COMM_WORLD);
				column_id++;
				count++;
			}
			else
			{
				MPI_Send(&column_id, 1, MPI_INT, status.MPI_SOURCE, TAG_TERMINATE, MPI_COMM_WORLD);
			}
			for (row_id = 0; row_id < height; row_id++)
			{
				draw_with_k(&session, row_id, result_buf[0], result_buf[row_id + 1]);
			}

		} while (count > 0);
		end = clock();  /* set end timestamp */
		total_time = (double)(end - start) / CLOCKS_PER_SEC;
		printf("The total running time is %fs.\n", total_time);

		/* save execution time log */
		FILE *log_file_p;
		char file_name[100];
		sprintf(file_name, "MPI_%d_processes_in_%dx%d_img_to_%d_execution_time.txt", NUM_PROCESSES, X_RESN, Y_RESN, MAX_CALCULATE_TIME);
		log_file_p = fopen(file_name, "a");
		fprintf(log_file_p, "%f\n", total_time);
		fclose(log_file_p);

		XFlush(display);
		sleep(2);
	}
	else /* slave process */
	{
		/* Mandlebrot variables */
		int k, row_id, column_id;  /* iteration times, x, y */
		MPI_Status status;
		Compl z, c;
		float lengthsq, temp;

		MPI_Recv(&column_id, 1, MPI_INT, ROOT, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
		while (status.MPI_TAG == TAG_COL_ID)
		{
			result_buf[0] = column_id;
			c.real = (float)(X_MIN + (float)(X_MAX - X_MIN) * column_id / X_RESN);  /* scale to x: [X_MIN, X_MAX] */
			for (row_id = 0; row_id < Y_RESN; row_id++)
			{
				z.real = z.imag = 0.0;
				c.imag = (float)(Y_MAX - (float)(Y_MAX - Y_MIN) * row_id / Y_RESN);    /* scale to y: [Y_MIN, Y_MAX] */
				k = 0;

				do
				{
					/* calculate points */
					temp = z.real * z.real - z.imag * z.imag + c.real;
					z.imag = 2.0 * z.real * z.imag + c.imag;
					z.real = temp;
					lengthsq = z.real * z.real + z.imag * z.imag;
					k++;

				} while ((lengthsq < MANDELBROT_SET_LEN_SQ) && (k < MAX_CALCULATE_TIME));
				result_buf[row_id + 1] = k;
			}
			MPI_Send(&result_buf[0], Y_RESN + 1, MPI_INT, ROOT, TAG_RESULT, MPI_COMM_WORLD);  /* send the result */
			MPI_Recv(&column_id, 1, MPI_INT, ROOT, MPI_ANY_TAG, MPI_COMM_WORLD, &status);  /* update column_id */
		}
	}

	/* Program Finished */
	MPI_Finalize();
	return EXIT_SUCCESS;
}
