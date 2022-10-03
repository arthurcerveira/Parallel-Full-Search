#include <stdio.h>
#include <cstdlib>

void readFrames(FILE *fp, unsigned char **frame1, unsigned char **frame2, int width, int height);
void fullSearch(unsigned char **frame1, unsigned char **frame2, unsigned char **Rv, unsigned char **Ra);

int main(int argc, char *argv[])
{
    int width = 640;
    int height = 360;
    unsigned char **frame1;
    unsigned char **frame2;
    unsigned char **Rv;
    unsigned char **Ra;

    frame1 = (unsigned char **)malloc(sizeof *frame1 * height);
    frame2 = (unsigned char **)malloc(sizeof *frame2 * height);

    FILE *fp = fopen("video_converted_640x360.yuv", "rb");

    if (fp == NULL)
    {
        printf("Cannot open file");
    }

    readFrames(fp, frame1, frame2, width, height);
    fullSearch(frame1, frame2, Rv, Ra);
    // Close file
    fclose(fp);

    // Write frame to image file
    FILE *f = fopen("frame1.ppm", "wb");
    fprintf(f, "P6\n%i %i 255\n", width, height);

    for (int x = 0; x < height; x++)
    {
        for (int y = 0; y < width; y++)
        {
            fputc(frame1[x][y], f);
            fputc(frame1[x][y], f);
            fputc(frame1[x][y], f);
        }
    }

    fclose(f);

    FILE *f2 = fopen("frame2.ppm", "wb");

    fprintf(f2, "P6\n%i %i 255\n", width, height);

    for (int x = 0; x < height; x++)
    {
        for (int y = 0; y < width; y++)
        {
            fputc(frame2[x][y], f2);
            fputc(frame2[x][y], f2);
            fputc(frame2[x][y], f2);
        }
    }

    fclose(f2);
}

void readFrames(FILE *fp, unsigned char **frame1, unsigned char **frame2, int width, int height)
{
    int i;
    unsigned char *temp;

    temp = (unsigned char *)malloc(sizeof *temp * width * height);

    // Read Y frame 1
    for (i = 0; i < height; i++)
    {
        frame1[i] = (unsigned char *)malloc(sizeof *frame1[i] * width);
        fread(frame1[i], sizeof(unsigned char), width, fp);
    }

    // Skip CbCR channels
    fread(temp, sizeof(unsigned char), width * height / 2, fp);

    // Read Y frame 2
    for (i = 0; i < height; i++)
    {
        frame2[i] = (unsigned char *)malloc(sizeof *frame2[i] * width);
        fread(frame2[i], sizeof(unsigned char), width, fp);
    }
}

void fullSearch(unsigned char **frame1, unsigned char **frame2, unsigned char **Rv, unsigned char **Ra)
{
    int i, j, k, l, m, n;
    int width = 640;
    int height = 360;

    int maxBlocks = width * height / 64;

    int totalDifference = 0;
    int minTotalDifference = 100000;

    int position = 0;
    int skip = 0;

    unsigned char *temp;

    // alocar os vetore Rv e Ra
    Rv = (unsigned char **)malloc(sizeof *Rv * maxBlocks);
    Ra = (unsigned char **)malloc(sizeof *Ra * maxBlocks);
    // pra cada bloco do frame atual (frame2)
    // comparar onde ele está no frame referencia (frame1)

    // para cada bloco do frame atual (2)
    for (i = 0; i < height; i = i+8) {
        for (j = 0; j < width; j = j+8) {
            // printf("Bloco atual I :%d, J: %d\n\n", i, j);
            temp = (unsigned char *)malloc(sizeof *temp * 2);
            minTotalDifference = 100000;

            // para cada bloco do frame de referencia (1)
            for (k = 0; k < height;k = k+8) {
                for (l = 0; l < width; l = l+8) {
                    // printf("Bloco ref: K: %d L: %d\n", k, l);
                    totalDifference = 0;

                    // para cada pixel do bloco do frame de referencia
                    for (m = 0; m < 8; m++) {
                        for (n = 0; n < 8; n++) {
                            //Não precisa ser exatamente igual, só o mais próximo possível
                            totalDifference += abs(frame2[i+m][j+n] - frame1[k+m][l+n]);
                            // printf("%d\n%d\n", totalDifference, minTotalDifference);
                        }
                    }

                    // Se essa diferenca for a menor diferenca
                    if (totalDifference <= minTotalDifference) {
                        // Atualiza a menor diferenca
                        minTotalDifference = totalDifference;
                        // Adiciona no temp a posicao
                        temp[0] = i;
                        temp[1] = j;

                    }

                    if (minTotalDifference == 0) {
                        skip = 1;
                        break;
                    }
                
                if (skip) {
                    printf("i: %d-%d, j: %d-%d\nk:%d-%d, l:%d-%d\nTrue\n", i,(i+8), j,(j+8), k,(k+8), l,(l+8));
                    skip = 0;
                    break;
                }

                }
            }

            if(minTotalDifference < 3) {
                printf("i: %d-%d, j: %d-%d\nk:%d-%d, l:%d-%d\nTrue\n", i,(i+8), j,(j+8), k,(k+8), l,(l+8));
                printf(":O :O :O :O :O :O :O :O :O :O :O :O :O :O :O :O :O :O :O :O :O :O :O :O :O :O :O :O :O\n");
                // se encontrar guardar nos vetores Rv e Ra
                int posicao = (i * j) / 8;

                *Rv[posicao] = *temp;
            }                
        }
    }
    // fazer isso pra todos os frames seguintes até o fim
}