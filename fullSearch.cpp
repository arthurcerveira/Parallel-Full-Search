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
    int nFrames = 5;
    int frameI = 0;

    positionArray *Rv;
    positionArray *Ra;
    int maxBlocks = width * height / 64;
    int size = 0;
    double begin = 0, end = 0;

    omp_set_num_threads(4);

    // if (world_rank == 0) {
    frameRef = (unsigned char **)malloc(sizeof *frameRef * height);

    MPI_File fp;
    MPI_File_open(MPI_COMM_WORLD, "video_converted_640x360.yuv", MPI_MODE_RDONLY, MPI_INFO_NULL, &fp);

    // mkdir("Ra_Rv",0777);

    if (fp == NULL)
    {
        printf("Cannot open MPI_File");
    }

    // Lê frame 1 como referencia
    readFrame(fp, 0, frameRef, width, height);

    // Lê quadros restante e guarda em array
    frames = (unsigned char ***)malloc(sizeof **frames * nFrames);

    for (frameI = 0; frameI < nFrames; frameI++) {
        frames[frameI] = (unsigned char **)malloc(sizeof *frames[frameI] * height);
        readFrame(fp, frameI+1, frames[frameI], width, height);
    }

    begin = omp_get_wtime();
    
    // Para cada quadro, executa fullSearch
    //#pragma omp parallel for shared(frames, fp, width, height, maxBlocks) private(size) lastprivate(Ra, Rv)
        for (frameI=0; frameI < 2 ; frameI++){
            printf("Processando frame %d\t[Thread %d]\t[Rank %d]\n", 
                frameI, omp_get_thread_num(), world_rank);
            // printf("Processando frame %d\n", frameI + 1);

            // Rv e Ra guardam resultados do fullSearch
            Rv = (positionArray *)malloc(sizeof(positionArray) * maxBlocks);
            Ra = (positionArray *)malloc(sizeof(positionArray) * maxBlocks);
            size = fullSearch(frameRef, frames[frameI], Rv, Ra);
            for(int i = 0; i < 3600; i++) {
                printf("[%d] Rv(%d, %d) -> Ra(%d,%d)\n", i, Rv[i].x, Rv[i].y, Ra[i].x, Ra[i].y);
            }
            

        //     // Escreve esses resultados em um arquivo binário
        //     char* file = new char[20];
        //     char* file1 = new char[20];

        //     snprintf(file, 12, "%d.bin", frameI);
        //     snprintf(file1, 18, "Ra_Rv/%d.txt", frameI + 1);

        //     FILE *result = fopen(file, "wb");
        //     FILE *result1 = fopen(file1, "w");

        //     for (int i = 0; i < size; i++) {
        //         fwrite(&Ra[i].x, sizeof(Ra[i].x), 1, result);
        //         fwrite(&Ra[i].y, sizeof(Ra[i].y), 1, result);
        //         fwrite(&Rv[i].x, sizeof(Rv[i].x), 1, result);
        //         fwrite(&Rv[i].y, sizeof(Rv[i].y), 1, result);

        //         fprintf(result1, "Ra: (%d, %d)\n", Ra[i].x, Ra[i].y);
        //         fprintf(result1, "Rv: (%d, %d)\n\n", Rv[i].x, Rv[i].y);
        //     }

        //     fclose(result);
        //     fclose(result1);
        }

    end = omp_get_wtime();
    
    // Fecha video
    MPI_File_close(&fp);

    // Concatena resultados em um único arquivo
    // FILE * finalResult = fopen("coded_video.bin", "wb");

    // // Escreve primeiro quadro no arquivo
    // for (int i = 0; i < height; i++) {
    //     for (int j = 0; j < width; j++) {
    //         fwrite(&frameRef[i][j], sizeof(frameRef[i][j]), 1, finalResult);
    //     }
    // }

    // Escreve resultados do fullSearch para outros quadros
    // for (frameI = 0; frameI < nFrames; frameI++) {
    //     char* file = new char[20];

    //     snprintf(file, 12, "%d.bin", frameI);

    //     FILE * partialResult = fopen(file, "rb");
        
    //     // Tem um valor X e Y para cada bloco
    //     size = (maxBlocks - 1) * 2;

    //     for (int i = 0; i < size; i++) {
    //         int RaI, RvI;

    //         fread(&RaI, sizeof(RaI), 1, partialResult);
    //         fread(&RvI, sizeof(RvI), 1, partialResult);

    //         fwrite(&RaI, sizeof(RaI), 1, finalResult);
    //         fwrite(&RvI, sizeof(RvI), 1, finalResult);
    //     }

    //     fclose(partialResult);

    //     remove(file);
    // }
    
    // free(Ra);
    // free(Rv);
    // free(frameRef);
    // free(frames);

    // fclose(finalResult);

    free(Ra);
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

    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    MPI_Datatype positionsStruct;
    defineStruct(&positionsStruct);
    int world_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    positionArray recvbuf[(3600/world_size)];
    positionArray temp[(3600/world_size)];

    printf("Pré-Scatter\n");
    MPI_Scatter(&Ra[0], 3600/world_size, positionsStruct, &recvbuf[0], 3600/world_size, positionsStruct, world_rank, MPI_COMM_WORLD);
    printf("Pós-Scatter\n");
    // Percorre blocos do frame atual
    for (i = 0; i < height/8; i++) {
        //printf("1. Primeiro For\n");  
        for (j = 0; j < (width/world_size)/8; j++) {
            //printf("2. Segundo For\n");            
            posI = i*8;
            posJ = j*8;
            //printf("2. Segundo For\n%d, %d\n", posI, posJ);

            minTotalDifference = 16500;
            minK = 0;
            minL = 0;

            // Percorre blocos do frame 1 (referencia)
            //#pragma omp parallel for collapse(2) schedule(guided) shared(minK, minL, posI, posJ) lastprivate(frame2, frame1, totalDifference, minTotalDifference)
            for (k = 0; k < height/8; k++) {
                for (l = 0; l < width/8; l++) { 
                    totalDifference = 0;
                    posK = k*8;
                    posL = l*8;
                    //printf("3. For paralelo\n%d, %d\n", posK, posL); 

                    // percorre pixels do bloco de referencia
                    for (m = 0; m < 8; m++) {
                        //printf("4. Primeiro For Bloco\n%d\n", m);  
                        for (n = 0; n < 8; n++) {
                            //printf("5. Segundo For Bloco\n%d\n", n);  
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
            //printf("Saída do for paralelo do omp\n");

            // Guarda bloco com menor diferenca nos vetores
            position = j;

            // Rv[position] = (unsigned int *)malloc(sizeof *Rv[position] * 2);
            Rv[position].x = minK;
            Rv[position].y = minL;

            // Ra[position] = (unsigned int *)malloc(sizeof *Ra[position] * 2);
            temp[position].x = posI;
            temp[position].y = posJ;
        }
    }
    //MPI_Gather();
    printf("Pré-Gather\n");
    //MPI_Request request;
    MPI_Gather(&temp[0], 3600/world_size, positionsStruct, &Ra[0], 3600/world_size, positionsStruct, world_rank, MPI_COMM_WORLD);
    printf("Pós-Gather\n");

    return position;
 }