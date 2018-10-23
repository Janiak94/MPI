#include <stdio.h>
#include <mpi.h>
#include <stdlib.h>
#include <string.h>

#define MASTER 0

int parse_args(
	int argc,
	char *argv[],
	size_t *start_node,
	size_t *end_node,
	char *file_name[50]
){	
	if(argc != 4){
		return 1;
	}
	char leftover[50];
	*start_node = (int) strtol(argv[1], &leftover, 10);
	*end_node = (int) strtol(argv[2], &leftover, 10);
	if(*start_node == *end_node){
		return 2;
	}
	strcpy(file_name, argv[3]);
	
	return 0;
}

void main_master(int num_mpi_proc, int argc, char *argv[]){
	size_t start_node, end_node;
	char file_name[50];
	int tag, error = parse_args(argc, argv, &start_node, &end_node, &file_name);

	/* broadcast error code and handle it */
	MPI_Bcast(&error, 1, MPI_INT, MASTER, MPI_COMM_WORLD);
	if(error != 0){
		printf("master exited with error code %d\n",error);
		MPI_Finalize();
		exit(error);
	}	
}

void main_worker(int num_mpi_proc, int mpi_rank){
	int error = 1, tag;
	MPI_Status status;

	/* check wether the arguments got accepted or not */
	MPI_Bcast(&error, 1, MPI_INT, MASTER, MPI_COMM_WORLD);
	if(error != 0){
		MPI_Finalize();
		exit(error);
	}
}

int main(int argc, char *argv[]){
	MPI_Init(&argc, &argv);
	
	int num_mpi_proc, mpi_rank;
	MPI_Comm_size(MPI_COMM_WORLD, &num_mpi_proc);
	MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
	
	if(mpi_rank == 0){
		main_master(num_mpi_proc, argc, argv);
	}else{
		for(int i = 1; i < num_mpi_proc; ++i){
			if(mpi_rank == i){
				main_worker(num_mpi_proc, mpi_rank);
			}
		}
	}

	MPI_Finalize();
	return 0;
}
