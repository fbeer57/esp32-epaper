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
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "epaper.h"

#define order(a, b) if (a > b) { int c = a; a = b; b = c; }
#define set_bitmask(p, mask, color) if (color) { *p |= mask; } else { *p &= ~mask; } 

uint8_t framebuffer[EPD_BYTES];

void set_pixel(int x, int y, int color)
{
    if (x < 0 || x >= EPD_WIDTH || y < 0 || y >= EPD_HEIGHT)
    {
        return;
    }

    int pos = (y * EPD_WIDTH + x) / 8;
    uint8_t bitmask = 0x80 >> (x & 0x7);
    set_bitmask((framebuffer + pos), bitmask, color);
}

void clear(int color)
{
    memset(framebuffer, color? 0xff : 0x00, sizeof(framebuffer));
}

static int clip(int a, int m)
{
    return (a < 0)? 0 : (a >= m)? m - 1 : a;
}

static void hLine(int x0, int x1, int y, int color)
{
    x0 = clip(x0, EPD_WIDTH);
    x1 = clip(x1, EPD_WIDTH);
    y =  clip(y, EPD_HEIGHT);
    order(x0, x1);
    ++x1;
    
    uint8_t* p = framebuffer + y * EPD_BYTES_PER_ROW;
    uint8_t* end = p + (x1 >> 3);
    p += (x0 >> 3);

    uint8_t startByte = 0xff >> (x0 & 0x7);
    uint8_t endByte = ~(0xff >> (x1 & 0x7));
    
    if (startByte != 0xff)
    {
        if (p == end)
        {
            startByte &= endByte;
            endByte = 0x00;
        }
        set_bitmask(p, startByte, color);
        ++p;
    }
    
    uint8_t fill = (color)? 0xff : 0x00;
    while(p < end)
    {
        *p++ = fill;
    }

    if (endByte)
    {
        set_bitmask(p, endByte, color);
    }
}

static void vLine(int x, int y0, int y1, int color)
{
    x  = clip(x, EPD_WIDTH);
    y0 = clip(y0, EPD_HEIGHT);
    y1 = clip(y1, EPD_HEIGHT);
    order(y0, y1);
    uint8_t* p =   framebuffer + y0 * EPD_BYTES_PER_ROW + (x >> 3);
    uint8_t* end = framebuffer + y1 * EPD_BYTES_PER_ROW + (x >> 3);
    uint8_t fill = 0x80 >> (x & 0x7);
    if (color)
    {
        while(p <= end)
        {
            *p |= fill; 
            p += EPD_BYTES_PER_ROW;
        }
    }
    else
    {
        fill = ~fill;
        while(p <= end)
        {
            *p &= fill; 
            p += EPD_BYTES_PER_ROW;
        }
    }
}

static void plotLineLow(int x0, int y0, int x1, int y1, int color)
{
  int dx = x1 - x0;
  int dy = y1 - y0;
  if (dy == 0)
  {
      hLine(x0, x1, y0, color);
      return;
  }
  int yi = 1;
  if (dy < 0)
  {
    yi = -1;
    dy = -dy;
  }
  int D = 2*dy - dx;
  int y = y0;

  for(int x = x0; x < x1; ++x)
  {
    set_pixel(x, y, color);
    if (D > 0) 
    {
       y = y + yi;
       D = D - 2*dx;
    }
    D = D + 2*dy;
  }
}

static void plotLineHigh(int x0, int y0, int x1, int y1, int color)
{
  int dx = x1 - x0;
  if (dx == 0)
  {
      vLine(x0, y0, y1, color);
      return;
  }
  int dy = y1 - y0;
  int xi = 1;
  if (dx < 0) 
  {
    xi = -1;
    dx = -dx;
  }
  
  int D = 2*dx - dy;
  int x = x0;

  for(int y = y0; y < y1; ++y)
  {
    set_pixel(x, y, color);
    if (D > 0)
    {
       x = x + xi;
       D = D - 2*dy;
    }
    D = D + 2*dx;
  }
}

static void dCircle(int xc, int yc, int x, int y, int color) 
{ 
    set_pixel(xc+x, yc+y, color); 
    set_pixel(xc-x, yc+y, color); 
    set_pixel(xc+x, yc-y, color); 
    set_pixel(xc-x, yc-y, color); 
    set_pixel(xc+y, yc+x, color); 
    set_pixel(xc-y, yc+x, color); 
    set_pixel(xc+y, yc-x, color); 
    set_pixel(xc-y, yc-x, color); 
}

static void dFilledCircle(int xc, int yc, int x, int y, int color) 
{ 
    hLine(xc-x, xc+x, yc+y, color); 
    hLine(xc-x, xc+x, yc-y, color); 
    hLine(xc-y, xc+y, yc+x, color); 
    hLine(xc-y, xc+y, yc-x, color); 
}

void draw_line(int x0, int y0, int x1, int y1, int color)
{
    if (abs(y1 - y0) < abs(x1 - x0))
    {
        if (x0 > x1)
        {
            plotLineLow(x1, y1, x0, y0, color);
        }
        else
        {
            plotLineLow(x0, y0, x1, y1, color);
        }
    }
    else
    {
        if (y0 > y1)
        {
            plotLineHigh(x1, y1, x0, y0, color);
        }
        else
        {
            plotLineHigh(x0, y0, x1, y1, color);
        }
    }
}

void draw_circle(int xc, int yc, int r, int color) 
{ 
    int x = 0, y = r; 
    int d = 3 - 2 * r; 
    dCircle(xc, yc, x, y, color); 
    while (y >= x) 
    { 
        x++; 
        if (d > 0) 
        { 
            y--;  
            d = d + 4 * (x - y) + 10; 
        } 
        else
            d = d + 4 * x + 6; 
        dCircle(xc, yc, x, y, color); 
    } 
} 

void draw_filled_circle(int xc, int yc, int r, int color) 
{ 
    int x = 0, y = r; 
    int d = 3 - 2 * r; 
    dFilledCircle(xc, yc, x, y, color); 
    while (y >= x) 
    { 
        x++; 
        if (d > 0) 
        { 
            y--;  
            d = d + 4 * (x - y) + 10; 
        } 
        else
            d = d + 4 * x + 6; 
        dFilledCircle(xc, yc, x, y, color); 
    } 
} 

void draw_filled_rect(int x0, int y0, int x1, int y1, int color)
{
    order(y0, y1);
    while(y0 < y1)
    {
        hLine(x0, x1, y0, color);
        ++y0;
    }
}

void draw_rect(int x0, int y0, int x1, int y1, int color)
{
    vLine(x0, y0, y1, color);
    vLine(x1, y0, y1, color);
    hLine(x0, x1, y0, color);
    hLine(x0, x1, y1, color);
}

int draw_glyph(uint8_t ch, int x0, int y0, int color, const lv_font_t* font_p)
{
    int fontWidth = lv_font_get_width(font_p, ch);
    if (fontWidth < 0)
    {
        return x0;
    }
    int fontHeight = font_p->h_px;

    if (x0 + fontWidth >= 0)
    {
        y0 = clip(y0, EPD_HEIGHT);

        const uint8_t* glyph = lv_font_get_bitmap(font_p, ch);
        uint8_t* p = framebuffer + EPD_BYTES_PER_ROW*y0 + (x0 >> 3);
        int shift = 8 - (x0 & 0x7);
        for(int i = 0; i < fontHeight; ++i)
        {
            if (y0 + i >= EPD_HEIGHT) break;
            
            uint8_t* dest = p + EPD_BYTES_PER_ROW*i;
            int stride = (fontWidth + 7) >> 3;
            int x = x0;
            for(int j = 0; j < stride; ++j)
            {
                uint16_t bits = glyph[i*stride + j] << shift;
                uint8_t hiByte = (bits >> 8) & 0xff;
                if (hiByte && x >= 0)
                {
                    set_bitmask(dest, hiByte, color);
                }
                ++dest;
                x += 8;
                uint8_t loByte = (bits & 0xff);
                if (loByte && x >= 0 && x < EPD_WIDTH)
                {
                    set_bitmask(dest, loByte, color);
                }
            }
        }
    }
    return x0 + fontWidth;
}

void draw_text(const char* text, int x0, int y0, int color, const lv_font_t * font_p, int xoff, int yoff)
{
    for(const uint8_t* pc = (const uint8_t*)text; *pc && x0 < EPD_WIDTH; pc++)
    {
        x0 = xoff + draw_glyph(*pc, x0, y0, color, font_p);
    }
}
