#include <stdio.h>
#include <cstdlib>
#include <omp.h>

using namespace std;

void readFrame(FILE *fp, unsigned char **frame, int width, int height);
int fullSearch(unsigned char **frame1, unsigned char **frame2, unsigned int **Rv, unsigned int **Ra);

int main(int argc, char *argv[]) {
    int width = 640;
    int height = 360;
    unsigned char **frameRef;
    unsigned char **frameAtual;

    // Array of frames
    unsigned char ***frames;
    int nFrames = 5;
    int frameI;

    unsigned int **Rv;
    unsigned int **Ra;
    int maxBlocks = width * height / 64;
    int size;
    double begin, end;
    omp_set_num_threads(8);

    frameRef = (unsigned char **)malloc(sizeof *frameRef * height);
    frameAtual = (unsigned char **)malloc(sizeof *frameAtual * height);

    FILE *fp = fopen("video_converted_640x360.yuv", "rb");

    if (fp == NULL)
    {
        printf("Cannot open file");
    }

    // Read frame 1 as reference
    readFrame(fp, frameRef, width, height);

    // Read remaining frames and store in array
    frames = (unsigned char ***)malloc(sizeof ***frames * nFrames);

    for (frameI = 0; frameI < nFrames; frameI++) {
        frames[frameI] = (unsigned char **)malloc(sizeof *frames[frameI]);
        readFrame(fp, frames[frameI], width, height);
    }

    begin = omp_get_wtime();
    
    // For each frame, run fullSearch
    #pragma omp parallel for shared(frames, fp, width, height, maxBlocks) private(size) lastprivate(Ra, Rv)
        for (frameI=0; frameI < nFrames ; frameI++){
            printf("Inicio frame %d. Thread %d\n", frameI, omp_get_thread_num());

            // Rv and Ra store the results of fullSearch
            Rv = (unsigned int **)malloc(sizeof *Rv * maxBlocks);
            Ra = (unsigned int **)malloc(sizeof *Ra * maxBlocks);
            size = fullSearch(frameRef, frames[frameI], Rv, Ra);

            // Write results to file
            char* fileName = new char[20];
            snprintf(fileName, 12, "%d.bin", frameI);

            FILE * result = fopen(fileName, "wb");

            for (int i = 0; i < size; i++) {
                for (int j = 0; j < 2; j++) {
                    fwrite(&Ra[i][j], sizeof(Ra[i][j]), 1, result);
                    fwrite(&Rv[i][j], sizeof(Rv[i][j]), 1, result);
                }
                // fwrite(&Ra[i], sizeof(Ra[i]), 1, result);
                // fwrite(&Rv[i], sizeof(Rv[i]), 1, result);
            }

            fclose(result);

            printf("Fim frame %d: %d blocos processados\n", frameI, size);
        }

    end = omp_get_wtime();

    printf("Fim da região paralela\n");
    
    // Close file
    fclose(fp);

    // Concatenate results into a single file
    FILE * result = fopen("coded_video.bin", "wb");

    // Write first frame to file
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            fwrite(&frameRef[i][j], sizeof(frameRef[i][j]), 1, result);
        }
    }

    // Write remaining frames to file
    for (frameI = 0; frameI < nFrames; frameI++) {
        char* fileName = new char[20];

        snprintf(fileName, 12, "%d.bin", frameI);

        FILE * partialResult = fopen(fileName, "rb");

        // fseek(partialResult, 0, SEEK_END);
        // size = ftell(partialResult) / 8;
        // fseek(partialResult, 0, SEEK_SET);
        
        // There is an X and Y value for each block
        size = (maxBlocks - 1) * 2;

        for (int i = 0; i < size; i++) {
            unsigned int Ra, Rv;

            fread(&Ra, sizeof(Ra), 1, partialResult);
            fread(&Rv, sizeof(Rv), 1, partialResult);

            fwrite(&Ra, sizeof(Ra), 1, result);
            fwrite(&Rv, sizeof(Rv), 1, result);
        }

        fclose(partialResult);

        remove(fileName);
    }
    
    // for (int i = 0; i < maxBlocks; i++) {
    //     printf("Ra: [%d] (%d, %d)\n", i, Ra[i][0], Ra[i][1]);
    //     printf("Rv: [%d] (%d, %d)\n\n", i, Rv[i][0], Rv[i][1]);
    // }

    printf("Tempo de execução: %.2f segundos\n", end-begin);
}


void readFrame(FILE *fp, unsigned char **frame, int width, int height) {
    int i;
    unsigned char *temp;

    temp = (unsigned char *)malloc(sizeof *temp * width * height);

    // Read Y frame
    for (i = 0; i < height; i++)
    {
        frame[i] = (unsigned char *)malloc(sizeof *frame[i] * width);
        fread(frame[i], sizeof(unsigned char), width, fp);
    }

    // Skip CbCR channels
    fread(temp, sizeof(unsigned char), width * height / 2, fp);
}


int fullSearch(unsigned char **frame1, unsigned char **frame2, unsigned int **Rv, unsigned int **Ra) {
    int i, j, k, l, m, n, aux = 0;
    int posI, posJ, posK, posL;
    int width = 640;
    int height = 360;

    int position = 0;

    int totalDifference = 0;
    int minTotalDifference = 16500;

    int minK, minL;

    // percorre blocos do frame 2 (atual)
    //# pragma omp for collapse(2) nowait
    for (i = 0; i < height/8; i++) {
        for (j = 0; j < width/8; j++) {
            posI = i*8;
            posJ = j*8;
            minTotalDifference = 16500;
            minK = 0;
            minL = 0;

            //printf("%d\n", omp_get_thread_num());

            // percorre blocos do frame 1 (referencia)
            // Parallel For
            #pragma omp for collapse(2) nowait
            for (k = 0; k < height/8; k++) {
                for (l = 0; l < width/8; l++) {
                    totalDifference = 0;
                    posK = i*8;
                    posL = j*8;

                    // percorre pixels do bloco de referencia
                    // Parallel for com reduction para o totalDifference
                        //# pragma omp for collapse(2)
                        for (m = 0; m < 8; m++) {
                            for (n = 0; n < 8; n++) {
                                // compara pixels do frame atual com referencia
                                totalDifference += abs(frame2[posI+m][posJ+n] - frame1[posK+m][posL+n]);
                            }
                        }

                    // Seção critica de atualizar o minTotalDifference
                    if (totalDifference < minTotalDifference) {
                        minTotalDifference = totalDifference;
                        minK = posK;
                        minL = posL;
                    }
                }
            }

            // guarda bloco com menor diferenca nos vetores
            position = (i * width / 8) + j;
            
            Rv[position] = (unsigned int *)malloc(sizeof *Rv[position] * 2);
            Rv[position][0] = minK;
            Rv[position][1] = minL;

            Ra[position] = (unsigned int *)malloc(sizeof *Ra[position] * 2);
            Ra[position][0] = posI;
            Ra[position][1] = posJ;
            
            // printf("Ra: [%d] (%d, %d)\n", position, Ra[position][0], Ra[position][1]);
            // printf("Rv: [%d] (%d, %d)\n", position, Rv[position][0], Rv[position][1]);
            // printf("%d\n\n", omp_get_thread_num());
        }
    }
    return position;
 }