#if 0//defined(__ARM_NEON__)

#include "vdrawhelper.h"

#error "NOPE"


void memfill32(uint32_t *dest, uint32_t value, int length)
{
    memset(dest, value, length);
}

static void color_SourceOver(uint32_t *dest, int length,
                                      uint32_t color,
                                     uint32_t const_alpha)
{
    if (const_alpha != 255) color = BYTE_MUL(color, const_alpha);

        for(int i = 0; i < length; i++)
            dest[i] = color;
}

void RenderFuncTable::neon()
{
    updateColor(BlendMode::Src , color_SourceOver);
}
#endif
