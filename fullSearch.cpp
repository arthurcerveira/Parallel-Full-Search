#include <stdio.h>
#include <cstdlib>


void readFrames(FILE *fp, unsigned char ** frame1, unsigned char ** frame2, int width, int height);


int main(int argc, char *argv[]) {
    int width = 640;
    int height = 360;
    unsigned char **frame1;
    unsigned char **frame2;

    frame1 = (unsigned char**)malloc(sizeof *frame1 * height);
    frame2 = (unsigned char**)malloc(sizeof *frame2 * height);

    FILE *fp = fopen("video_converted_640x360.yuv", "rb");
    
    if (fp == NULL) {
        printf("Cannot open file");
    }

    readFrames(fp, frame1, frame2, width, height);

    // Close file
    fclose(fp);

    // Write frame to image file
    FILE *f = fopen("frame1.ppm", "wb");
    fprintf(f, "P6\n%i %i 255\n", width, height);

    for (int x=0; x<height; x++) {
        for (int y=0; y<width; y++) {
            fputc(frame1[x][y], f);
            fputc(frame1[x][y], f);
            fputc(frame1[x][y], f); 
        }
    }

    fclose(f);

    FILE *f2 = fopen("frame2.ppm", "wb");
    
    fprintf(f2, "P6\n%i %i 255\n", width, height);

    for (int x=0; x<height; x++) {
        for (int y=0; y<width; y++) {
            fputc(frame2[x][y], f2);
            fputc(frame2[x][y], f2);
            fputc(frame2[x][y], f2); 
        }
    }

    fclose(f2);
}


void readFrames(FILE *fp, unsigned char ** frame1, unsigned char ** frame2, int width, int height) {
    int i;
    unsigned char * temp;

    temp = (unsigned char*)malloc(sizeof *temp * width * height);

    // Read Y frame 1
    for (i = 0; i < height; i++) {
        frame1[i] = (unsigned char*)malloc(sizeof *frame1[i] * width);
        fread(frame1[i], sizeof(unsigned char), width, fp);
    }

    // Skip CbCR channels
    fread(temp, sizeof(unsigned char), width * height / 2, fp);
    
    // Read Y frame 2
    for (i = 0; i < height; i++) {
        frame2[i] = (unsigned char*)malloc(sizeof *frame2[i] * width);
        fread(frame2[i], sizeof(unsigned char), width, fp);
    }
}
