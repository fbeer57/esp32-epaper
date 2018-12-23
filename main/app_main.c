/*
** This code is in the Public Domain.
**
** Unless required by applicable law or agreed to in writing, this
** software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
** CONDITIONS OF ANY KIND, either express or implied.
**
*/

#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "sdkconfig.h"
#include "esp_log.h"
#include "esp_console.h"
#include "driver/uart.h"

#include "epaper.h"

static const char* TAG="ESP32-test";

void hello_task(void *pvParameter)
{
    while(1) {
        ESP_LOGI(TAG, "Hello World");
        vTaskDelay(10000 / portTICK_PERIOD_MS);
    }
}

void display_task(void *pvParameter)
{
    while(1) {
        for(int i = 0; i < 100; ++i) {
            clear(1);
            int x0 = rand() % EPD_WIDTH;
            int y0 = rand() % EPD_HEIGHT;
            int x1 = rand() % EPD_WIDTH;
            int y1 = rand() % EPD_HEIGHT;
            int r = rand() % (EPD_WIDTH / 3);
            switch(i%6)
            {
                case 0:
                    draw_filled_rect(x0, y0, x1, y1, 0);
                    break;
                case 1:
                    draw_rect(x0, y0, x1, y1, 0);
                    break;
                case 2:
                    draw_line(x0, y0, x1, y1, 0);
                    break;
                case 3:
                    draw_circle(x0, y0, r, 0);
                    break;
                case 4:
                    draw_filled_circle(x0, y0, r, 0);
                    break;
                case 5:
                    draw_filled_circle(x0, y0, 5, 0);
                    break;
            }
            refresh();
            vTaskDelay(5000 / portTICK_PERIOD_MS);
       }
    }
}

void app_main()
{
    epaper_init();
    xTaskCreate(&hello_task, "hello_task", 2048, NULL, 5, NULL);
    xTaskCreate(&display_task, "display_task", 2048, NULL, 5, NULL);
}
