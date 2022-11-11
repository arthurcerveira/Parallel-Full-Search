#include <stdio.h>
#include <cstdlib>
#include <fstream>
#include <mpi.h>


void readFrame(MPI_File *fp, int frameI, unsigned char **frame, int width, int height) {
    int i;
    unsigned char *temp;

    int offset = frameI * width * height * 3 / 2;

    temp = (unsigned char *)malloc(sizeof *temp * width * height);

    // Lê Y de acordo com o offset
    MPI_File_read_at(*fp, offset, temp, width * height, 
                     MPI::UNSIGNED_CHAR, MPI_STATUS_IGNORE);

    // Formata bytes em matriz
    for (i = 0; i < height; i++) {
        frame[i] = (unsigned char *)malloc(sizeof *frame[i] * width);

        for (int j = 0; j < width; j++)
            frame[i][j] = temp[i*width + j];
    }

    // Salva quadro como imagem
    char* fileName = new char[20];

    snprintf(fileName, 12, "frame%d.ppm", frameI);

    FILE *f4 = fopen(fileName, "wb");

    fprintf(f4, "P6\n%i %i 255\n", width, height);

    for (int x=0; x<height; x++) {
        for (int y=0; y<width; y++) {
            fputc(frame[x][y], f4);
            fputc(frame[x][y], f4);
            fputc(frame[x][y], f4); 
        }
    }

    fclose(f4);
}


int main(int argc, char *argv[]) {
    MPI_Init(&argc, &argv);
    
    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    int world_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    printf("Hello world from process %d of %d\n", world_rank, world_size);

    int width = 640;
    int height = 360;
    unsigned char **frameRef;

    // Array de frames
    unsigned char ***frames;
    int nFrames = 10;
    int frameI = 0;

    unsigned int **Rv;
    unsigned int **Ra;
    int maxBlocks = width * height / 64;
    int size = 0;
    double begin = 0, end = 0;

    frameRef = (unsigned char **)malloc(sizeof *frameRef * height);

    MPI_File fp;
    MPI_File_open(MPI_COMM_WORLD, 
                    "video_converted_640x360.yuv", 
                    MPI_MODE_RDONLY, 
                    MPI_INFO_NULL, 
                    &fp);

    // Lê frame 1 como referencia
    readFrame(&fp, 0, frameRef, width, height);

    // // Lê quadros restante e guarda em array
    frames = (unsigned char ***)malloc(sizeof ***frames * nFrames);

    for (frameI = 0; frameI < nFrames; frameI++) {
        printf("frameI: %d\n", frameI);
        frames[frameI] = (unsigned char **)malloc(sizeof *frames[frameI] * height);
        readFrame(&fp, frameI + 1, frames[frameI], width, height);
    }

    printf("deu\n");

    // Fecha video
    MPI_File_close(&fp);

    printf("após\n");
    exit(0);

    MPI_Finalize();
}