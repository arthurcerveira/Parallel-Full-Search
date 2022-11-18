#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv) {
  // Initialize the MPI environment
  MPI_Init(NULL, NULL);
  // Find out rank, size
  int world_rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
  int world_size;
  MPI_Comm_size(MPI_COMM_WORLD, &world_size);

  int token = 10;

  for (int i = 1; i < sizeof(token); i++) {
    if(world_rank == 0){
    MPI_Send(&token, 1, MPI_INT, world_rank + i, i,
            MPI_COMM_WORLD);
    }
    if (world_rank != 0) {
        MPI_Recv(&token, 1, MPI_INT, 0, i, MPI_COMM_WORLD,
                MPI_STATUS_IGNORE);
        printf("Process %d received token\n", world_rank);
    }
    }
  // Receive from the lower process and send to the higher process. Take care
  // of the special case when you are the first process to prevent deadlock.

  MPI_Finalize();

}