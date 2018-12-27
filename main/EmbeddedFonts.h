/*
 * The fonts and methods in this file have been taken from
 * https://github.com/littlevgl/lvgl
 */

/* inclusion guard */
#ifndef __EMBEDDEDFONTS_H__
#define __EMBEDDEDFONTS_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    uint32_t w_px         :8;
    uint32_t glyph_index  :24;
} lv_font_glyph_dsc_t;

typedef struct
{
    uint32_t unicode         :21;
    uint32_t glyph_dsc_index :11;
} lv_font_unicode_map_t;

typedef struct _lv_font_struct
{
    uint32_t unicode_first;
    uint32_t unicode_last;
    const uint8_t * glyph_bitmap;
    const lv_font_glyph_dsc_t * glyph_dsc;
    const uint32_t * unicode_list;
    const uint8_t * (*get_bitmap)(const struct _lv_font_struct *,uint32_t);     /*Get a glyph's  bitmap from a font*/
    int16_t (*get_width)(const struct _lv_font_struct *,uint32_t);        /*Get a glyph's width with a given font*/
    const struct _lv_font_struct * next_page;    /*Pointer to a font extension*/
    uint32_t h_px       :8;
    uint32_t bpp        :4;                /*Bit per pixel: 1, 2 or 4*/
    uint32_t monospace  :8;                /*Fix width (0: normal width)*/
    uint16_t glyph_cnt;                    /*Number of glyphs (letters) in the font*/
} lv_font_t;


/**
 * Return with the bitmap of a font.
 * @param font_p pointer to a font
 * @param letter an UNICODE character code
 * @return  pointer to the bitmap of the letter
 */
extern const uint8_t * lv_font_get_bitmap(const lv_font_t * font_p, uint32_t letter);

/**
 * Get the width of a letter in a font. If `monospace` is set then return with it.
 * @param font_p pointer to a font
 * @param letter an UNICODE character code
 * @return the width of a letter
 */
extern uint8_t lv_font_get_width(const lv_font_t * font_p, uint32_t letter);


/**
 * Get the height of a font
 * @param font_p pointer to a font
 * @return the height of a font
 */
static inline uint8_t lv_font_get_height(const lv_font_t * font_p)
{
    return font_p->h_px;
}

/**
 * Generic bitmap get function used in 'font->get_bitmap' when the font contains all characters in the range
 * @param font pointer to font
 * @param unicode_letter an unicode letter which bitmap should be get
 * @return pointer to the bitmap or NULL if not found
 */
extern const uint8_t * lv_font_get_bitmap_continuous(const lv_font_t * font, uint32_t unicode_letter);

/**
 * Generic glyph width get function used in 'font->get_width' when the font contains all characters in the range
 * @param font pointer to font
 * @param unicode_letter an unicode letter which width should be get
 * @return width of the gylph or -1 if not found
 */
extern int16_t lv_font_get_width_continuous(const lv_font_t * font, uint32_t unicode_letter);


extern const lv_font_t lv_font_dejavu_10;
extern const lv_font_t lv_font_dejavu_20;
extern const lv_font_t lv_font_dejavu_30;
extern const lv_font_t lv_font_dejavu_40;

#ifdef __cplusplus
}
#endif

#endif /* __EMBEDDEDFONTS_H__ */
