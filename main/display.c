#include "display.h"
#include "oled.h"


all_info_t info = {
    .process = {
        {"Process1", 10},
        {"Process2", 20},
        {"Process3", 30},
        {"Process4", 40},
        {"Process5", 50}
    },
    .cpu = 60,
    .gpu = 70,
    .ram = 80,
    .page = 0,
    .selected_page = 0
};


void home(int cpu,int ramq, int gpu)
{   clear_framebuffer();
    draw_rectangle(0, 0, 127, 63);
    draw_line(0,9,127,9);
    draw_string(48,2,"HOME");
    draw_string(4,13,"CPU :");
    draw_number(40,13,cpu);
    draw_string(4,24,"RAM :");
    draw_number(40,24,ramq);
    draw_string(4,35,"GPU :");
    if(gpu>=0)
        draw_number(40,35,gpu);
    else
        draw_string(40,35,"N/A");
    display_framebeffer();
}


void process(const all_info_t *info)
{
    clear_framebuffer();
    draw_rectangle(0, 0, 127, 63);
    draw_line(0,9,127,9);
    draw_string(40,2,"PROCESSES");

    for(int i=0;i<5;i++)
    {
        draw_string(4,13+i*10,info->process[i].process_name);
        draw_number(80,13+i*10,info->process[i].process_cpu);
    }
    display_framebeffer();
}


void lock_page(bool locked)
{
    clear_framebuffer();

    draw_rectangle(0, 0, 127, 63);
    draw_line(0, 9, 127, 9);

    draw_string(49, 2, "LOCK");

    if (locked)
    {
        draw_string(16, 20, "PC LOCKED");
        draw_string(10, 40, "STATUS: DONE");
    }
    else
    {
        draw_string(16, 20, "LOCK PC?");
        draw_string(10, 35, "PRESS TO LOCK");
    }

    display_framebeffer();
}


void volume_page(int volume)
{
    clear_framebuffer();

    draw_rectangle(0, 0, 127, 63);
    draw_line(0, 9, 127, 9);

    draw_string(43, 2, "VOLUME");

    draw_string(8, 17, "LEVEL:");

    if (volume < 0)
        volume = 0;
    if (volume > 100)
        volume = 100;

    draw_number(55, 17, volume);

    draw_rectangle(8, 32, 119, 42);

    int bar_width = (volume * 109) / 100;

    if (bar_width > 0)
        draw_rectangle(9, 33, 8 + bar_width, 41);

    draw_string(8, 49, "- DOWN");
    draw_string(76, 49, "+ UP");

    display_framebeffer();
}


void media_page(bool playing)
{
    clear_framebuffer();

    draw_rectangle(0, 0, 127, 63);
    draw_line(0, 9, 127, 9);

    draw_string(43, 2, "MEDIA");

    draw_string(8, 17, "PLAYER:");

    if (playing)
        draw_string(8, 29, "PLAYING");
    else
        draw_string(8, 29, "PAUSED");

    draw_rectangle(28, 42, 100, 57);

    if (playing)
    {
        // Pause symbol: two vertical bars
        draw_rectangle(49, 46, 53, 53);
        draw_rectangle(59, 46, 63, 53);
    }
    else
    {
        // Play symbol: a simple right-pointing triangle
        draw_line(52, 45, 52, 54);
        draw_line(53, 46, 53, 53);
        draw_line(54, 47, 54, 52);
        draw_line(55, 48, 55, 51);
        pixel(56, 49, true);
    }

    display_framebeffer();
}


void show_selected_page(void)
{
    switch (selected_page)
    {
        case PAGE_HOME:
            home(info.cpu, info.ram, info.gpu);
            break;

        case PAGE_PROCESSES:
            process(&info);
            break;

        case PAGE_VOLUME:
            volume_page(65);
            break;

        case PAGE_MEDIA:
            media_page(false);
            break;

        case PAGE_LOCK:
            lock_page(false);
            break;

        default:
            break;
    }
}