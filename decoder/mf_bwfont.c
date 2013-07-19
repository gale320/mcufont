#include "mf_bwfont.h"
#include <stdbool.h>

/* Find the character range and index that contains a given glyph.. */
static const struct mf_bwfont_char_range_s *find_char_range(
    const struct mf_bwfont_s *font, uint16_t character, uint16_t *index_ret)
{
    unsigned i, index;
    const struct mf_bwfont_char_range_s *range;
    for (i = 0; i < font->char_range_count; i++)
    {
        range = &font->char_ranges[i];
        index = character - range->first_char;
        if (character >= range->first_char && index < range->char_count)
        {
            *index_ret = index;
            return range;
        }
    }
    
    return 0;
}

static uint8_t get_width(const struct mf_bwfont_char_range_s *r, uint16_t index)
{
    if (r->width)
    {
        return r->width;
    }
    else
    {
        return r->glyph_offsets[index + 1] - r->glyph_offsets[index];
    }
}

static uint8_t render_char(const struct mf_bwfont_char_range_s *r,
                           int16_t x0, int16_t y0, uint16_t index,
                           mf_pixel_callback_t callback,
                           void *state)
{
    const uint8_t *data, *p;
    uint8_t stride, runlen;
    uint8_t x, y, width, height;
    uint8_t bit, byte, mask;
    bool oldstate, newstate;
    
    if (r->width)
    {
        data = r->glyph_data + r->width * index * r->height_bytes;
    }
    else
    {
        data = r->glyph_data + r->glyph_offsets[index] * r->height_bytes;
    }
    
    stride = r->height_bytes;
    width = get_width(r, index);
    height = r->height_pixels;
    y0 += r->offset_y;
    x0 += r->offset_x;
    bit = 0;
    byte = 0;
    
    for (y = 0; y < height; y++)
    {
        mask = (1 << bit);
        
        oldstate = false;
        runlen = 0;
        p = data + byte;
        for (x = 0; x < width; x++, p += stride)
        {
            newstate = *p & mask;
            if (newstate != oldstate)
            {
                if (oldstate && runlen)
                {
                    callback(x0 + x - runlen, y0 + y, runlen, 255, state);
                }
                
                oldstate = newstate;
                runlen = 0;
            }
            
            runlen++;
        }
        
        if (oldstate && runlen)
        {
            callback(x0 + x - runlen, y0 + y, runlen, 255, state);
        }
        
        bit++;
        if (bit > 7)
        {
            bit = 0;
            byte++;
        }
    }
    
    return width + r->offset_x;
}

uint8_t mf_bwfont_render_character(const struct mf_font_s *font,
                                   int16_t x0, int16_t y0,
                                   uint16_t character,
                                   mf_pixel_callback_t callback,
                                   void *state)
{
    const struct mf_bwfont_s *bwfont = (const struct mf_bwfont_s*)font;
    const struct mf_bwfont_char_range_s *range;
    uint16_t index;
    
    range = find_char_range(bwfont, character, &index);
    if (!range)
        return 0;
    
    return render_char(range, x0, y0, index, callback, state);
}

uint8_t mf_bwfont_character_width(const struct mf_font_s *font,
                                  uint16_t character)
{
    const struct mf_bwfont_s *bwfont = (const struct mf_bwfont_s*)font;
    const struct mf_bwfont_char_range_s *range;
    uint16_t index;
    
    range = find_char_range(bwfont, character, &index);
    if (!range)
        return 0;
    
    return get_width(range, index) + range->offset_x;
}
