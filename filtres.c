#include <stdio.h>
#include <stdio.h>
#include "image.h"


//фильтр для оттенков серого (-gs)
void apply_grayscale(image* img) {
  
    //проходимся по каждому пикселю
    for(int y = 0; y < img -> height; y++) {
        for(int x = 0; x < img -> widht; x++) {
            //рассматриваем один пиксель и меняем его на серый
            pixel* p = &img -> data[y][x];
            unsigned char gray = (unsigned char)(0.299*p -> r + 0.587*p -> g + 0.114*p -> b);

            p -> r = gray;
            p -> g = gray;
            p -> b = gray;
        }
    }
}


//фильтр негативный (-neg) (я осуждаю негатив)
void apply_negative(image* img) {


    for(int y = 0; y < img -> height; y++) {
        for(int x = 0; x < img -> widht; x++) {
            pixel* p = &img -> data[y][x];

            p -> r = 255 - p -> r;
            p -> g = 255 - p -> g;
            p -> b = 255 - p -> b;
        }
    }
}


//фильтр обрезки (-crop W H)
void apply_crop(image* img, int new_widht, int new_height) {


    if (new_widht < = 0 || new_height < = 0) {
        return -1;
    }


    //уточняем, чтобы новый формат изображения, не был больше изначального
    int w = (new_widht < img -> widht) ? new_widht : img -> widht;
    int h = (new_height < img -> height) ? new_height : img -> height;


    //готовим память под новый массив
    pixel** new_data = malloc(h * size(pixel*));
    for (int y = 0; y < h; y++) {
        new_data[y] = malloc(w * size(pixel));
        memcpy(new_data[y], img -> data[y], w * size(pixel*));
    }
    
    for (int y = 0; y < img->height; y++) {
        free(img->data[y]);
    }
    free(img->data);

    img -> data = new_data;
    img -> wight = w;
    img -> height = h;

    return 0; }