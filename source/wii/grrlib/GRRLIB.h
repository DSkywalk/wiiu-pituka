/*===========================================
        GRRLIB (GX version) 3.0.1 alpha
        Code     : NoNameNo
        GX hints : RedShade
===========================================*/

#ifndef __GXHDR__
#define __GXHDR__
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <malloc.h>
#include <math.h>
#include <gccore.h>

#include "../libpngu/pngu.h"

extern Mtx GXmodelView2D;

/**** FREETYPE START ****/
extern void GRRLIB_InitFreetype();
extern void *GRRLIB_TextToTexture(const char *string, unsigned int fontSize, unsigned int fontColour);
/**** FREETYPE END ****/

void GRRLIB_FillScreen(u32 color);

void GRRLIB_Plot(f32 x,f32 y, u32 color);
void GRRLIB_NPlot(guVector v[],GXColor c[],long n);

void GRRLIB_Line(f32 x1, f32 y1, f32 x2, f32 y2, u32 color);

void GRRLIB_Rectangle(f32 x, f32 y, f32 width, f32 height, u32 color, u8 filled);
void GRRLIB_NGone(guVector v[],GXColor c[],long n);
void GRRLIB_NGoneFilled(guVector v[],GXColor c[],long n);


/*
 SKY UPDATE
  DISPLAY A 4x4 RGB565 TEXTURE
*/
void GRRLIB_DrawBuffer(f32 xpos, f32 ypos, u16 width, u16 height, u8 data[], float degrees, float scaleX, f32 scaleY, u8 alpha );
void GRRLIB_printf(f32 xpos, f32 ypos, u8 data[], u32 color, u16 charHeight, u16 charWidth, f32 zoom, const char *text,...);


u8 * GRRLIB_LoadTexture(const unsigned char my_png[]);
void GRRLIB_DrawImg(f32 xpos, f32 ypos, u16 width, u16 height, u8 data[], float degrees, float scaleX, f32 scaleY, u8 alphaF );
void GRRLIB_DrawTile(f32 xpos, f32 ypos, u16 width, u16 height, u8 data[], float degrees, float scaleX, f32 scaleY, u8 alpha, f32 frame,f32 maxframe );

//void GRRLIB_DrawChar(f32 xpos, f32 ypos, u16 width, u16 height, u8 data[], float degrees, float scaleX, f32 scaleY, u8 frame,f32 maxframe, GXColor c );
//void GRRLIB_Printf(f32 xpos, f32 ypos, u8 data[], u32 color, f32 zoom, const char* text,...);
//void GRRLIB_Printf(f32 xpos, f32 ypos, u8 font[],u16 font_width ,u16 font_height , u32 color, f32 zoom, char *text,...);
void GRRLIB_Printf(f32 xpos, f32 ypos, u8 font[], u16 font_width, u16 font_height, u16 chars_wide, u16 chars_high, int chars_space, u32 color, f32 zoom, char *text,...);
//void GRRLIB_DrawGChar(f32 xpos, f32 ypos, u16 width, u16 height, u8 data[], u16 chars_wide, u16 chars_high,float degrees, float scaleX, f32 scaleY, u8 frame, GXColor c);
void GRRLIB_GPrintf(f32 xpos, f32 ypos, u8 font[], u16 font_width, u16 font_height, char *fontmap, u16 chars_wide, u16 chars_high, u32 color, f32 zoom, char *text,...);
void GRRLIB_Printf_ORG(f32 xpos, f32 ypos, u8 data[], u32 color, u16 charWidth, u16 charHeight, f32 zoom, const char *text,...);

GXColor GRRLIB_Splitu32(u32 color);

void GRRLIB_GXEngine(guVector v[], GXColor c[], long count,u8 fmt);


void GRRLIB_InitVideo ();
void GRRLIB_Start();
void GRRLIB_Stop();
void GRRLIB_Render ();
void GRRLIB_VSync ();
bool GRRLIB_ScrShot(const char*);
u8 *GRRLIB_Screen2Texture();

void GRRLIB_DrawImg_FadeInOut(u16 width, u16 height, u8 data[], float scaleX, f32 scaleY, u16 speed);
void GRRLIB_DrawImg_FadeIn(u16 width, u16 height, u8 data[], float scaleX, f32 scaleY, u16 speed);
void GRRLIB_DrawImg_FadeOut(u16 width, u16 height, u8 data[], float scaleX, f32 scaleY, u16 speed);

#endif
