#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "image.h"

// точка входа программы
int main(int argc, char* argv[]) {
    // если запустили без аргументов — показываем справку
    if (argc == 1) {
        printf("usage: %s in.bmp out.bmp -gs -neg -crop 800 600 ...\n", argv[0]);
        return 0;
    }

    // если аргументов меньше трёх — ошибка
    if (argc < 3) {
        fprintf(stderr, "error: input and output filenames are required\n");
        return 1;
    }

    // загружаем изображение из файла
    image* img = NULL;
    if (!load_bmp(argv[1], &img)) {
        fprintf(stderr, "error: failed to load '%s'\n", argv[1]);
        return 1;
    }

    // применяем фильтры по порядку, начиная с 4-го аргумента (argv[3])
    int i = 3;
    while (i < argc) {
        if (strcmp(argv[i], "-gs") == 0) {
            apply_grayscale(img);
            i++;
        }
        else if (strcmp(argv[i], "-neg") == 0) {
            apply_negative(img);
            i++;
        }
        else if (strcmp(argv[i], "-crop") == 0) {
            if (i + 2 >= argc) {
                fprintf(stderr, "error: crop requires two parameters (width height)\n");
                free_image(img);
                return 1;
            }
            int w = atoi(argv[i+1]);
            int h = atoi(argv[i+2]);
            apply_crop(img, w, h);
            i += 3;
        }
        else if (strcmp(argv[i], "-edge") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "error: edge requires a threshold (0.0–1.0)\n");
                free_image(img);
                return 1;
            }
            double t = atof(argv[i+1]);
            apply_edge(img, t);
            i += 2;
        }
        else if (strcmp(argv[i], "-med") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "error: med requires window size\n");
                free_image(img);
                return 1;
            }
            int w = atoi(argv[i+1]);
            apply_median(img, w);
            i += 2;
        }
        else if (strcmp(argv[i], "-blur") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "error: blur requires sigma\n");
                free_image(img);
                return 1;
            }
            double s = atof(argv[i+1]);
            apply_blur(img, s);
            i += 2;
        }
        else if (strcmp(argv[i], "-glass") == 0) {
            int rad = (i+1 < argc && argv[i+1][0] != '-') ? atoi(argv[++i]) : 3;
            apply_glass_distortion(img, rad);
            i++;
        }
        else if (strcmp(argv[i], "-brush") == 0) {
            brush(img);
            i++;
        }
        else {
            fprintf(stderr, "error: unknown filter: %s\n", argv[i]);
            free_image(img);
            return 1;
        }
    }

    // сохраняем результат в выходной файл
    if (!save_bmp(argv[2], img)) {
        fprintf(stderr, "error: failed to save '%s'\n", argv[2]);
        free_image(img);
        return 1;
    }

    // освобождаем память
    free_image(img);
    printf("done! result: %s\n", argv[2]);
    return 0;
}
