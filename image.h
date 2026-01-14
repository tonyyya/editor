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

// фильтры
void apply_grayscale(image* img);
void apply_negative(image* img);
void apply_crop(image* img, int new_width, int new_height);
void apply_edge(image* img, double threshold);
void apply_median(image* img, int window);
void apply_blur(image* img, double sigma);
void apply_glass_distortion(image* img, int radius);
void apply_sharpen(image* img);
void brush(image* img);


// вспомогательные
void convolution_3x3_to_tmp(const image* img, int kernel[3][3], pixel** tmp);
float** create_gaussian_kernel(int size, double sigma);
void free_gaussian_kernel(float** kernel, int size);

#endif 
