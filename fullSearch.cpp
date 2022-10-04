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
    int i, j, k, l, m, n, aux = 0;
    int width = 640;
    int height = 360;
    // alocar os vetore Rv e Ra
    Rv = (unsigned char **)malloc(sizeof *Rv * 8 * 8);
    Ra = (unsigned char **)malloc(sizeof *Ra * 8 * 8);
    // pra cada bloco do frame atual (frame2)
    // comparar onde ele está no frame referencia (frame1)
    for (i = 0; i < height; i = i+9) {
        for (j = 0; j < width; j = j+9) {
            for (k = 0; k < height;k = k+9) {
                for (l = 0; l < width; l = l+9) {
                    for (m = 0; m < 8; m++) {
                        for (n = 0; n < 8; n++) {
                            if(frame1[i+m][j+n] == frame2[k+m][l+n]) {
                                aux = aux + 1;
                            }
                        }
                    }
                    if(aux==7) printf("i: %d-%d, j: %d-%d\nk:%d-%d, l:%d-%d\nTrue\n", i,(i+8), j,(j+8), k,(k+8), l,(l+8));
                    aux = 0;

                }
            }
        }
    }
    // se encontrar guardar nos vetores Rv e Ra
    // fazer isso pra todos os frames seguintes até o fim
}