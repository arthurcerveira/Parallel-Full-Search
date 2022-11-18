#include <stdio.h>
#include <cstdlib>
#include <fstream>
#include <sys/stat.h>
#include <omp.h>
#include <mpi.h>


using namespace std;

typedef struct positionArray{
  int x;
  int y;
} positionArray;

void readFrame(MPI_File fp, int frameI, unsigned char **frame, int width, int height);
int fullSearch(unsigned char **frame1, unsigned char **frame2, positionArray *Rv, positionArray *Ra);
void defineStruct(MPI_Datatype *tstype);

int main(int argc, char *argv[]) {
    MPI_Init(&argc, &argv);

    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    int world_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    int width = 640;
    int height = 360;
    unsigned char **frameRef;

    // Array de frames
    unsigned char ***frames;
    int totalFrames = 13;
    int frameI = 0;
    int nFrames;
    int framePos;

    positionArray **RvArray;
    positionArray **RaArray;

    int maxBlocks = width * height / 64;
    int size = 0;
    double begin = 0, end = 0;

    if ((totalFrames - 1) % world_size != 0) {
        if (world_rank == 0) {
            printf("Error: The number of frames (%d) must be divisible by the number of nodes\n", totalFrames - 1);  
        }

        MPI_Finalize();
        return 0;
    }

    omp_set_num_threads(4);

    frameRef = (unsigned char **)malloc(sizeof(*frameRef) * height);

    MPI_File fp;
    MPI_File_open(MPI_COMM_WORLD, "video_converted_640x360.yuv", MPI_MODE_RDONLY, MPI_INFO_NULL, &fp);

    if (fp == NULL)
    {
        printf("Cannot open MPI_File");
    }

    // Lê frame 1 como referencia
    readFrame(fp, 0, frameRef, width, height);

    nFrames = (totalFrames - 1) / world_size;

    // Lê quadros restante e guarda em array
    frames = (unsigned char ***)malloc(sizeof(**frames) * nFrames);

    int startFrame = (world_rank * nFrames) + 1;
    int endFrame = (world_rank * nFrames + nFrames) + 1;

    for (frameI = startFrame; frameI < endFrame; frameI++)
    {
        framePos = frameI - startFrame;
        // printf("[Rank %d] frames[%d] = Frame %d\n", world_rank, framePos, frameI);
        frames[framePos] = (unsigned char **)malloc(sizeof(**frames) * height);
        readFrame(fp, frameI, frames[framePos], width, height);
    }

    // Fecha video
    MPI_File_close(&fp);

    // Aloca memoria para Rv e Ra
    RvArray = (positionArray **)malloc(sizeof(*RvArray) * nFrames);
    RaArray = (positionArray **)malloc(sizeof(*RaArray) * nFrames);

    begin = omp_get_wtime();
    
    // Para cada quadro, executa fullSearch
    #pragma omp parallel for shared(frames, fp, width, height, maxBlocks, RvArray, RaArray) private(size, framePos)
        for (frameI = startFrame; frameI < endFrame; frameI++) {
            framePos = frameI - startFrame;

            printf("Processando frame %d\t[Rank %d]\t[Thread %d]\n", 
                   frameI + 1, world_rank, omp_get_thread_num());

            // Rv e Ra guardam resultados do fullSearch
            RvArray[framePos] = (positionArray *)malloc(sizeof(positionArray) * maxBlocks);
            RaArray[framePos] = (positionArray *)malloc(sizeof(positionArray) * maxBlocks);

            // Executa fullSearch
            size = fullSearch(frameRef, frames[framePos], RvArray[framePos], RaArray[framePos]);
        }

    end = omp_get_wtime();

    printf("Tempo de execução: %f\t[Rank %d]\n", end - begin, world_rank);

    MPI_Datatype tstype;
    defineStruct(&tstype);

    positionArray *numbers;

    if (world_rank == 0) {
        positionArray **RvArrayFinal;
        positionArray **RaArrayFinal;

        RvArrayFinal = (positionArray **)malloc(sizeof(*RvArrayFinal) * totalFrames);
        RaArrayFinal = (positionArray **)malloc(sizeof(*RaArrayFinal) * totalFrames);

        // Copia resultados do rank 0
        for (frameI = 0; frameI < nFrames; frameI++) {
            RvArrayFinal[frameI] = RvArray[frameI];
            RaArrayFinal[frameI] = RaArray[frameI];
        }

        // Recebe resultados dos outros ranks
        for (int rank = 1; rank < world_size; rank++) {
            for (frameI = 0; frameI < nFrames; frameI++) {  
                // printf("Recebendo resultados do rank %d\t[Rank %d]\n", rank, world_rank);            
                RvArrayFinal[rank * nFrames + frameI] = (positionArray *)malloc(sizeof(positionArray) * maxBlocks);
                RaArrayFinal[rank * nFrames + frameI] = (positionArray *)malloc(sizeof(positionArray) * maxBlocks);

                MPI_Recv(RvArrayFinal[rank * nFrames + frameI], maxBlocks, tstype, rank, frameI, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Recv(RaArrayFinal[rank * nFrames + frameI], maxBlocks, tstype, rank, frameI, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            }        
        }
    } else {
        // Envia resultados para rank 0
        for (frameI = 0; frameI < nFrames; frameI++) {
            // printf("Enviando resultados para rank 0\t[Rank %d]\n", world_rank);
            MPI_Send(RvArray[frameI], maxBlocks, tstype, 0, frameI, MPI_COMM_WORLD);
            MPI_Send(RaArray[frameI], maxBlocks, tstype, 0, frameI, MPI_COMM_WORLD);
        }
    }

    MPI_Type_free(&tstype);
    MPI_Finalize();
}


void defineStruct(MPI_Datatype *tstype) {
    const int count = 2;
    int          blocklens[count] = {1,1};
    MPI_Datatype types[count] = {MPI_INT, MPI_INT};
    MPI_Aint     disps[count] = {offsetof(positionArray,x), 
                                 offsetof(positionArray,y)};

    MPI_Type_create_struct(count, blocklens, disps, types, tstype);
    MPI_Type_commit(tstype);
}


void readFrame(MPI_File fp, int frameI, unsigned char **frame, int width, int height) {
    int i;
    unsigned char *temp;

    int offset = frameI * width * height * 3 / 2;

    temp = (unsigned char *)malloc(sizeof *temp * width * height);

    // Lê Y de acordo com o offset
    MPI_File_read_at(fp, offset, temp, width * height, 
                     MPI::UNSIGNED_CHAR, MPI_STATUS_IGNORE);

    // Formata bytes em matriz
    for (i = 0; i < height; i++) {
        frame[i] = (unsigned char *)malloc(sizeof *frame[i] * width);

        for (int j = 0; j < width; j++)
            frame[i][j] = temp[i*width + j];
    }
}


int fullSearch(unsigned char **frame1, unsigned char **frame2, positionArray *Rv, positionArray *Ra) {
    int i, j, k, l, m, n;
    int posI=0, posJ=0, posK=0, posL=0;
    int width = 640;
    int height = 360;

    int position = 0;

    int totalDifference = 0;
    int minTotalDifference = 16500;

    int minK=0, minL=0;

    // Percorre blocos do frame atual
    for (i = 0; i < height/8; i++) {
        for (j = 0; j < width/8; j++) {            
            posI = i*8;
            posJ = j*8;

            minTotalDifference = 16500;
            minK = 0;
            minL = 0;

            // Percorre blocos do frame 1 (referencia)
            #pragma omp for collapse(2) nowait schedule(guided)
            for (k = 0; k < height/8; k++) {
                for (l = 0; l < width/8; l++) {
                    totalDifference = 0;
                    posK = k*8;
                    posL = l*8;

                    // percorre pixels do bloco de referencia
                    for (m = 0; m < 8; m++) {
                        for (n = 0; n < 8; n++) {
                            // compara pixels do frame atual com referencia
                            totalDifference += abs(
                                frame2[posI+m][posJ+n] - frame1[posK+m][posL+n]
                            );
                        }
                    }

                    if (totalDifference < minTotalDifference) {
                        minTotalDifference = totalDifference;
                        minK = posK;
                        minL = posL;
                    }
                }
            }

            // Guarda bloco com menor diferenca nos vetores
            position = (i * width / 8) + j;

            // Rv[position] = (unsigned int *)malloc(sizeof *Rv[position] * 2);
            Rv[position].x = minK;
            Rv[position].y = minL;

            // Ra[position] = (unsigned int *)malloc(sizeof *Ra[position] * 2);
            Ra[position].x = posI;
            Ra[position].y = posJ;
        }
    }

    return position;
 }