#include <stdio.h>
#include <cstdlib>

int main(int argc, char *argv[]) {
    int width = 640;
    int height = 360;
    unsigned char **frame;

    // Read YUV file
    FILE *fp = fopen("video_converted_640x360.yuv", "rb");
    if (fp == NULL) {
        printf("Cannot open file");
    }

    frame = (unsigned char**)malloc(sizeof *frame * height);

    // Iterate through rows
    for (int i = 0; i < height; i++) {
        frame[i] = (unsigned char*)malloc(sizeof *frame[i] * width);
        fread(frame[i], sizeof(unsigned char), width, fp);
    }

    // Close file
    fclose(fp);

    // Write frame to image file
    FILE *f = fopen("out.ppm", "wb");
    fprintf(f, "P6\n%i %i 255\n", width, height);

    for (int x=0; x<height; x++) {
        for (int y=0; y<width; y++) {
            fputc(frame[x][y], f);   // 0 .. 255
            fputc(frame[x][y], f);   // 0 .. 255
            fputc(frame[x][y], f);   // 0 .. 255
        }
    }

    fclose(f);
}
    