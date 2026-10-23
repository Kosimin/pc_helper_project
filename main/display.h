#ifndef DISPLAY_H
#define DISPLAY_H
#include <stdbool.h>
typedef enum {
    PAGE_HOME = 0,
    PAGE_PROCESSES,
    PAGE_VOLUME,
    PAGE_MEDIA,
    PAGE_LOCK,
    PAGE_COUNT
} page_t;
typedef struct{
    char process_name[20];
    int process_cpu;
} proocess_info_t;


typedef struct{
    proocess_info_t process[5];
    int cpu, gpu, ram;
    int page;
    int selected_page;

} all_info_t;
extern all_info_t info;
extern volatile page_t selected_page;
void home(int cpu, int ram, int gpu);
void process(const all_info_t *info);
void lock_page(bool locked);
void volume_page(int volume);
void media_page(bool playing);
void show_selected_page(void);
#endif