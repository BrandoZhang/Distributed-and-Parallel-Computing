/* Sequential Mandelbrot program */

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <pthread.h>

const double MANDELBROT_SET_LEN_SQ = 4.0;

/* default preference */
int NUM_PTHREADS = 4;
int X_RESN = 800; /* x resolution */
int Y_RESN = 800; /* y resolution */
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

typedef struct pthread_information
{
	int pthread_id;  /* this thread's PID */
	int * num_workload;  /* number of workload */
} pthread_info;

Window win;                     /* initialization for a window */
unsigned int width, height,     /* window size */
x, y,                           /* window position */
border_width,                   /*border width in pixels */
display_width, display_height,  /* size of screen */
screen;                         /* which screen */

char *window_name = "Mandelbrot Set", *display_name = NULL;
GC gc;  /* graphic context */
unsigned long valuemask = 0;
XGCValues values;
Display *display;
XSizeHints size_hints;
XSession session;
pthread_mutex_t draw_mutex;

/* draw a particular point with foreground according to its k */
void draw_with_k(XSession * session, int row_id, int column_id, int k)
{
	//XSetForeground(session->display, *session->gc, 0xFFFFFF / MAX_CALCULATE_TIME * (MAX_CALCULATE_TIME - k));  /* Mandelbrot Set is drawn with black */
	if (k == MAX_CALCULATE_TIME)
		XDrawPoint(session->display, *session->win, *session->gc, column_id, row_id);
}

void *calculate_and_draw(void *args)
{
	int thread_id, num_workload, i, column_id, row_id, k;
	Compl z, c;
	float lengthsq, temp;
	pthread_info * args_info = (pthread_info *)args;
	thread_id = args_info->pthread_id;
	num_workload = *(args_info->num_workload);
	//printf("thread_id: %d  num_workload is %d\n", thread_id, num_workload);

	for (i = 0; i < num_workload; i++)
	{
		column_id = thread_id + i * NUM_PTHREADS;
		if (column_id >= X_RESN)
		{
			/* pthread exit */
			pthread_exit(NULL);
		}

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
			pthread_mutex_lock(&draw_mutex);
			draw_with_k(&session, row_id, column_id, k);
			pthread_mutex_unlock(&draw_mutex);
		}
	}
	pthread_exit(NULL);
}

int main(int argc, char **argv)
{
	/* measure time interval */
	//clock_t start, end;
	//double total_time;
	struct timespec start, end;
	double total_time;

	/* modify preference */
	if (argc < 9)
	{
		printf("Hint: Execute with %s NUM_THREADS X_RESN Y_RESN X_MIN X_MAX Y_MIN Y_MAX MAX_CALCULATE_TIME\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	sscanf(argv[1], "%d", &NUM_PTHREADS);
	sscanf(argv[2], "%d", &X_RESN);
	sscanf(argv[3], "%d", &Y_RESN);
	sscanf(argv[4], "%f", &X_MIN);
	sscanf(argv[5], "%f", &X_MAX);
	sscanf(argv[6], "%f", &Y_MIN);
	sscanf(argv[7], "%f", &Y_MAX);
	sscanf(argv[8], "%d", &MAX_CALCULATE_TIME);
	NUM_PTHREADS = NUM_PTHREADS < X_RESN ? NUM_PTHREADS : X_RESN;

	printf("%dx%d x: [%f, %f] y: [%f, %f]\n", X_RESN, Y_RESN, X_MIN, X_MAX, Y_MIN, Y_MAX);

	if (X_MAX <= X_MIN || Y_MAX <= Y_MIN)
	{
		printf("ValueError: Please ensure that X_MAX should be greater than X_MIN while Y_MAX is greater than Y_MIN.\n");
		exit(EXIT_FAILURE);
	}

	XSetWindowAttributes attr[1];

	/* connect to Xserver */

	if ((display = XOpenDisplay(display_name)) == NULL)
	{
		fprintf(stderr, "drawon: cannot connect to X server %s\n",
			XDisplayName(display_name));
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

	/* create pthreads */
	pthread_mutex_init(&draw_mutex, NULL);
	pthread_t *threads = malloc(sizeof(pthread_t) * NUM_PTHREADS);

	int rc, i, num_workload;
	num_workload = (X_RESN + NUM_PTHREADS - 1) / NUM_PTHREADS;

	//start = clock();  /* set start timestamp */
	clock_gettime(CLOCK_MONOTONIC, &start);
	for (i = 0; i < NUM_PTHREADS; i++)
	{
		pthread_info *args = malloc(sizeof args);
		args->pthread_id = i;
		args->num_workload = &num_workload;
		rc = pthread_create(&threads[i], NULL, calculate_and_draw, (void*)args);
		if (rc) {
			printf("ERROR: return code from pthread_create() is %d", rc);
			exit(1);
		}
	}

	for (i = 0; i < NUM_PTHREADS; i++) {
		pthread_join(threads[i], NULL);
	}
	//end = clock();  /* set end timestamp */
	clock_gettime(CLOCK_MONOTONIC, &end);
	//total_time = (double)(end - start) / CLOCKS_PER_SEC;
	total_time = end.tv_sec - start.tv_sec + (end.tv_nsec - start.tv_nsec) / 1000000000.0;
	printf("The total running time is %fs.\n", total_time);

	/* save execution time log */
	FILE *log_file_p;
	char file_name[100];
	sprintf(file_name, "Pthread_%d_threads_in_%dx%d_img_to_%d_execution_time.txt", NUM_PTHREADS, X_RESN, Y_RESN, MAX_CALCULATE_TIME);
	log_file_p = fopen(file_name, "a");
	fprintf(log_file_p, "%f\n", total_time);
	fclose(log_file_p);

	XFlush(display);
	sleep(2);

	/* Program Finished */
	pthread_mutex_destroy(&draw_mutex);
	pthread_exit(NULL);
	return EXIT_SUCCESS;
}
