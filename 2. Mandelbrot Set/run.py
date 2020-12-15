import subprocess
import time
import os

program = input("Which program do you want to run?\n\tMPI\t\tPthread\t\tBoth\n> ").lower().strip()

X_RESN = input("What is the width resolution? (default: 800)\n> ").strip() or "800"
Y_RESN = input("What is the height resolution? (default: 800)\n> ").strip() or "800"

X_MIN = input("What is the left bound on real axis? (default: -2)\n> ").strip() or "-2"
X_MAX = input("What is the right bound on real axis? (default: 2)\n> ").strip() or "2"
Y_MIN = input("What is the lower bound on real axis? (default: -2)\n> ").strip() or "-2"
Y_MAX = input("What is the upper bound on real axis? (default: 2)\n> ").strip() or "2"

MAX_CALCULATE_TIME = input("What is the maximum computing time? (default: 100)\n> " ).strip() or "100"

times = int(input("How many times do you want to run? (default: 1)\n> ").strip() or "1")

num_workers = input("How many processes/threads do you want to run?\n> ").strip()

MPI_run_command = "mpiexec -oversubscribe -np " + num_workers + " MS_MPI " + " ".join((X_RESN, Y_RESN, X_MIN, X_MAX, Y_MIN, Y_MAX, MAX_CALCULATE_TIME))
Pthread_run_command = os.path.join(".", "MS_Pthread ") + " ".join((num_workers, X_RESN, Y_RESN, X_MIN, X_MAX, Y_MIN, Y_MAX, MAX_CALCULATE_TIME)) 
    
if(program == "mpi"):
    MPI_file_name = input("The MPI program that you want to execute (with extension):\n> ") or "MPI_mandelbrot_set.c"
    MPI_compile_command = "mpicc -o MS_MPI " + MPI_file_name + " -lX11"
    subprocess.call(MPI_compile_command, shell=True)
    for i in range(times):
        print("Experiment " + str(i + 1))
        print(MPI_run_command)
        subprocess.call(MPI_run_command, shell=True)
        time.sleep(1)
elif (program == "pthread"):
    Pthread_file_name = input("The Pthread program that you want to execute (with extension):\n> ") or "Pthread_mandelbrot_set.c"
    Pthread_compile_command = "gcc -o MS_Pthread " + Pthread_file_name + " -lpthread -lX11"
    subprocess.call(Pthread_compile_command, shell=True)
    for i in range(times):
        print("Experiment " + str(i + 1))
        print(Pthread_run_command)
        subprocess.call(Pthread_run_command, shell=True)
        time.sleep(1)
elif (program == "both"):
    MPI_file_name = input("The MPI program that you want to execute (with extension):\n> ") or "MPI_mandelbrot_set.c"
    Pthread_file_name = input("The Pthread program that you want to execute (with extension):\n> ") or "Pthread_mandelbrot_set.c"
    MPI_compile_command = "mpicc -o MS_MPI " + MPI_file_name + " -lX11"
    Pthread_compile_command = "gcc -o MS_Pthread " + Pthread_file_name + " -lpthread -lX11"
    subprocess.call(MPI_compile_command, shell=True)
    subprocess.call(Pthread_compile_command, shell=True)
    for i in range(times):
        print("Experiment " + str(i + 1))
        print(MPI_run_command)
        subprocess.call(MPI_run_command, shell=True)
        time.sleep(0.1)
        print(Pthread_run_command)
        subprocess.call(Pthread_run_command, shell=True)
        time.sleep(0.1)
else:
    print("ValueError: Cannot resolve your input, please check and try it again!")
