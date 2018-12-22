/*
** This code is in the Public Domain.
**
** Unless required by applicable law or agreed to in writing, this
** software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
** CONDITIONS OF ANY KIND, either express or implied.
**
*/

#include <stdio.h>
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
        ESP_LOGI(TAG, "epaper wakeup");
        epaper_wakeup();

        ESP_LOGI(TAG, "epaper clear");
        epaper_clear();

        ESP_LOGI(TAG, "epaper display");
        epaper_display(gImage_4in2);

        ESP_LOGI(TAG, "epaper sleep");
        epaper_sleep();
        
        vTaskDelay(30000 / portTICK_PERIOD_MS);
    }
}

void app_main()
{
    epaper_init();
    xTaskCreate(&hello_task, "hello_task", 2048, NULL, 5, NULL);
    xTaskCreate(&display_task, "display_task", 2048, NULL, 5, NULL);
}
