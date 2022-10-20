#include <stdio.h>
#include <cstdlib>
#include <omp.h>

void readFrame(FILE *fp, unsigned char **frame, int width, int height);
int fullSearch(unsigned char **frame1, unsigned char **frame2, unsigned int **Rv, unsigned int **Ra);

int main(int argc, char *argv[])
{
    int width = 640;
    int height = 360;
    unsigned char **frameRef;
    unsigned char **frameAtual;

    unsigned int **Rv;
    unsigned int **Ra;
    int maxBlocks = width * height / 64;
    int size;
    double begin, end;
    omp_set_num_threads(5);
    
    begin = omp_get_wtime();

    frameRef = (unsigned char **)malloc(sizeof *frameRef * height);
    frameAtual = (unsigned char **)malloc(sizeof *frameAtual * height);

    FILE *fp = fopen("video_converted_640x360.yuv", "rb");

    if (fp == NULL)
    {
        printf("Cannot open file");
    }

    readFrame(fp, frameRef, width, height);

    #pragma omp parallel for shared(frameAtual, fp, width, height, maxBlocks) private(size) lastprivate(Ra, Rv)
        for (int frameI=0; frameI < 5 ; frameI++){
            printf("Inicio frame %d\n", frameI);

            readFrame(fp, frameAtual, width, height);

            // alocar os vetore Rv e Ra
            Rv = (unsigned int **)malloc(sizeof *Rv * maxBlocks);
            Ra = (unsigned int **)malloc(sizeof *Ra * maxBlocks);
            size = fullSearch(frameRef, frameAtual, Rv, Ra);

            printf("Fim frame %d: %d blocos movimentados\n", frameI, size);
        }

    end = omp_get_wtime();

    printf("Fim da região paralela\n");
    
    // Close file
    fclose(fp);

    for (int i = 0; i < 100; i++) {
        printf("Ra: [%d] (%d, %d)\n", i, Ra[i][0], Ra[i][1]);
        printf("Rv: [%d] (%d, %d)\n\n", i, Rv[i][0], Rv[i][1]);
    }

    printf("Tempo de execução: %.2f segundos\n", end-begin);
}


void readFrame(FILE *fp, unsigned char **frame, int width, int height)
{
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


int fullSearch(unsigned char **frame1, unsigned char **frame2, unsigned int **Rv, unsigned int **Ra)
{
    int i, j, k, l, m, n, aux = 0;
    int posI, posJ;
    int width = 640;
    int height = 360;

    int position = 0;

    int totalDifference = 0;
    int minTotalDifference = 16500;

    int differenceTreshold = 100;

    int minK, minL;

    int skip = 0;

    // percorre blocos do frame 2 (atual)
    // # pragma omp for collapse(2) nowait
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
            // # pragma omp for collapse(2) nowait
            for (k = 0; k < height; k = k+8) {
                for (l = 0; l < width; l = l+8) {
                    totalDifference = 0;

                    // percorre pixels do bloco de referencia
                    // Parallel for com reduction para o totalDifference
                        // # pragma omp for collapse(2)
                        for (m = 0; m < 8; m++) {
                            for (n = 0; n < 8; n++) {
                                // compara pixels do frame atual com referencia
                                totalDifference += abs(frame2[i+m][j+n] - frame1[k+m][l+n]);
                            }
                        }

                    // Seção critica de atualizar o minTotalDifference
                    if (totalDifference < minTotalDifference) {
                        minTotalDifference = totalDifference;
                        minK = k;
                        minL = l;
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