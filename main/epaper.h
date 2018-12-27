/*
 * Copyright 2018 <copyright holder> <email>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/* inclusion guard */
#ifndef __EPAPER_H__
#define __EPAPER_H__

#include <stdint.h>

#include "EmbeddedFonts.h"
#include "epd.h"

#ifdef __cplusplus
extern "C" {
#endif

    extern uint8_t framebuffer[EPD_BYTES];

    extern void clear(int color);
    extern void set_pixel(int x, int y, int color);
    extern void draw_line(int x0, int y0, int x1, int y1, int color);
    extern void draw_circle(int xc, int yc, int r, int color);
    extern void draw_filled_circle(int xc, int yc, int r, int color);
    extern void draw_rect(int x0, int y0, int x1, int y1, int color);
    extern void draw_filled_rect(int x0, int y0, int x1, int y1, int color);
    extern void draw_text(const char* text, int x0, int y0, int color, const lv_font_t * font_p, int xoff, int yoff);

#ifdef __cplusplus
}
#endif

#endif /* __EPAPER_H__ */
