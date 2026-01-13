#include "image.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static void print_error(const char* msg) {
    fprintf(stderr, "error: %s\n", msg);
} //выводим ошибки

image* create_image(int width, int height) { //создаем черную картинку
    if (width <= 0 || height <= 0) {
        print_error("image width or height must be positive");
        return NULL;
    }
    image* img = malloc(sizeof(image));
    if (!img) {
        print_error("not enough memory to create image");
        return NULL;
    }

    img->width = width;
    img->height = height;
    img->data = malloc(height * sizeof(pixel*));
    if (!img->data) { //если не хватает памяти для массива пикселей
        free(img);
        print_error("not enough memory for image rows");
        return NULL;
    }
        for (int y = 0; y < height; y++) {
        img->data[y] = calloc(width, sizeof(pixel)); // выделяем память и сразу обнуляем
        if (!img->data[y]) {
            for (int i = 0; i < y; i++) free(img->data[i]);
            free(img->data);
            free(img);
            print_error("not enough memory for one row of pixels");
            return NULL;
        }
    }
    return img;
}

int load_bmp(const char* filename, image** img) {
    FILE* f = fopen(filename, "rb");
    if (!f) return 0; // если не открылся 

    //пропускаем первые 18 байт (там служебная инфа)
    fseek(f, 18, SEEK_SET);

    int width, height; //читаем ширину и высоту
    fread(&width, 4, 1, f);
    fread(&height, 4, 1, f);

    fseek(f, 28, SEEK_SET);//пропускаем ещё 4 байта

    short bit_count; //читаем, сколько бит на пиксель, надо 24
    fread(&bit_count, 2, 1, f);

    if (bit_count != 24) {
        fclose(f);
        return 0;
    }
    int h = height < 0 ? -height : height; //высота может быть отрицательной

    // Создаём чёрную картинку нужного размера
    image* new_img = create_image(width, h);
    if (!new_img) {
        fclose(f);
        return 0;
    }

    int row_bytes = (width * 3 + 3) / 4 * 4;  //проверяем, сколько байт в строке
    unsigned char* buffer = malloc(row_bytes);

    if (height > 0) {  //если строки снизу вверх, читаем с конца
        for (int y = h - 1; y >= 0; y--) {
            fread(buffer, 1, row_bytes, f);
            for (int x = 0; x < width; x++) {
                new_img->data[y][x].b = buffer[x * 3 + 0];
                new_img->data[y][x].g = buffer[x * 3 + 1];
                new_img->data[y][x].r = buffer[x * 3 + 2];
            }
        }
    } else { //читаем как есть
        for (int y = 0; y < h; y++) {
            fread(buffer, 1, row_bytes, f);
            for (int x = 0; x < width; x++) {
                new_img->data[y][x].b = buffer[x * 3 + 0];
                new_img->data[y][x].g = buffer[x * 3 + 1];
                new_img->data[y][x].r = buffer[x * 3 + 2];
            }
        }
    }

    free(buffer);
    fclose(f);
    *img = new_img; // передаём картинку наружу
    return 1;
}

int save_bmp(const char* filename, const image* img) {
    if (!img) return 0;

    FILE* f = fopen(filename, "wb");
    if (!f) return 0;

    int w = img->width;
    int h = img->height;

    //cколько байт в одной строке, bmp требует кратно 4
    int row_bytes = (w * 3 + 3) / 4 * 4;
    int file_size = 54 + row_bytes * h;

    //заголовок файла (14 байn)
    fwrite("BM", 1, 2, f);          
    fputc(file_size, f);                 
    fputc(file_size >> 8, f);
    fputc(file_size >> 16, f);
    fputc(file_size >> 24, f);
    fwrite("\0\0\0\0", 1, 4, f);         
    fputc(54, f); fputc(0, f);        
    fputc(0, f); fputc(0, f);

    // заголовок изображения (40 байт) 
    fputc(40, f); fputc(0, f); fputc(0, f); fputc(0, f); // размер заголовка
    fputc(w, f); fputc(w >> 8, f); fputc(w >> 16, f); fputc(w >> 24, f); // ширина
    fputc(-h, f); fputc((-h) >> 8, f); fputc((-h) >> 16, f); fputc((-h) >> 24, f); // высота (отрицательная!)
    fputc(1, f); fputc(0, f);            // planes
    fputc(24, f); fputc(0, f);           // 24 бита на пиксель
    fwrite("\0\0\0\0", 1, 4, f);         // сжатие = 0
    fwrite("\0\0\0\0", 1, 4, f);         // размер изображения
    fwrite("\0\0\0\0", 1, 4, f);         // X разрешение
    fwrite("\0\0\0\0", 1, 4, f);         // Y разрешение
    fwrite("\0\0\0\0", 1, 4, f);         // цветов нет
    fwrite("\0\0\0\0", 1, 4, f);         // важных цветов нет

    unsigned char* row = malloc(row_bytes);
    for (int y = 0; y < h; y++) {
        //заполняем строку: bgr для каждого пикселя
        for (int x = 0; x < w; x++) {
            row[x * 3 + 0] = img->data[y][x].b;
            row[x * 3 + 1] = img->data[y][x].g;
            row[x * 3 + 2] = img->data[y][x].r;
        }
        //lополняем нулями до кратности 4
        for (int i = w * 3; i < row_bytes; i++) {
            row[i] = 0;
        }
        fwrite(row, 1, row_bytes, f);
    }

    free(row);
    fclose(f);
    return 1;
}

void free_image(image* img) {
    if (!img) return; 

    for (int y = 0; y < img->height; y++) {
        free(img->data[y]);
    }

    free(img->data);
    free(img);
}

pixel get_pixel_clamped(const image* img, int x, int y) {
    if (!img || !img->data) { //если изображение не существует, то возвращаем чёрный пиксель
        pixel black = {0, 0, 0};
        return black;
    }
    if (x < 0) x = 0;
    if (x >= img->width) x = img->width - 1;

    if (y < 0) y = 0;
    if (y >= img->height) y = img->height - 1;
    
    return img->data[y][x];
}