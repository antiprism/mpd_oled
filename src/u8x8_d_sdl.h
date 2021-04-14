
#ifndef U8X8_D_SDL_H
#define U8X8_D_SDL_H

#include <libu8g2arm/u8g2.h>

#ifdef __cplusplus
extern "C" {
#endif

int u8g2_SetupBuffer_SDL(u8g2_t *u8g2, const u8g2_cb_t *u8g2_cb, int width, int height);

#ifdef __cplusplus
}
#endif

#endif // U8X8_D_SDL_H

