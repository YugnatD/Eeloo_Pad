/*
Fichier : image.c
Auteur : Tanguy Dietrich
Description :
*/
#include "image.h"
#include <stdio.h>
#include <stdlib.h>


void createImageRGB(imageRGB *image, uint32_t width, uint32_t height)
{
    image->width = width;
    image->height = height;
    image->data = malloc(height * sizeof(int **));
    for (uint32_t i = 0; i < height; i++)
    {
        image->data[i] = malloc(width * sizeof(int *));
        for (uint32_t j = 0; j < width; j++)
        {
            image->data[i][j] = malloc(3 * sizeof(int));
        }
    }
}

// open Raw PPM file (P6)
void openPPM(imageRGB *image, char *filename)
{
    FILE *file = fopen(filename, "r");
    if (file == NULL)
    {
        printf("Error opening file %s \n", filename);
        exit(1);
    }
    char *line = NULL;
    size_t len = 0;
    int32_t read;
    read = getline(&line, &len, file);
    if (read == -1)
    {
        printf("Error reading file %s ERROR=1\n", filename);
        exit(1);
    }
    if (line[0] != 'P' || line[1] != '6')
    {
        printf("Error reading file %s ERROR=2\n", filename);
        exit(1);
    }
    read = getline(&line, &len, file);
    if (read == -1)
    {
        printf("Error reading file %s ERROR=3\n", filename);
        exit(1);
    }
    int width, height;
    sscanf(line, "%d %d", &width, &height);
    createImageRGB(image, width, height);
    read = getline(&line, &len, file);
    if (read == -1)
    {
        printf("Error reading file %s ERROR=4\n", filename);
        exit(1);
    }
    int max;
    sscanf(line, "%d", &max);
    if (max != 255)
    {
        printf("Error reading file %s ERROR=5\n", filename);
        exit(1);
    }
    for (int i = 0; i < image->height; i++)
    {
        for (int j = 0; j < image->width; j++)
        {
            for (int k = 0; k < 3; k++)
            {
                image->data[i][j][k] = fgetc(file);
            }
        }
    }
    fclose(file);
}

// save Raw PPM file (P6)
void savePPM(imageRGB *image, char *filename)
{
    FILE *file = fopen(filename, "w");
    if (file == NULL)
    {
        printf("Error opening file %s \n", filename);
        exit(1);
    }
    fprintf(file, "P6\n");
    fprintf(file, "%d %d\n", image->width, image->height);
    fprintf(file, "255\n");
    for (int i = 0; i < image->height; i++)
    {
        for (int j = 0; j < image->width; j++)
        {
            for (int k = 0; k < 3; k++)
            {
                fputc(image->data[i][j][k], file);
            }
        }
    }
    fclose(file);
}


void freeImageRGB(imageRGB *image)
{
    for (int i = 0; i < image->height; i++)
    {
        for (int j = 0; j < image->width; j++)
        {
            free(image->data[i][j]);
        }
        free(image->data[i]);
    }
    free(image->data);
}