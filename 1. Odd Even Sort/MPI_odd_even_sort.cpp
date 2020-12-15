#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <iostream>
#include <time.h>
#include "mpi.h"

const int ROOT = 0;
const int SEND = 1;
const int RECV = 2;

const int PHASE_ODD = 1;
const int PHASE_EVEN = 2;

/*	Setting of Array generation	*/
const int ARRAY_LENGTH = 40;
const int ARRAY_LOWERBOUND = 0;
const int ARRAY_UPPERBOUND = 1000;


inline bool is_even(int num) {
	return num % 2 == 0;
}

inline bool is_odd(int num) {
	return num % 2 == 1;
}

void swap(int &a, int &b) {
	int temp = a;
	a = b;
	b = temp;
}

void print_array(int *arr, int array_len) {
	for (int i = 0; i < array_len - 1; i++) {
		printf(" %d,", arr[i]);
	}
	printf(" %d\n", arr[array_len - 1]);  // for fine format
}

int subarray_first_idx_in_global(int rank, int subarray_len) {
	return rank * subarray_len;
}

int subarray_last_idx_in_global(int rank, int subarray_len) {
	return (rank + 1) * subarray_len - 1;
}

/*	Generate an random array with given length and upper bound	*/
void generate_array(int Array[], int length, int lower_bound, int upper_bound) {
	srand(time(NULL));
	for (int i = 0; i < length; i++) {
		Array[i] = rand() % upper_bound + 1 + lower_bound;
	}

}

int cal_subarray_len(int length, int num_process, bool &padding) {
	int subarray_len = length / num_process;
	if (length % num_process != 0) {  // remainder exists
		subarray_len++;
		padding = true;
		if (length / num_process <= 1) {
			MPI_Finalize();
			perror("\nThe length of the Array should greater than the number of processors!\n");
			exit(EXIT_FAILURE);
		}
	}
	else
	{
		padding = false;
	}
	return subarray_len;
}

int cal_num_padding(int length, int num_process) {
	return (num_process - (length % num_process)) % num_process;
}

bool need_send(int phase, int rank, int subarray_len) {
	if (rank != ROOT) {  // rank 0 does not need to send data actively, it only send data after it receive one 
		if (is_odd(phase) && is_odd(subarray_first_idx_in_global(rank, subarray_len))) {
			return true;
		}
		else if (is_even(phase) && is_even(subarray_first_idx_in_global(rank, subarray_len))) {
			return true;
		}
	}
	return false;
}

bool need_recv(int phase, int rank, int subarray_len, int num_process) {
	if (rank != num_process - 1) {  // the last rank 
		if (is_odd(phase) && is_even(subarray_last_idx_in_global(rank, subarray_len))) {
			return true;
		}
		else if (is_even(phase) && is_odd(subarray_last_idx_in_global(rank, subarray_len))) {
			return true;
		}
	}
	return false;
}


void odd_even_sort(int phase, int rank, int *subarray, int subarray_len, int num_process) {
	int idx;
	if (is_odd(phase) && is_even(subarray_first_idx_in_global(rank, subarray_len))) {
		idx = 0;
	}
	else if (is_even(phase) && is_odd(subarray_first_idx_in_global(rank, subarray_len))) {
		idx = 0;
	}
	else {
		idx = 1;
	}
	for (; idx < subarray_len; idx += 2) {
		if (idx == subarray_len - 1 && rank == num_process - 1) {
			break;
		}
		if (subarray[idx] > subarray[idx + 1]) {
			swap(subarray[idx], subarray[idx + 1]);
		}
	}
}

void commu_and_sort(int phase, int rank, int *subarray, int subarray_len, int num_process, MPI_Status &status) {
	if (need_send(phase, rank, subarray_len)) {
		MPI_Send(&subarray[0], 1, MPI_INT, rank - 1, 0, MPI_COMM_WORLD);
		//std::cout << "\nPhase:" << phase << ", Rank" << rank << " send: " << subarray[0] << " to Rank" << rank - 1;
	}
	if (need_recv(phase, rank, subarray_len, num_process)) {
		MPI_Recv(&subarray[subarray_len], 1, MPI_INT, rank + 1, 0, MPI_COMM_WORLD, &status);
		//std::cout << "\nPhase:" << phase << ", Rank" << rank << " recev: " << subarray[subarray_len] << " from Rank" << rank + 1 << "\n";
	}
	/*	odd-even here	*/
	odd_even_sort(phase, rank, subarray, subarray_len, num_process);
	MPI_Barrier(MPI_COMM_WORLD);


	if (need_recv(phase, rank, subarray_len, num_process)) {  // send the buffer back to its LHS
		MPI_Send(&subarray[subarray_len], 1, MPI_INT, rank + 1, 0, MPI_COMM_WORLD);
		//std::cout << "\nPhase:" << phase << ", Rank" << rank << " send: " << subarray[subarray_len] << " to Rank" << rank + 1;
	}
	if (need_send(phase, rank, subarray_len)) {
		MPI_Recv(&subarray[0], 1, MPI_INT, rank - 1, 0, MPI_COMM_WORLD, &status);
		//std::cout << "\nPhase:" << phase << ", Rank" << rank << " recev: " << subarray[0] << " from Rank" << rank - 1 << "\n";
	}
	//std::cout << "After phase " << phase << ", the subarray of Rank" << rank << " becomes: ";
	//print_array(subarray, subarray_len);
}

int main(int argc, char * argv[])
{
	int rank, num_process;

	int Array_len = ARRAY_LENGTH;  // the required length for array generation
	int Array[ARRAY_LENGTH];

	int phase;  // either PHASE_ODD or PHASE_EVEN, indicates the phase of odd-even sort 
	double start, end, total_time;
	bool padding = false;  // true if need to add INT_MAX to the tail of the Array

	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &num_process);
	MPI_Status status;

	/*  prepare for the distributed work  */
	int subarray_len = cal_subarray_len(Array_len, num_process, padding);  // the length of subarray, except the buffer
	int num_padding = cal_num_padding(Array_len, num_process);

	if (rank == ROOT) {
		/*	Display copyright for once	*/
		printf("\nName: Brando Zhang");
		printf("\nID: 116******\n");

		/*	Report the padding status	*/
		printf("\nThe padding flag is %d.", (int)padding);
		printf("\nThe number of padding is %d.", num_padding);

		/*	Test random array generation	*/
		generate_array(Array, Array_len, ARRAY_LOWERBOUND, ARRAY_UPPERBOUND);
		printf("\nThe generated array is: ");
		print_array(Array, Array_len);
		printf("\nSorting... Don't terminate this program.\n");
	}

	MPI_Barrier(MPI_COMM_WORLD);

	start = MPI_Wtime();  // set start timestamp

	/*	data structure of the subarray:
	 * ----------------------------------------------------
	 * | val0 | val1 | ... | val(subarray_len-1) | buffer |
	 * ----------------------------------------------------
	 */
	int *subarray = (int *)malloc(sizeof(int) * (subarray_len + 1));
	MPI_Scatter(&Array, subarray_len, MPI_INT, subarray, subarray_len, MPI_INT, ROOT, MPI_COMM_WORLD);

	/*	Padding INT_MAX to the last subarray	*/
	if (padding && rank == num_process - 1) {
		for (int p = 0; p < num_padding + 1; p++) {
			subarray[subarray_len - p] = INT_MAX;
		}
	}

	/*  Sort the array here  */
	for (int m = 0; m < Array_len / 2; m++) {  // any m-length array can be sorted by odd-even sort within m steps
		phase = PHASE_ODD;
		commu_and_sort(phase, rank, subarray, subarray_len, num_process, status);

		phase = PHASE_EVEN;
		commu_and_sort(phase, rank, subarray, subarray_len, num_process, status);
	}

	/*	Report the result in each subarray	*/
	MPI_Barrier(MPI_COMM_WORLD);
	for (int k = 0; k < num_process; k++) {
		if (rank == k) {
			printf("Rank %d outputs:", rank);
			print_array(subarray, subarray_len);
		}
	}

	int *sorted_Array = (int *)malloc(sizeof(int) * (Array_len + num_padding));  // avoid wrting data to an existing variable
	MPI_Gather(subarray, subarray_len, MPI_INT, sorted_Array, subarray_len, MPI_INT, ROOT, MPI_COMM_WORLD);

	/*	Report	*/
	end = MPI_Wtime();  // set end timestamp
	if (rank == ROOT) {
		total_time = end - start;
		printf("\nFinal result of the whole Array:");
		print_array(sorted_Array, Array_len);
		printf("\nThe total running time is %fs.", total_time);
		printf("\nThe size of Array is %d.", Array_len);
	}

	MPI_Finalize();
	return 0;
}