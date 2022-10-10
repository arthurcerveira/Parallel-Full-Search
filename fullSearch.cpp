#include <stdio.h>
#include <cstdlib>

void readFrames(FILE *fp, unsigned char **frame1, unsigned char **frame2, int width, int height);
int fullSearch(unsigned char **frame1, unsigned char **frame2, unsigned char **Rv, unsigned char **Ra);

int main(int argc, char *argv[])
{
    int width = 640;
    int height = 360;
    unsigned char **frame1;
    unsigned char **frame2;
    unsigned char **Rv;
    unsigned char **Ra;
    int maxBlocks = width * height / 64;
    int size;

    frame1 = (unsigned char **)malloc(sizeof *frame1 * height);
    frame2 = (unsigned char **)malloc(sizeof *frame2 * height);

    FILE *fp = fopen("video_converted_640x360.yuv", "rb");

    if (fp == NULL)
    {
        printf("Cannot open file");
    }

    readFrames(fp, frame1, frame2, width, height);

    // alocar os vetore Rv e Ra
    Rv = (unsigned char **)malloc(sizeof *Rv * maxBlocks);
    Ra = (unsigned char **)malloc(sizeof *Ra * maxBlocks);

    size = fullSearch(frame1, frame2, Rv, Ra);
    // Close file
    fclose(fp);

    printf("Chegou\n\n");
    // printf("Ra: (%d, %d)\n", Ra[0][0], Ra[1][1]);
    // printf("Rv: (%d, %d)\n\n", Rv[0][0], Rv[1][1]);

    for (int i = 0; i < size; i++) {
        printf("Ra: (%d, %d)\n", Ra[i][0], Ra[i][1]);
        printf("Rv: (%d, %d)\n\n", Rv[i][0], Rv[i][1]);
    }
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

int fullSearch(unsigned char **frame1, unsigned char **frame2, unsigned char **Rv, unsigned char **Ra)
{
    int i, j, k, l, m, n, aux = 0;
    int width = 640;
    int height = 360;

    int position = 0;

    int encontrou = 0;
 
    // percorre blocos do frame 2 (atual)
    for (i = 0; i < height; i = i+8) {
        for (j = 0; j < width; j = j+8) {

            // percorre blocos do frame 1 (referencia)
            for (k = 0; k < height;k = k+8) {
                for (l = 0; l < width; l = l+8) {

                    // percorre pixels do bloco de referencia
                    for (m = 0; m < 8; m++) {
                        for (n = 0; n < 8; n++) {
                            // compara pixels do frame atual com referencia
                            if(frame2[i+m][j+n] == frame1[k+m][l+n]) {
                                aux = aux + 1;
                            }
                        }
                    }
                    if(aux==7) {
                        encontrou = 1;
                        break;
                    }
                    aux = 0;
                }
                aux = 0;
                if (encontrou) {
                    // printf("i: %d-%d, j: %d-%d\nk:%d-%d, l:%d-%d\nTrue\n", i,(i+8), j,(j+8), k,(k+8), l,(l+8));
                    encontrou = 0;

                    // guarda nos vetores
                    Rv[position] = (unsigned char *)malloc(sizeof *Rv[i] * 2);
                    Rv[position][0] = k;
                    Rv[position][1] = l;

                    Ra[position] = (unsigned char *)malloc(sizeof *Ra[i] * 2);
                    Ra[position][0] = i;
                    Ra[position][1] = j;

                    // printf("Ra [%d]: (%d, %d)\n", position, Ra[position][0], Ra[position][1]);
                    // printf("Rv [%d]: (%d, %d)\n", position, Rv[position][0], Rv[position][1]);

                    position++;

                    break;
                }
            }
        }
    }
    // se encontrar guardar nos vetores Rv e Ra
    // fazer isso pra todos os frames seguintes até o fim

    return position;
 }