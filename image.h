#ifndef IMAGE_H
#define IMAGE_H

// пиксель, на цвет по байту
typedef struct {
    unsigned char r, g, b;
} pixel;

//изображение. data[0][0] -- левый верхний угол
typedef struct {
    int width;
    int height;
    pixel** data;
} image;

//cоздать новое изображение (все пиксели чёрные)
image* create_image(int width, int height);

//создаем черную картинку размерами как исходная, по адресу img
int load_bmp(const char* filename, image** img); // возвращаем 1 при успехе, 0 при ошибке

//сохранить как 24-битный BMP
int save_bmp(const char* filename, const image* img); // возвращаем 1 при успехе, 0 при ошибке

//освободить память
void free_image(image* img);

//безопасное получение пикселя, если вышли за край -- берем ближайший крайний
pixel get_pixel_clamped(const image* img, int x, int y);

#endif 