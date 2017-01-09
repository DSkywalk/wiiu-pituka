/*===========================================
        GRRLIB (GX version) 3.0.1 alpha
        Code     : NoNameNo
        GX hints : RedShade
===========================================*/
#include <ogc/conf.h>
#include <fat.h>
#include "GRRLIB.h"

/******************************************************************************/
/**** FREETYPE START ****/
/* This is a very rough implementation if freetype using GRRLIB */

extern u8 font_ttf[];
extern const u32 font_ttf_size;

#include <ft2build.h> /* I presume you have freetype for the Wii installed */
#include FT_FREETYPE_H

static FT_Library ftLibrary;
static FT_Face ftFace;

/* Static function prototypes */
static void DrawText(const char *string, unsigned int fontSize, void *buffer);
static bool BlitGlyph(FT_Bitmap *bitmap, int offset, int top, void *buffer);
static void BitmapTo4x4RGBA(const unsigned char *src, void *dst, const unsigned int width, const unsigned int height);

extern void GRRLIB_InitFreetype(void) {
	unsigned int error = FT_Init_FreeType(&ftLibrary);
	if (error) {
		exit(0);
	}

	error = FT_New_Memory_Face(ftLibrary, font_ttf, font_ttf_size, 0, &ftFace);
	/* Note: You could also directly load a font from the SD card like this:
	error = FT_New_Face(ftLibrary, "fat3:/apps/myapp/font.ttf", 0, &ftFace); */
	
	if (error == FT_Err_Unknown_File_Format) {
		exit(0);
	} else if (error) {
		/* Some other error */
		exit(0);
	}
}

/* Render the text string to a 4x4RGBA texture, return a pointer to this texture */
extern void *GRRLIB_TextToTexture(const char *string, unsigned int fontSize, unsigned int fontColour) {
	unsigned int error = FT_Set_Pixel_Sizes(ftFace, 0, fontSize);
	if (error) {
		/* Failed to set the font size to the requested size. 
		 * You probably should set a default size or something. 
		 * I'll leave that up to the reader. */
	}

	/* Set the font colour, alpha is determined when we blit the glyphs */
	fontColour = fontColour << 8;

	/* Create a tempory buffer, 640 pixels wide, for freetype draw the glyphs */
	void *tempTextureBuffer = (void*) malloc(640 * (fontSize*2) * 4);
	if (tempTextureBuffer == NULL) {
		/* Oops! Something went wrong! */
		exit(0);
	}
	
	/* Set the RGB pixels in tempTextureBuffer to the requested colour */
	unsigned int *p = tempTextureBuffer;
	unsigned int loop = 0;
	for (loop = 0; loop < (640 * (fontSize*2)); ++loop) {
		*(p++) = fontColour;	
	}

	/* Render the glyphs on to the temp buffer */
	DrawText(string, fontSize, tempTextureBuffer);

	/* Create a new buffer, this time to hold the final texture 
	 * in a format suitable for the Wii */
	void *texture = memalign(32, 640 * (fontSize*2) * 4);

	/* Convert the RGBA temp buffer to a format usuable by GX */
	BitmapTo4x4RGBA(tempTextureBuffer, texture, 640, (fontSize*2));
	DCFlushRange(texture, 640 * (fontSize*2) * 4);

	/* The temp buffer is no longer required */
	free(tempTextureBuffer);

	return texture;
}

static void DrawText(const char *string, unsigned int fontSize, void *buffer) {
	unsigned int error = 0;
	int penX = 0;
	int penY = fontSize;
	FT_GlyphSlot slot = ftFace->glyph;
	FT_UInt glyphIndex = 0;
	FT_UInt previousGlyph = 0;
	FT_Bool hasKerning = FT_HAS_KERNING(ftFace);

	/* Convert the string to UTF32 */
	size_t length = strlen(string);
	wchar_t *utf32 = (wchar_t*)malloc(length * sizeof(wchar_t)); 
	length = mbstowcs(utf32, string, length);
	
	/* Loop over each character, drawing it on to the buffer, until the 
	 * end of the string is reached, or until the pixel width is too wide */
	unsigned int loop = 0;
	for (loop = 0; loop < length; ++loop) {
		glyphIndex = FT_Get_Char_Index(ftFace, utf32[ loop ]);
		
		/* To the best of my knowledge, none of the other freetype 
		 * implementations use kerning, so my method ends up looking
		 * slightly better :) */
		if (hasKerning && previousGlyph && glyphIndex) {
			FT_Vector delta;
			FT_Get_Kerning(ftFace, previousGlyph, glyphIndex, FT_KERNING_DEFAULT, &delta);
			penX += delta.x >> 6;
		}
	
		error = FT_Load_Glyph(ftFace, glyphIndex, FT_LOAD_RENDER);
		if (error) {
			/* Whoops, something went wrong trying to load the glyph 
			 * for this character... you should handle this better */
			continue;
		}
	
		if (BlitGlyph(&slot->bitmap, penX + slot->bitmap_left, penY - slot->bitmap_top, buffer) == true) {
			/* The glyph was successfully blitted to the buffer, move the pen forwards */
			penX += slot->advance.x >> 6;
			previousGlyph = glyphIndex;
		} else {
			/* BlitGlyph returned false, the line must be full */
			free(utf32);
			return;
		}
	}

	free(utf32);
}

/* Returns true if the character was draw on to the buffer, false if otherwise */
static bool BlitGlyph(FT_Bitmap *bitmap, int offset, int top, void *buffer) {
	int bitmapWidth = bitmap->width;
	int bitmapHeight = bitmap->rows;

	if (offset + bitmapWidth > 640) {
		/* Drawing this character would over run the buffer, so don't draw it */
		return false;
	}

	/* Draw the glyph onto the buffer, blitting from the bottom up */
	/* CREDIT: Derived from a function by DragonMinded */
	unsigned char *p = buffer;
	unsigned int y = 0;
	for (y = 0; y < bitmapHeight; ++y) {
		int sywidth = y * bitmapWidth;
		int dywidth = (y + top) * 640;

		unsigned int column = 0;
		for (column = 0; column < bitmapWidth; ++column) {
			unsigned int srcloc = column + sywidth;
			unsigned int dstloc = ((column + offset) + dywidth) << 2;
			/* Copy the alpha value for this pixel into the texture buffer */
			p[ dstloc + 3 ] = bitmap->buffer[ srcloc ];
		}
	}
	
	return true;
}

static void BitmapTo4x4RGBA(const unsigned char *src, void *dst, const unsigned int width, const unsigned int height) {
	unsigned int block = 0;
	unsigned int i = 0;
	unsigned int c = 0;
	unsigned int ar = 0;
	unsigned int gb = 0;
	unsigned char *p = (unsigned char*)dst;

	for (block = 0; block < height; block += 4) {
		for (i = 0; i < width; i += 4) {
			/* Alpha and Red */
			for (c = 0; c < 4; ++c) {
				for (ar = 0; ar < 4; ++ar) {
					/* Alpha pixels */
					*p++ = src[(((i + ar) + ((block + c) * width)) * 4) + 3];
					/* Red pixels */	
					*p++ = src[((i + ar) + ((block + c) * width)) * 4];
				}
			}
			
			/* Green and Blue */
			for (c = 0; c < 4; ++c) {
				for (gb = 0; gb < 4; ++gb) {
					/* Green pixels */
					*p++ = src[(((i + gb) + ((block + c) * width)) * 4) + 1];
					/* Blue pixels */
					*p++ = src[(((i + gb) + ((block + c) * width)) * 4) + 2];
				}
			}
		} /* i */
	} /* block */
}

/**** FREETYPE END ****/
/******************************************************************************/

#define DEFAULT_FIFO_SIZE (256 * 1024)

 u32 fb=0;
 static void *xfb[2] = { NULL, NULL};
 GXRModeObj *rmode;
 void *gp_fifo = NULL;


void GRRLIB_FillScreen(u32 color){
	GRRLIB_Rectangle(-40, -40, 680,520, color, 1);
}

void GRRLIB_Plot(f32 x,f32 y, u32 color){
   guVector  v[]={{x,y,0.0f}};
   GXColor c[]={GRRLIB_Splitu32(color)};
	
	GRRLIB_NPlot(v,c,1);
}
void GRRLIB_NPlot(guVector v[],GXColor c[],long n){
	GRRLIB_GXEngine(v,c,n,GX_POINTS);
}

void GRRLIB_Line(f32 x1, f32 y1, f32 x2, f32 y2, u32 color){
   guVector v[]={{x1,y1,0.0f},{x2,y2,0.0f}};
   GXColor col = GRRLIB_Splitu32(color);
   GXColor c[]={col,col};

	GRRLIB_NGone(v,c,2);
}

void GRRLIB_Rectangle(f32 x, f32 y, f32 width, f32 height, u32 color, u8 filled){
   guVector v[]={{x,y,0.0f},{x+width,y,0.0f},{x+width,y+height,0.0f},{x,y+height,0.0f},{x,y,0.0f}};
   GXColor col = GRRLIB_Splitu32(color);
   GXColor c[]={col,col,col,col,col};

	if(!filled){
		GRRLIB_NGone(v,c,5);
	}
	else{
		GRRLIB_NGoneFilled(v,c,4);
	}
}
void GRRLIB_NGone(guVector v[],GXColor c[],long n){
	GRRLIB_GXEngine(v,c,n,GX_LINESTRIP);
}
void GRRLIB_NGoneFilled(guVector v[],GXColor c[],long n){
	GRRLIB_GXEngine(v,c,n,GX_TRIANGLEFAN);
}




u8 * GRRLIB_LoadTexture(const unsigned char my_png[]) {
   PNGUPROP imgProp;
   IMGCTX ctx;
   void *my_texture;

   	ctx = PNGU_SelectImageFromBuffer(my_png);
        PNGU_GetImageProperties (ctx, &imgProp);
        my_texture = memalign (32, imgProp.imgWidth * imgProp.imgHeight * 4);
        PNGU_DecodeTo4x4RGBA8 (ctx, imgProp.imgWidth, imgProp.imgHeight, my_texture, 255);
        PNGU_ReleaseImageContext (ctx);
        DCFlushRange (my_texture, imgProp.imgWidth * imgProp.imgHeight * 4);
	return my_texture;
}

#define BFR_CORRECTION_X 0
#define BFR_CORRECTION_Y 15

void GRRLIB_DrawBuffer(f32 xpos, f32 ypos, u16 width, u16 height, u8 data[], float degrees, float scaleX, f32 scaleY, u8 alpha ){
   GXTexObj texObj;

  	GX_InvVtxCache();      // elimina los problemas 
  	GX_InvalidateTexAll(); // con los bordes

	GX_SetTevOp (GX_TEVSTAGE0, GX_MODULATE);
  	GX_SetVtxDesc (GX_VA_TEX0, GX_DIRECT);

	GX_InitTexObj(&texObj, data, width,height, GX_TF_RGB565 ,GX_CLAMP, GX_CLAMP,GX_FALSE);
	GX_LoadTexObj(&texObj, GX_TEXMAP0);

	Mtx m,m1,m2, mv;

	guMtxIdentity (m1);
	guMtxScaleApply(m1,m1,scaleX,scaleY,1.0);
	guVector axis =(guVector) {0 , 0, 1 };
	guMtxRotAxisDeg (m2, &axis, degrees);
	guMtxConcat(m2,m1,m);

	guMtxTransApply(m,m, xpos+BFR_CORRECTION_X,ypos+BFR_CORRECTION_Y,0);
	guMtxConcat (GXmodelView2D, m, mv);
	GX_LoadPosMtxImm (mv, GX_PNMTX0);
	
	GX_Begin(GX_QUADS, GX_VTXFMT0,4);
  	GX_Position3f32(0, 0,  0);
  	GX_Color4u8(0xFF,0xFF,0xFF,alpha);
  	GX_TexCoord2f32(0, 0);
  
  	GX_Position3f32(width, 0,  0);
 	GX_Color4u8(0xFF,0xFF,0xFF,alpha);
  	GX_TexCoord2f32(1, 0);
  
  	GX_Position3f32(width, height,  0);
	GX_Color4u8(0xFF,0xFF,0xFF,alpha);
  	GX_TexCoord2f32(1, 1);
  
  	GX_Position3f32(0, height,  0);
	GX_Color4u8(0xFF,0xFF,0xFF,alpha);
  	GX_TexCoord2f32(0, 1);
	GX_End();
	GX_LoadPosMtxImm (GXmodelView2D, GX_PNMTX0);

	GX_SetTevOp (GX_TEVSTAGE0, GX_PASSCLR);
  	GX_SetVtxDesc (GX_VA_TEX0, GX_NONE);

}

void GRRLIB_DrawImg(f32 xpos, f32 ypos, u16 width, u16 height, u8 data[], float degrees, float scaleX, f32 scaleY, u8 alpha ){
   GXTexObj texObj;

	
	GX_InitTexObj(&texObj, data, width,height, GX_TF_RGBA8,GX_CLAMP, GX_CLAMP,GX_FALSE);
	//GX_InitTexObjLOD(&texObj, GX_NEAR, GX_NEAR, 0.0f, 0.0f, 0.0f, 0, 0, GX_ANISO_1);
	GX_LoadTexObj(&texObj, GX_TEXMAP0);

	GX_SetTevOp (GX_TEVSTAGE0, GX_MODULATE);
  	GX_SetVtxDesc (GX_VA_TEX0, GX_DIRECT);

	Mtx m,m1,m2, mv;
	width *=.5;
	height*=.5;
	guMtxIdentity (m1);
	guMtxScaleApply(m1,m1,scaleX,scaleY,1.0);
	guVector axis =(guVector) {0 , 0, 1 };
	guMtxRotAxisDeg (m2, &axis, degrees);
	guMtxConcat(m2,m1,m);

	guMtxTransApply(m,m, xpos+width,ypos+height,0);
	guMtxConcat (GXmodelView2D, m, mv);
	GX_LoadPosMtxImm (mv, GX_PNMTX0);
	
	GX_Begin(GX_QUADS, GX_VTXFMT0,4);
  	GX_Position3f32(-width, -height,  0);
  	GX_Color4u8(0xFF,0xFF,0xFF,alpha);
  	GX_TexCoord2f32(0, 0);
  
  	GX_Position3f32(width, -height,  0);
 	GX_Color4u8(0xFF,0xFF,0xFF,alpha);
  	GX_TexCoord2f32(1, 0);
  
  	GX_Position3f32(width, height,  0);
	GX_Color4u8(0xFF,0xFF,0xFF,alpha);
  	GX_TexCoord2f32(1, 1);
  
  	GX_Position3f32(-width, height,  0);
	GX_Color4u8(0xFF,0xFF,0xFF,alpha);
  	GX_TexCoord2f32(0, 1);
	GX_End();
	GX_LoadPosMtxImm (GXmodelView2D, GX_PNMTX0);

	GX_SetTevOp (GX_TEVSTAGE0, GX_PASSCLR);
  	GX_SetVtxDesc (GX_VA_TEX0, GX_NONE);

}

void GRRLIB_DrawTile(f32 xpos, f32 ypos, u16 width, u16 height, u8 data[], float degrees, float scaleX, f32 scaleY, u8 alpha, f32 frame,f32 maxframe ){
GXTexObj texObj;
f32 s1= frame/maxframe;
f32 s2= (frame+1)/maxframe;
f32 t1=0;
f32 t2=1;
	
	GX_InitTexObj(&texObj, data, width*maxframe,height, GX_TF_RGBA8,GX_CLAMP, GX_CLAMP,GX_FALSE);
	GX_InitTexObjLOD(&texObj, GX_NEAR, GX_NEAR, 0.0f, 0.0f, 0.0f, 0, 0, GX_ANISO_1);
	GX_LoadTexObj(&texObj, GX_TEXMAP0);

	GX_SetTevOp (GX_TEVSTAGE0, GX_MODULATE);
  	GX_SetVtxDesc (GX_VA_TEX0, GX_DIRECT);

	Mtx m,m1,m2, mv;
	width *=.5;
	height*=.5;
	guMtxIdentity (m1);
	guMtxScaleApply(m1,m1,scaleX,scaleY,1.0);
	guVector axis =(guVector) {0 , 0, 1 };
	guMtxRotAxisDeg (m2, &axis, degrees);
	guMtxConcat(m2,m1,m);
	guMtxTransApply(m,m, xpos+width,ypos+height,0);
	guMtxConcat (GXmodelView2D, m, mv);
	GX_LoadPosMtxImm (mv, GX_PNMTX0);
	GX_Begin(GX_QUADS, GX_VTXFMT0,4);
  	GX_Position3f32(-width, -height,  0);
  	GX_Color4u8(0xFF,0xFF,0xFF,alpha);
  	GX_TexCoord2f32(s1, t1);
  
  	GX_Position3f32(width, -height,  0);
 	GX_Color4u8(0xFF,0xFF,0xFF,alpha);
  	GX_TexCoord2f32(s2, t1);
  
  	GX_Position3f32(width, height,  0);
	GX_Color4u8(0xFF,0xFF,0xFF,alpha);
  	GX_TexCoord2f32(s2, t2);
  
  	GX_Position3f32(-width, height,  0);
	GX_Color4u8(0xFF,0xFF,0xFF,alpha);
  	GX_TexCoord2f32(s1, t2);
	GX_End();
	GX_LoadPosMtxImm (GXmodelView2D, GX_PNMTX0);

	GX_SetTevOp (GX_TEVSTAGE0, GX_PASSCLR);
  	GX_SetVtxDesc (GX_VA_TEX0, GX_NONE);

}

// This version adds ability to have a font with different number of characters wide and high instead of hardcoded to 16x16
void GRRLIB_DrawGChar(f32 xpos, f32 ypos, u16 width, u16 height, u8 data[], u16 chars_wide, u16 chars_high, float degrees, float scaleX, f32 scaleY, u8 frame, GXColor c ){
GXTexObj texObj;
f32 s1= ((frame    %chars_wide))    /(f32)chars_wide;
f32 s2= ((frame    %chars_wide)+1)    /(f32)chars_wide;
f32 t1= ((frame    /chars_wide))    /(f32)chars_high;
f32 t2= ((frame    /chars_wide)+1)    /(f32)chars_high;

    GX_InitTexObj(&texObj, data, width*chars_wide,height*chars_high, GX_TF_RGBA8,GX_CLAMP, GX_CLAMP,GX_FALSE);
    GX_InitTexObjLOD(&texObj, GX_NEAR, GX_NEAR, 0.0f, 0.0f, 0.0f, 0, 0, GX_ANISO_1);
    GX_LoadTexObj(&texObj, GX_TEXMAP0);

    GX_SetTevOp (GX_TEVSTAGE0, GX_MODULATE);
      GX_SetVtxDesc (GX_VA_TEX0, GX_DIRECT);

    Mtx m,m1,m2,mv;
    width *=.5;
    height*=.5;

    guMtxIdentity (m1);
    guMtxScaleApply(m1,m1,scaleX,scaleY,1.0);
    guVector axis =(guVector) {0,0,1};
    guMtxRotAxisDeg (m2, &axis, degrees);
    guMtxConcat(m2,m1,m);
    guMtxTransApply(m,m, xpos+width,ypos+height,0);
    guMtxConcat (GXmodelView2D, m, mv);
    GX_LoadPosMtxImm (mv, GX_PNMTX0);

    GX_Begin(GX_QUADS, GX_VTXFMT0,4);

          GX_Position3f32(-width, -height,  0);
          GX_Color4u8(c.r,c.g,c.b,c.a);
          GX_TexCoord2f32(s1, t1);

          GX_Position3f32(width, -height,  0);
         GX_Color4u8(c.r,c.g,c.b,c.a);
          GX_TexCoord2f32(s2, t1);

          GX_Position3f32(width, height,  0);
        GX_Color4u8(c.r,c.g,c.b,c.a);
          GX_TexCoord2f32(s2, t2);

          GX_Position3f32(-width, height,  0);
        GX_Color4u8(c.r,c.g,c.b,c.a);
          GX_TexCoord2f32(s1, t2);

    GX_End();

    GX_LoadPosMtxImm (GXmodelView2D, GX_PNMTX0);
    GX_SetTevOp (GX_TEVSTAGE0, GX_PASSCLR);
      GX_SetVtxDesc (GX_VA_TEX0, GX_NONE);
}

/*
    This version of GRRLIB_Printf takes in a fontmap along with the number of character wide and high of the the font.
    The fontmap is just a terminated string containing the characters that appear in the font in the position they
    appear. e.g. "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789:-+!" I would advise this to part of the font file along with
    the number of characters wide and high.

    It will not output anything for a character that is not in present in the fontmap
*/
void GRRLIB_GPrintf(f32 xpos, f32 ypos, u8 font[], u16 font_width, u16 font_height, char fontmap[], u16 chars_wide, u16 chars_high, u32 color, f32 zoom, char *text,...){
    int i,size=0;
    char tmp[1024];

    va_list argp;
    va_start(argp, text);
    size = vsprintf(tmp, text, argp);
    va_end(argp);

    GXColor col = GRRLIB_Splitu32(color);
    for(i=0;i<size;i++){
        char * pch = strchr(fontmap, tmp[i]);
        if(pch!=NULL) {
            GRRLIB_DrawGChar(xpos+i * font_width * zoom, ypos, font_width, font_height, font, chars_wide, chars_high, 0, zoom, zoom, pch-fontmap, col );
        }
    }
}

void GRRLIB_Printf(f32 xpos, f32 ypos, u8 font[], u16 font_width, u16 font_height, u16 chars_wide, u16 chars_high, int chars_space, u32 color, f32 zoom, char *text,...){
        int i,size=0;
        char tmp[1024];

        va_list argp;
        va_start(argp, text);
        size = vsprintf(tmp, text, argp);
        va_end(argp);

        GXColor col = GRRLIB_Splitu32(color);

        for(i=0;i<size;i++){
                u8 c = tmp[i];
		GRRLIB_DrawGChar(xpos+i * (font_width + chars_space) * zoom, ypos, font_width, font_height, font, chars_wide, chars_high, 0, zoom, zoom, c, col );
        }
}


void GRRLIB_GXEngine(guVector v[], GXColor c[], long n,u8 fmt){
   int i=0;	

	GX_Begin(fmt, GX_VTXFMT0,n);
	for(i=0;i<n;i++){
  		GX_Position3f32(v[i].x, v[i].y,  v[i].z);
  		GX_Color4u8(c[i].r, c[i].g, c[i].b, c[i].a);
  	}
	GX_End();
}

GXColor GRRLIB_Splitu32(u32 color){
   u8 a,r,g,b;

	a = (color >> 24) & 0xFF; 
	r = (color >> 16) & 0xFF; 
	g = (color >> 8) & 0xFF; 
	b = (color) & 0xFF; 

	return (GXColor){r,g,b,a};
}




//********************************************************************************************
void GRRLIB_InitVideo () {

	rmode = VIDEO_GetPreferredMode(NULL);


        if (rmode == NULL)
            return;

        switch (rmode->viTVMode) 
        {
            case VI_DEBUG_PAL:      // PAL 50hz 576i
                    rmode = &TVPal528IntDf; //&TVPal574IntDfScale;
                    break;
        }

        // Widescreen patch by CashMan
 	if (CONF_GetAspectRatio() == CONF_ASPECT_16_9)
 	{
              if(VIDEO_HaveComponentCable())
              {
	          rmode->viWidth = VI_MAX_WIDTH_PAL-12;
	          rmode->viXOrigin = ((VI_MAX_WIDTH_PAL - rmode->viWidth) / 2) + 2;
              }
              else
              {
 	          rmode->viWidth = 678;
 	          rmode->viXOrigin = (VI_MAX_WIDTH_NTSC - 678)/2;
              }
 	}

	VIDEO_Configure (rmode);
	xfb[0] = (u32 *)MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode)); 
	xfb[1] = (u32 *)MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));

	// Initialise the console, required for printf
        //TODO: CREAR UN FB PARA LA CONSOLA PRINTF, etc..
	console_init (xfb[0], 20, 64, rmode->fbWidth, rmode->xfbHeight, rmode->fbWidth * 2);

        /*** Clear framebuffer to black ***/
        VIDEO_ClearFrameBuffer (rmode, xfb[0], COLOR_BLACK);
        VIDEO_ClearFrameBuffer (rmode, xfb[1], COLOR_BLACK);

        /*** Set the framebuffer to be displayed at next VBlank ***/
        VIDEO_SetNextFramebuffer (xfb[0]);

	VIDEO_SetBlack(0);
	VIDEO_Flush();
	VIDEO_WaitVSync();
	if(rmode->viTVMode&VI_NON_INTERLACE) VIDEO_WaitVSync();

	gp_fifo = (u8 *) memalign(32,DEFAULT_FIFO_SIZE);

	// The console understands VT terminal escape codes
	// This positions the cursor on row 2, column 0
	// we can use variables for this with format codes too
	// e.g. printf ("\x1b[%d;%dH", row, column );
	printf("\x1b[2;0H");

}

void GRRLIB_Stop() {
    free(MEM_K1_TO_K0(xfb[0])); xfb[0] = NULL;
    free(MEM_K1_TO_K0(xfb[1])); xfb[1] = NULL;
    free(gp_fifo); gp_fifo = NULL;

    GX_AbortFrame();
    GX_Flush();
}


void GRRLIB_Start(){
  
   f32 yscale;
   u32 xfbHeight;

   Mtx perspective;

	GX_Init (gp_fifo, DEFAULT_FIFO_SIZE);

        // Clears the BG to color and clears the z-buffer
        GX_SetCopyClear((GXColor){ 0, 0, 0, 0xff }, GX_MAX_Z24);

	// other gx setup
	yscale = GX_GetYScaleFactor(rmode->efbHeight,rmode->xfbHeight);
	xfbHeight = GX_SetDispCopyYScale(yscale);
	GX_SetScissor(0,0,rmode->fbWidth,rmode->efbHeight);
	GX_SetDispCopySrc(0,0,rmode->fbWidth,rmode->efbHeight);
	GX_SetDispCopyDst(rmode->fbWidth,xfbHeight);
	GX_SetCopyFilter(rmode->aa,rmode->sample_pattern,GX_TRUE,rmode->vfilter);
	GX_SetFieldMode(rmode->field_rendering,((rmode->viHeight==2*rmode->xfbHeight)?GX_ENABLE:GX_DISABLE));

	if (rmode->aa)
		GX_SetPixelFmt(GX_PF_RGB565_Z16, GX_ZC_LINEAR);
	else
		GX_SetPixelFmt(GX_PF_RGB8_Z24, GX_ZC_LINEAR);

	GX_SetDispCopyGamma(GX_GM_1_0);

	// setup the vertex descriptor
	// tells the flipper to expect direct data
	GX_ClearVtxDesc();
	GX_InvVtxCache ();
	GX_InvalidateTexAll();

	GX_SetVtxDesc(GX_VA_TEX0, GX_NONE);
	GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
	GX_SetVtxDesc (GX_VA_CLR0, GX_DIRECT);


	GX_SetVtxAttrFmt (GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
	GX_SetVtxAttrFmt (GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
	GX_SetZMode (GX_FALSE, GX_LEQUAL, GX_TRUE);

	GX_SetNumChans(1);
	GX_SetNumTexGens(1);
	GX_SetTevOp (GX_TEVSTAGE0, GX_PASSCLR);
	GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
	GX_SetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY);

	guMtxIdentity(GXmodelView2D);
	guMtxTransApply (GXmodelView2D, GXmodelView2D, 0.0F, 0.0F, -50.0F);
	GX_LoadPosMtxImm(GXmodelView2D,GX_PNMTX0);

	guOrtho(perspective,0,479,0,639,0,300);
	GX_LoadProjectionMtx(perspective, GX_ORTHOGRAPHIC);

	GX_SetViewport(0,0,rmode->fbWidth,rmode->efbHeight,0,1);
	GX_SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR);
	GX_SetAlphaUpdate(GX_TRUE);
	
	GX_SetCullMode(GX_CULL_NONE);
}
void GRRLIB_Start_org(){
   
   f32 yscale;
   u32 xfbHeight;
   Mtx perspective;

	GX_Init (gp_fifo, DEFAULT_FIFO_SIZE);

	// clears the bg to color and clears the z buffer
	GXColor background = { 0, 0, 0, 0xff };
	GX_SetCopyClear (background, 0x00ffffff);

	//VIDEO_SetBlack(FALSE);

	// other gx setup
	yscale = GX_GetYScaleFactor(rmode->efbHeight,rmode->xfbHeight);
	xfbHeight = GX_SetDispCopyYScale(yscale);
	GX_SetScissor(0,0,rmode->fbWidth,rmode->efbHeight);
	GX_SetDispCopySrc(0,0,rmode->fbWidth,rmode->efbHeight);
	GX_SetDispCopyDst(rmode->fbWidth,xfbHeight);
	GX_SetCopyFilter(rmode->aa,rmode->sample_pattern,GX_TRUE,rmode->vfilter);
	GX_SetFieldMode(rmode->field_rendering,((rmode->viHeight==2*rmode->xfbHeight)?GX_ENABLE:GX_DISABLE));

	if (rmode->aa)
		GX_SetPixelFmt(GX_PF_RGB565_Z16, GX_ZC_LINEAR);
	else
		GX_SetPixelFmt(GX_PF_RGB8_Z24, GX_ZC_LINEAR);

	GX_SetDispCopyGamma(GX_GM_1_0);
 

	// setup the vertex descriptor
	// tells the flipper to expect direct data
	GX_ClearVtxDesc();
		GX_InvVtxCache ();
		GX_InvalidateTexAll();

	GX_SetVtxDesc(GX_VA_TEX0, GX_NONE);
	GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
	GX_SetVtxDesc (GX_VA_CLR0, GX_DIRECT);


		GX_SetVtxAttrFmt (GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
		GX_SetVtxAttrFmt (GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
		GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
		GX_SetZMode (GX_FALSE, GX_LEQUAL, GX_TRUE);

	GX_SetNumChans(1);
	GX_SetNumTexGens(1);
	GX_SetTevOp (GX_TEVSTAGE0, GX_PASSCLR);
	GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
	GX_SetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY);

	guMtxIdentity(GXmodelView2D);
	guMtxTransApply (GXmodelView2D, GXmodelView2D, 0.0F, 0.0F, -50.0F);
	GX_LoadPosMtxImm(GXmodelView2D,GX_PNMTX0);

	guOrtho(perspective,0,479,0,639,0,300);
	GX_LoadProjectionMtx(perspective, GX_ORTHOGRAPHIC);

	GX_SetViewport(0,0,rmode->fbWidth,rmode->efbHeight,0,1);
	GX_SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR);
	GX_SetAlphaUpdate(GX_TRUE);
	
	GX_SetCullMode(GX_CULL_NONE);


}

void GRRLIB_Render () {
        GX_DrawDone ();

	fb ^= 1;		// flip framebuffer
	GX_SetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
	GX_SetColorUpdate(GX_TRUE);
	GX_CopyDisp(xfb[fb],GX_TRUE);
	VIDEO_SetNextFramebuffer(xfb[fb]);
 	VIDEO_Flush();
 	//VIDEO_WaitVSync();

}

void GRRLIB_VSync () {
 	VIDEO_WaitVSync();
}


/**
 * Make a PNG screenshot on the SD card.
 * @param File Name of the file to write.
 * @return True if every thing worked, false otherwise.
 */
bool GRRLIB_ScrShot(const char* File) {
    IMGCTX ctx;
	int ErrorCode = -1;

    if(fatInitDefault() && (ctx = PNGU_SelectImageFromDevice(File)))
    {
		ErrorCode = PNGU_EncodeFromYCbYCr(ctx, 640, 480, xfb[fb], 0);
        PNGU_ReleaseImageContext(ctx);

	fatUnmount("sd");
    }
	return !ErrorCode;
}

/**
 * Make a snapshot of the sreen in a texture.
 * @return A texture representing the screen.
 */
u8 *GRRLIB_Screen2Texture() {
	void *my_texture;

	GX_SetTexCopySrc(0, 0, 640, 480);
	GX_SetTexCopyDst(640, 480, GX_TF_RGBA8, GX_FALSE);
	my_texture = memalign(32, 640 * 480 * 4); // GX_GetTexBufferSize(640, 480, GX_TF_RGBA8, GX_FALSE, 1)
	GX_CopyTex(my_texture, GX_FALSE);
	GX_PixModeSync();
	return my_texture;
}

/**
 * Fade in, than fade out
 * @param width	 Texture width.
 * @param height Texture height.
 * @param data   Texture.
 * @param scaleX Texture X scale.
 * @param scaleY Texture Y scale.
 * @param speed  Fade speed (1 is the normal speed, 2 is two time the normal speed, etc).
 */
void GRRLIB_DrawImg_FadeInOut(u16 width, u16 height, u8 data[], float scaleX, f32 scaleY, u16 speed)
{
	GRRLIB_DrawImg_FadeIn(width, height, data, scaleX, scaleY, speed);
	GRRLIB_DrawImg_FadeOut(width, height, data, scaleX, scaleY, speed);
}

/**
 * Fade in
 * @param width	 Texture width.
 * @param height Texture height.
 * @param data   Texture.
 * @param scaleX Texture X scale.
 * @param scaleY Texture Y scale.
 * @param speed  Fade speed (1 is the normal speed, 2 is two time the normal speed, etc).
 */
void GRRLIB_DrawImg_FadeIn(u16 width, u16 height, u8 data[], float scaleX, f32 scaleY, u16 speed)
{
	int alpha;
	f32 xpos = (640 - width) / 2;
	f32 ypos = (480 - height) / 2;

	for(alpha = 0; alpha < 255; alpha += speed)
	{
		GRRLIB_DrawImg(xpos, ypos, width, height, data, 0, scaleX, scaleY, alpha);
		GRRLIB_Render();
	}
	GRRLIB_DrawImg(xpos, ypos, width, height, data, 0, scaleX, scaleY, 255);
	GRRLIB_Render();
}

/**
 * Fade out
 * @param width	 Texture width.
 * @param height Texture height.
 * @param data   Texture.
 * @param scaleX Texture X scale.
 * @param scaleY Texture Y scale.
 * @param speed  Fade speed (1 is the normal speed, 2 is two time the normal speed, etc).
 */
void GRRLIB_DrawImg_FadeOut(u16 width, u16 height, u8 data[], float scaleX, f32 scaleY, u16 speed)
{
	int alpha;
	f32 xpos = (640 - width) / 2;
	f32 ypos = (480 - height) / 2;

	for(alpha = 255; alpha > 0; alpha -= speed)
	{
		GRRLIB_DrawImg(xpos, ypos, width, height, data, 0, scaleX, scaleY, alpha);
		GRRLIB_Render();
	}
	GRRLIB_DrawImg(xpos, ypos, width, height, data, 0, scaleX, scaleY, 0);
	GRRLIB_Render();
}
