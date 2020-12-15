/* Paralleled N-Body simulation via MPI with Barnes-Hut */

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "utils.h"
#include "mpi.h"

const int ROOT = 0;
const double DELTA_TIME = 0.001;  /* minimun delta time of movement */
const double THETA = 0.5;  /* threshold for approximation */
const int MASS_MIN = 500;  /* minimun mass for random */
const int MASS_MAX = 1000; /* maximun mass for random */

/* default preference */
int X_RESN = 800;  /* x resolution */
int Y_RESN = 800;  /* y resolution */
int N = 500;  /* amount of bodies */
int MAX_STEP = 5000;  /* total simulation steps */
int X11_ENABLE = 0;  /* 1 for enabling display */


/* Display via X11 */
Window win;						/* initialization for a window */
unsigned int x = 0, y = 0,		/* window position */
border_width,					/* border width in pixels */
display_width, display_height,	/* size of screen */
screen;							/* which screen */
char * window_name = "MPI N-Body 116010308", *display_name = NULL;
GC gc;  /* graphic context */
unsigned long valuemask = 0;
XGCValues values;
Display *display;
XSizeHints size_hints;
Pixmap pm;

struct body * bodies;

void initialization(int *X_U, int *X_L, int *Y_U, int *Y_L) {
	int i;
	bodies = malloc(N * sizeof(struct body));
	for (i = 0; i < N; i++) {

		srand(time(0) + rand());
		bodies[i].m = rand() % (MASS_MAX - MASS_MIN) + MASS_MIN;

		bodies[i].vx = 0;
		bodies[i].vy = 0;

		srand(time(0) + rand());
		bodies[i].x = rand() % (*X_U - *X_L) + *X_L;

		srand(time(0) + rand());
		bodies[i].y = rand() % (*Y_U - *Y_L) + *Y_L;

		bodies[i].fx = 0;
		bodies[i].fy = 0;
	}
}

int main(int argc, char *argv[])
{
	/* measure time interval */
	struct timespec start, end;
	double total_time;
	int i, j, current_step = 0;

	int rank, NUM_PROCESSES;

	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &NUM_PROCESSES);
	MPI_Barrier(MPI_COMM_WORLD);

	/* modify preference */
	if (rank == ROOT && argc < 6)
	{
		printf("Hint: Execute with mpiexec -oversubscribe -np NUM_PROCESSES %s X_RESN Y_RESN N MAX_STEP X11_ENABLE\n", argv[0]);
		MPI_Finalize();
		exit(EXIT_FAILURE);
	}
	sscanf(argv[1], "%d", &X_RESN);
	sscanf(argv[2], "%d", &Y_RESN);
	sscanf(argv[3], "%d", &N);
	sscanf(argv[4], "%d", &MAX_STEP);
	sscanf(argv[5], "%d", &X11_ENABLE);

	int X_L = 3 * X_RESN / 8.0;     /* X lower bound for particle initial local */
	int X_U = 5 * X_RESN / 8.0;     /* X upper bound for particle initial local */
	int Y_L = 3 * Y_RESN / 8.0;     /* Y lower bound for particle initial local */
	int Y_U = 5 * Y_RESN / 8.0;     /* Y upper bound for particle initial local */

	int X_L_M = 2 * X_RESN / 8.0;   /* X lower bound for particle movement */
	int X_U_M = 6 * X_RESN / 8.0;   /* X upper bound for particle movement */
	int Y_L_M = 2 * Y_RESN / 8.0;   /* Y lower bound for particle movement */
	int Y_U_M = 6 * Y_RESN / 8.0;   /* Y upper bound for particle movement */

	initialization(&X_U, &X_L, &Y_U, &Y_L);

	if (rank == ROOT)
	{
		printf("Initializing... Don't terminate.\n");
		if (X11_ENABLE)
		{
			/* connect to Xserver */
			if ((display = XOpenDisplay(display_name)) == NULL)
			{
				fprintf(stderr, "drawon: cannot connect to X server %s\n",
					XDisplayName(display_name));
				exit(EXIT_FAILURE);
			}
			/* get screen size */
			screen = DefaultScreen(display);
			/* create opaque window */
			border_width = 4;
			win = XCreateSimpleWindow(display, RootWindow(display, screen),
				x, y, X_RESN, Y_RESN, border_width,
				WhitePixel(display, screen), BlackPixel(display, screen));
			XSelectInput(display, win, ExposureMask | KeyPressMask);
			XMapWindow(display, win);
			XStoreName(display, win, window_name);
			gc = DefaultGC(display, screen);
			pm = XCreatePixmap(display, win, X_RESN, Y_RESN, DefaultDepth(display, screen));
			XFillRectangle(display, pm, gc, x, y, X_RESN, Y_RESN);
			XSetForeground(display, gc, WhitePixel(display, screen));
		}

		/* set start timestamp */
		clock_gettime(CLOCK_MONOTONIC, &start);
	}
	else
	{
		bodies = malloc(N * sizeof(struct body));
	}

	for (i = 0; i < N; i++) {
		MPI_Bcast(&(bodies[i].m), 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
		MPI_Bcast(&(bodies[i].vx), 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
		MPI_Bcast(&(bodies[i].vy), 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
		MPI_Bcast(&(bodies[i].x), 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
		MPI_Bcast(&(bodies[i].y), 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
		MPI_Bcast(&(bodies[i].fx), 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
		MPI_Bcast(&(bodies[i].fy), 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
	}

	for (; current_step < MAX_STEP; current_step++) 
	{

		struct node * rootnode;
		double xmin = X_U_M, xmax = X_L_M, ymin = Y_U_M, ymax = Y_L_M;

		for (i = 0; i < N; i++) {
			bodies[i].fx = 0;
			bodies[i].fy = 0;
			xmin = xmin < bodies[i].x ? xmin : bodies[i].x;
			xmax = xmax > bodies[i].x ? xmax : bodies[i].x;
			ymin = ymin < bodies[i].y ? ymin : bodies[i].y;
			ymax = ymax > bodies[i].y ? ymax : bodies[i].y;
		}

		rootnode = Create_Node(bodies, xmin, xmax, ymin, ymax);

		for (i = 1; i < N; i++) {
			Insert_Body(bodies + i, rootnode);
		}

		for (i = 0; i < N; i++) {
			if (rank == i % NUM_PROCESSES) {
				Calculate_force(rootnode, bodies + i, THETA);
			}
		}

		for (i = 0; i < N; i++) {
			MPI_Bcast(&(bodies[i].fx), 1, MPI_DOUBLE, i % NUM_PROCESSES, MPI_COMM_WORLD);
			MPI_Bcast(&(bodies[i].fy), 1, MPI_DOUBLE, i % NUM_PROCESSES, MPI_COMM_WORLD);
		}

		for (i = 0; i < N; i++) {
			bodies[i].vx += DELTA_TIME * bodies[i].fx / bodies[i].m;
			bodies[i].vy += DELTA_TIME * bodies[i].fy / bodies[i].m;

			bodies[i].x += DELTA_TIME * bodies[i].vx;
			bodies[i].y += DELTA_TIME * bodies[i].vy;

			if (bodies[i].x < X_L_M || bodies[i].x > X_U_M) {
				bodies[i].vx = -bodies[i].vx;
			}

			if (bodies[i].y < Y_L_M || bodies[i].y > Y_U_M) {
				bodies[i].vy = -bodies[i].vy;
			}
		}

		if (rank == ROOT && X11_ENABLE)
		{
			char str[20];
			sprintf(str, "Step: %d", current_step);
			XSetForeground(display, gc, BlackPixel(display, screen));
			XFillRectangle(display, pm, gc, x, y, X_RESN, Y_RESN);
			XSetForeground(display, gc, WhitePixel(display, screen));
			XDrawString(display, pm, gc, 25, 25, str, strlen(str));
			for (j = 0; j < N; j++) {
				XSetForeground(display, gc, 0xFFFFFF / (MASS_MAX - MASS_MIN) * (bodies[j].m));
				XDrawPoint(display, pm, gc, (int)bodies[j].x, (int)bodies[j].y);
			}
			XCopyArea(display, pm, win, gc, x, y, X_RESN, Y_RESN, x, y);
		}

		current_step += 1;
		Destroy_Tree(rootnode);

	}

	if (rank == ROOT) {

		/* set end timestamp */
		clock_gettime(CLOCK_MONOTONIC, &end);

		total_time = end.tv_sec - start.tv_sec + (end.tv_nsec - start.tv_nsec) / 1000000000.0;
		printf("The total running time is %fs.\n", total_time);

		if (X11_ENABLE)
		{
			XFreePixmap(display, pm);
			XCloseDisplay(display);
		}

		/* save execution time log */
		FILE *log_file_p;
		char file_name[100];
		sprintf(file_name, "MPI_%d_processes_of_%d-Body_in_%dx%d_img_to_%d_steps.txt", NUM_PROCESSES, N, X_RESN, Y_RESN, MAX_STEP);
		log_file_p = fopen(file_name, "a");
		fprintf(log_file_p, "%f\n", total_time);
		fclose(log_file_p);
	}

	free(bodies);
	MPI_Finalize();
	return EXIT_SUCCESS;
}

