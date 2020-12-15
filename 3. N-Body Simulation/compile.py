import subprocess
import time

try:
    input = raw_input
except NameError:
    pass

print("Note: if you want to compile MPI program, you should load the module first\n")

program = input("Which program do you want to compile?\n\tSequential\t\tMPI\t\tPthread\t\tAll\n> ").lower().strip()
head_file = input("The head file that you want to link (with extension, default is 'utils.c'):\n> ") or "utils.c"
 
if (program == "sequential"):
    Seq_file_name = input("The Sequential program that you want to compile (with extension):\n> ") or "Sequential_N_Body.c"
    Sequential_compile_command = "gcc -o NB_Seq " + Seq_file_name + " " + head_file + " -lm -lX11"
    subprocess.call(Sequential_compile_command, shell=True)
    print(Sequential_compile_command)
elif (program == "mpi"):
    MPI_file_name = input("The MPI program that you want to compile (with extension):\n> ") or "MPI_N_Body.c"
    MPI_compile_command = "mpicc -o NB_MPI " + MPI_file_name + " " +  head_file + " -lm -lX11"
    subprocess.call(MPI_compile_command, shell=True)
    print(MPI_compile_command)
elif (program == "pthread"):
    Pthread_file_name = input("The Pthread program that you want to compile (with extension):\n> ") or "Pthread_N_Body.c"
    Pthread_compile_command = "gcc -o NB_Pthread " + Pthread_file_name + " " +  head_file + " -lm -lpthread -lX11"
    subprocess.call(Pthread_compile_command, shell=True)
    print(Pthread_compile_command)
elif (program == "all"):
    Seq_file_name = input("The Sequential program that you want to compile (with extension):\n> ") or "Sequential_N_Body.c"
    MPI_file_name = input("The MPI program that you want to compile (with extension):\n> ") or "MPI_N_Body.c"
    Pthread_file_name = input("The Pthread program that you want to compile (with extension):\n> ") or "Pthread_N_Body.c"
    Sequential_compile_command = "gcc -o NB_Seq " + Seq_file_name + " " + head_file + " -lm -lX11"
    MPI_compile_command = "mpicc -o NB_MPI " + MPI_file_name + " " + head_file + " -lm -lX11"
    Pthread_compile_command = "gcc -o NB_Pthread " + Pthread_file_name + " " + head_file+ " -lm -lpthread -lX11"
    subprocess.call(Sequential_compile_command, shell=True)
    print(Sequential_compile_command)
    subprocess.call(MPI_compile_command, shell=True)
    print(MPI_compile_command)
    subprocess.call(Pthread_compile_command, shell=True)
    print(Pthread_compile_command)
else:
    print("ValueError: Cannot resolve your input, please check and try it again!")
