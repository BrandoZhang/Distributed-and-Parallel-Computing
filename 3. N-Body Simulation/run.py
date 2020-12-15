import subprocess
import time
import os

try:
    input = raw_input
except NameError:
    pass

program = input("Which program do you want to run?\n\tSequential\t\tMPI\t\tPthread\t\tAll\n> ").lower().strip()
X11_ENABLE = input("Do you want to turn on the display? (1 for yes, 0 for no, default is 0)\n> ").strip() or "0"
X_RESN = input("What is the width resolution? (default: 800)\n> ").strip() or "800"
Y_RESN = input("What is the height resolution? (default: 800)\n> ").strip() or "800"

N = input("What is the amount of bodies? (default: 1000)\n> ").strip() or "1000"
MAX_CALCULATE_TIME = input("What is the maximum computing time? (default: 5000)\n> " ).strip() or "5000"

if (program != "Sequential"):
    num_workers = input("How many processes/threads do you want to run? (default: 1)\n> ").strip() or "1"

times = int(input("How many times do you want to run? (default: 1)\n> ").strip() or "1")

Seq_run_command = os.path.join(".", "NB_Seq ") + " ".join((X_RESN, Y_RESN, N, MAX_CALCULATE_TIME, X11_ENABLE)) 
MPI_run_command = "mpiexec -oversubscribe -np " + num_workers + " NB_MPI " + " ".join((X_RESN, Y_RESN, N, MAX_CALCULATE_TIME, X11_ENABLE))
Pthread_run_command = os.path.join(".", "NB_Pthread ") + " ".join((num_workers, X_RESN, Y_RESN, N, MAX_CALCULATE_TIME, X11_ENABLE)) 

if (program == "sequential"):
    for i in range(times):
        print("Experiment " + str(i + 1))
        print(Seq_run_command)
        subprocess.call(Seq_run_command, shell=True)
        time.sleep(0.1)
elif(program == "mpi"):
    for i in range(times):
        print("Experiment " + str(i + 1))
        print(MPI_run_command)
        subprocess.call(MPI_run_command, shell=True)
        time.sleep(0.1)
elif (program == "pthread"):
    for i in range(times):
        print("Experiment " + str(i + 1))
        print(Pthread_run_command)
        subprocess.call(Pthread_run_command, shell=True)
        time.sleep(0.1)
elif (program == "all"):
    for i in range(times):
        print("Experiment " + str(i + 1))
        print(Seq_run_command)
        subprocess.call(Seq_run_command, shell=True)
        time.sleep(0.1)
        print(MPI_run_command)
        subprocess.call(MPI_run_command, shell=True)
        time.sleep(0.1)
        print(Pthread_run_command)
        subprocess.call(Pthread_run_command, shell=True)
        time.sleep(0.1)
else:
    print("ValueError: Cannot resolve your input, please check and try it again!")
