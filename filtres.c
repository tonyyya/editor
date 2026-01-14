#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "image.h"

// вспомогательная функция для освобождения ядра
void free_gaussian_kernel(float** kernel, int size) {
    for (int i = 0; i < size; i++) {
        free(kernel[i]);
    }
    free(kernel);
}

// доп функция для матриц
void convolution_3x3_to_tmp(const image* img, int kernel[3][3], pixel** tmp) {
    if (img->height < 1 || img->width < 1) return;

    for (int y = 0; y < img->height; y++) {
        for (int x = 0; x < img->width; x++) {
            float sum_r = 0, sum_g = 0, sum_b = 0;

            for (int dy = -1; dy <= 1; dy++) {
                for (int dx = -1; dx <= 1; dx++) {
                    int nx = x + dx;
                    int ny = y + dy;

                    // вот тут рассматриваем крайние случаи 
                    if (nx < 0) nx = 0;
                    if (nx >= img->width) nx = img->width - 1;
                    if (ny < 0) ny = 0; 
                    if (ny >= img->height) ny = img->height - 1;

                    pixel p = img->data[ny][nx];
                    int k = kernel[dy+1][dx+1];

                    sum_r += k * p.r;
                    sum_g += k * p.g;
                    sum_b += k * p.b;
                }
            }

            tmp[y][x].r = (sum_r < 0) ? 0 : (sum_r > 255 ? 255 : (unsigned char)sum_r);
            tmp[y][x].g = (sum_g < 0) ? 0 : (sum_g > 255 ? 255 : (unsigned char)sum_g);
            tmp[y][x].b = (sum_b < 0) ? 0 : (sum_b > 255 ? 255 : (unsigned char)sum_b);
        }
    }
}

// фильтр для оттенков серого (-gs)
void apply_grayscale(image* img) {
    // проходимся по каждому пикселю
    for(int y = 0; y < img->height; y++) {
        for(int x = 0; x < img->width; x++) {
            // рассматриваем один пиксель и меняем его на серый
            pixel* p = &img->data[y][x];
            unsigned char gray = (unsigned char)(0.299 * p->r + 0.587 * p->g + 0.114 * p->b);

            p->r = gray;
            p->g = gray;
            p->b = gray;
        }
    }
}

void brush(const image* in, image* out) { // горизонтальное размытие
    if (!in || !out) return;
    if (in->width != out->width || in->height != out->height) return;

    int w = in->width;
    int h = in->height;
    const int radius = 3; // длина мазка 7 пикселей

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            long sum_r = 0, sum_g = 0, sum_b = 0;
            int count = 0;

            // горизонтальный мазок
            for (int dx = -radius; dx <= radius; dx++) {
                pixel p = get_pixel_clamped(in, x + dx, y);
                sum_r += p.r;
                sum_g += p.g;
                sum_b += p.b;
                count++;
            }

            out->data[y][x].r = (unsigned char)(sum_r / count);
            out->data[y][x].g = (unsigned char)(sum_g / count);
            out->data[y][x].b = (unsigned char)(sum_b / count);
        }
    }
}

// фильтр негативный (-neg) (я осуждаю негатив)
void apply_negative(image* img) {
    for(int y = 0; y < img->height; y++) {
        for(int x = 0; x < img->width; x++) {
            pixel* p = &img->data[y][x];

            p->r = 255 - p->r;
            p->g = 255 - p->g;
            p->b = 255 - p->b;
        }
    }
}

// фильтр обрезки (-crop W H)
void apply_crop(image* img, int new_width, int new_height) {
    if (new_width <= 0 || new_height <= 0) {
        return;
    }

    // уточняем, чтобы новый формат изображения, не был больше изначального
    int w = (new_width < img->width) ? new_width : img->width;
    int h = (new_height < img->height) ? new_height : img->height;

    // готовим память под новый массив
    pixel** new_data = malloc(h * sizeof(pixel*));
    for (int y = 0; y < h; y++) {
        new_data[y] = malloc(w * sizeof(pixel));
        memcpy(new_data[y], img->data[y], w * sizeof(pixel));
    }
    
    for (int y = 0; y < img->height; y++) {
        free(img->data[y]);
    }
    free(img->data);

    img->data = new_data;
    img->width = w;
    img->height = h;
}

// фильтр с оттеками серого (-edge threshold)
void apply_edge(image* img, double threshold) {
    apply_grayscale(img);

    // тут делаем, чтобы если изображение было слишком маленькое
    if (img->width < 3 || img->height < 3) {
        for(int y = 0; y < img->height; y++) {
            for(int x = 0; x < img->width; x++) {
                img->data[y][x] = (pixel){0, 0, 0};
            }
        }
        return;
    }

    pixel** tmp = malloc(img->height * sizeof(pixel*));
    for (int y = 0; y < img->height; y++) {
        tmp[y] = malloc(img->width * sizeof(pixel));
    }

    int kernel[3][3] = {
        {-1, -1, -1},
        {-1, 8, -1},
        {-1, -1, -1}
    };

    convolution_3x3_to_tmp(img, kernel, tmp);

    int thresh_val = (int)(threshold * 255);
    for (int y = 0; y < img->height; y++) {
        for (int x = 0; x < img->width; x++) {
            unsigned char gray = tmp[y][x].r;
            unsigned char result = (gray > thresh_val) ? 255 : 0;
            img->data[y][x] = (pixel){result, result, result};
        } 
    }
    
    for (int y = 0; y < img->height; y++) {
        free(tmp[y]);
    }
    free(tmp);
}

// фильтрация шума (-med window)
void apply_median(image* img, int window) {
    if (window % 2 == 0) {
        window++;
    }
    if (window <= 0) {
        window = 3;
    }

    int half = window / 2;

    pixel** tmp = malloc(img->height * sizeof(pixel*));
    for (int y = 0; y < img->height; y++) {
        tmp[y] = malloc(img->width * sizeof(pixel));
    }
    
    for (int y = 0; y < img->height; y++) {
        for (int x = 0; x < img->width; x++) {
            unsigned char* reds = malloc(window * window * sizeof(unsigned char));
            unsigned char* greens = malloc(window * window * sizeof(unsigned char));
            unsigned char* blues = malloc(window * window * sizeof(unsigned char));
            int count = 0;

            for (int dy = -half; dy <= half; dy++) {
                for (int dx = -half; dx <= half; dx++) {
                    int nx = x + dx;
                    int ny = y + dy;

                    if (nx < 0) nx = 0;
                    if (nx >= img->width) nx = img->width - 1;
                    if (ny < 0) ny = 0;
                    if (ny >= img->height) ny = img->height - 1;

                    pixel p = img->data[ny][nx];
                    reds[count] = p.r;
                    greens[count] = p.g;
                    blues[count] = p.b;
                    count++;
                }
            }

            for (int i = 0; i < count - 1; i++) {
                for (int j = i + 1; j < count; j++) {
                    if (reds[i] > reds[j]) { unsigned char t = reds[i]; reds[i] = reds[j]; reds[j] = t; }
                    if (greens[i] > greens[j]) { unsigned char t = greens[i]; greens[i] = greens[j]; greens[j] = t; }
                    if (blues[i] > blues[j]) { unsigned char t = blues[i]; blues[i] = blues[j]; blues[j] = t; }
                }
            }

            int mid = count / 2;
            unsigned char median_r = reds[mid];
            unsigned char median_g = greens[mid];
            unsigned char median_b = blues[mid];

            tmp[y][x].r = median_r;
            tmp[y][x].g = median_g;
            tmp[y][x].b = median_b;

            free(reds);
            free(greens);
            free(blues);
        }
    }

    for (int y = 0; y < img->height; y++) {
        memcpy(img->data[y], tmp[y], img->width * sizeof(pixel));
        free(tmp[y]);
    }
    free(tmp);
}

// еще один фильтр сигма размытие 
void apply_blur(image* img, double sigma) {
    int size = (int)(6 * sigma + 1);
    if (size < 3) size = 3;
    if (size % 2 == 0) size++;

    float** kernel = create_gaussian_kernel(size, sigma);
    int half = size / 2;

    pixel** tmp = malloc(img->height * sizeof(pixel*));
    for (int y = 0; y < img->height; y++) {
        tmp[y] = malloc(img->width * sizeof(pixel));
    }

    for (int y = 0; y < img->height; y++) {
        for (int x = 0; x < img->width; x++) {
            float sum_r = 0, sum_g = 0, sum_b = 0;

            for (int dy = -half; dy <= half; dy++) {
                for (int dx = -half; dx <= half; dx++) {
                    int nx = x + dx;
                    int ny = y + dy;

                    if (nx < 0) nx = 0;
                    if (nx >= img->width) nx = img->width - 1;
                    if (ny < 0) ny = 0;
                    if (ny >= img->height) ny = img->height - 1;

                    pixel p = img->data[ny][nx];
                    float k = kernel[dy + half][dx + half];

                    sum_r += k * p.r;
                    sum_g += k * p.g;
                    sum_b += k * p.b;
                }
            }

            tmp[y][x].r = (unsigned char)(sum_r + 0.5f);
            tmp[y][x].g = (unsigned char)(sum_g + 0.5f);
            tmp[y][x].b = (unsigned char)(sum_b + 0.5f);
        }
    }

    for (int y = 0; y < img->height; y++) {
        memcpy(img->data[y], tmp[y], img->width * sizeof(pixel));
        free(tmp[y]);
    }
    free(tmp);
    free_gaussian_kernel(kernel, size);
}

// glass
void apply_glass_distortion(image* img, int radius) {
    if (radius <= 0) radius = 3;

    srand(12345);

    pixel** tmp = malloc(img->height * sizeof(pixel*));
    for (int y = 0; y < img->height; y++) {
        tmp[y] = malloc(img->width * sizeof(pixel));
    }

    for (int y = 0; y < img->height; y++) {
        for (int x = 0; x < img->width; x++) {
            int dx = (rand() % (2 * radius + 1)) - radius;
            int dy = (rand() % (2 * radius + 1)) - radius;

            int nx = x + dx;
            int ny = y + dy;

            if (nx < 0) nx = 0;
            if (nx >= img->width) nx = img->width - 1;
            if (ny < 0) ny = 0;
            if (ny >= img->height) ny = img->height - 1;

            tmp[y][x] = img->data[ny][nx];
        } 
    }

    for (int y = 0; y < img->height; y++) {
        memcpy(img->data[y], tmp[y], img->width * sizeof(pixel));
        free(tmp[y]);
    }
    free(tmp);
}
