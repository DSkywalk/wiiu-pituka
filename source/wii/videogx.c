/*! \file videogx.c
 *  \brief Video Driver.
 *         Wii Library.
 *
 *  \version 0.8
 *
 *  Video para 640x480 (16bits)
 *
 *   Pituka - Nintendo Wii/Gamecube Port
 *  (c) Copyright 2008-2009 David Colmenero (aka D_Skywalk)
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <malloc.h>
#include <math.h>
#include <gccore.h>
#include <wiiuse/wpad.h>
#include <fat.h>

#include "libpngu/pngu.h"
#include "grrlib/GRRLIB.h"
#include "grrlib/GRRLIB_font1.h" //printf font

#include "videowii.h"
#include "menu/menu.h"

#include "../global.h"

Mtx GXmodelView2D;
extern void *pix;
extern GXRModeObj *rmode;

t_img console_fnt;

float screen_x = 0;
float screen_y = 0;
float cpc_zoom = CONST_ZOOM_RATIO;

/*
 * Image Wii Functions
 */

/*! \fn void InitImg (t_img * img)
    \brief Inicializa la estructura img.

    \param img imagen a inicializar, DEBE ESTAR LIBERADA.
    \note La funcion no comprueba si el buffer esta liberado o no.
*/
void InitImg (t_img * img)
{
    img->Width = 0;
    img->Height = 0;
    img->buffer = NULL;
}

/*! \fn void FreeImg (t_img * img)
    \brief Libera y deja en valores nulos la imagen.

    \param img imagen a liberar.
*/
void FreeImg (t_img * img)
{

  if(img->buffer == NULL)
	return;

  free(img->buffer);
  InitImg(img);

}


#define FREETYPE_WIDTH 640
/*! \fn bool LoadFont (t_img * img, const unsigned char my_fnt[], const char * Text, int TextSize, u32 TextColor)
    \brief Renderiza un texto, usando freetype en una imagen.

    \param img Imagen a usar, DEBE ESTAR INICIALIZADA.
    \param my_fnt No usado, en un futuro podria usarse para cargar una ttf especifica.
    \param Text El texto a renderizar.
    \param TextSize Tamaño de la cadena de texto.
    \param TextColor Color a usar al renderizar la fuente.

    \return true, si todo fue correcto.
    \todo La funcion no comprueba si el buffer es NULL.
*/
bool LoadFont (t_img * img, const unsigned char my_fnt[], const char * Text, int TextSize, u32 TextColor)
{

//  ctx = loadTTF(my_fnt);

  img->Width = FREETYPE_WIDTH;
  img->Height = TextSize*2;

  img->buffer = GRRLIB_TextToTexture(Text, TextSize, TextColor);

  return true;
}

/*! \fn bool LoadTexture (t_img * img, const unsigned char my_png[])
    \brief Carga un PNG en la estructura img.

    \param img Imagen a usar, DEBE ESTAR LIBERADA.
    \param my_png Buffer del png a cargar.

    \return true, si todo fue correcto.
    \todo La funcion no comprueba si el buffer esta liberado o no.
*/
bool LoadTexture (t_img * img, const unsigned char my_png[])
{
  PNGUPROP imgProp;
  IMGCTX ctx;

  ctx = PNGU_SelectImageFromBuffer(my_png);
  PNGU_GetImageProperties (ctx, &imgProp);
  PNGU_ReleaseImageContext (ctx);

  img->Width = imgProp.imgWidth;
  img->Height = imgProp.imgHeight;

  img->buffer = GRRLIB_LoadTexture(my_png);

  return true;
}

/*! \fn void DrawTexture (t_img * img, f32 xpos, f32 ypos, float degrees, float scaleX, f32 scaleY, u8 alpha )
    \brief Dibuja la imagen en pantalla.

    \param img Imagen a usar, DEBE ESTAR INICIALIZADA.
    \param xpos Posicion de X donde debe ser dibujada.
    \param ypos Posicion de Y donde debe ser dibujada.
    \param degrees Grados de giro.
    \param scaleX Escala del eje X.
    \param scaleY Escala del eje Y.
    \param alpha Nivel de transparencia.

    \note La funcion no comprueba si el buffer esta liberado o no.
*/
void DrawTexture (t_img * img, f32 xpos, f32 ypos, float degrees, float scaleX, f32 scaleY, u8 alpha )
{

  GRRLIB_DrawImg(xpos, ypos, img->Width, img->Height, img->buffer, degrees, scaleX, scaleY, alpha );

}

#define CONSOLEFNT_CHRCOLS 128
#define CONSOLEFNT_CHRROWS 1
#define CONSOLEFNT_CHRSPACE 0

/*! \fn void PrintW (f32 xpos, f32 ypos, const char *text,...)
    \brief Renderiza un texto de fuente bitmap en una imagen.

    \param xpos Posicion de X donde debe ser dibujado.
    \param ypos Posicion de Y donde debe ser dibujado.
    \param text Texto a renderizar.

*/
void PrintW (f32 xpos, f32 ypos, const char *text,...)
{
	char tmp[1024];
	va_list argp;

	va_start(argp, text);
	vsprintf(tmp, text, argp);
	va_end(argp);

    GRRLIB_Printf(xpos, ypos, console_fnt.buffer, (console_fnt.Width / CONSOLEFNT_CHRCOLS), (console_fnt.Height / CONSOLEFNT_CHRROWS), CONSOLEFNT_CHRCOLS, CONSOLEFNT_CHRROWS, CONSOLEFNT_CHRSPACE, 0xFFFFFFFF, 1, tmp);
}

/*
 * Video System Wii Functions
 */


/*** Texture memory ***/
static u8 *texturemem;
static int texturesize;

/*! \fn void _clear_buffer (void)
    \brief Limpia el buffer usado para renderizar las imagenes.

*/
void _clear_buffer (void)
{
  memset(pix, 0x0, 4 * HRES * WRES);
  memset(texturemem, 0x0, texturesize);
  DCFlushRange (pix, WRES * HRES * 4);
  DCFlushRange (texturemem, texturesize);
}

/*! \fn int _init_buffer (void)
    \brief Inicializa el buffer usado para renderizar las imagenes.

    \return true, si todo termino correctamente.
*/
int _init_buffer (void)
{
  pix = (void *) memalign(32, 4 * HRES * WRES); //calloc(1, 4 * HRES * WRES);
  if(pix == NULL)
      return 0;


  /*** Allocate 32byte aligned texture memory ***/
  texturesize = (HRES * WRES) * 2;
  texturemem = (u8 *) memalign(32, texturesize);

  _clear_buffer();

  return 1;
}

/*! \fn void _close_buffer (void)
    \brief Libera el buffer usado para renderizar las imagenes.

*/
void _close_buffer (void)
{

  if(pix != NULL)
    {
      free(pix);
      pix = NULL;
    }
}

/*! \fn int VideoInit (void)
    \brief Funcion global que inicializa el sistema de video de la Wii.

    \return true, si todo termino correctamente.
*/
int VideoInit (void)
{
	// Video initialization
	VIDEO_Init();
	GRRLIB_InitVideo();
        GRRLIB_InitFreetype(); //init freetype
	GRRLIB_Start();

	// Set Shutdown Callbacks.
	//SYS_SetPowerCallback( doPowerOff );
	//WPAD_SetPowerButtonCallback( doPadPowerOff );

	LoadTexture(&console_fnt, GRRLIB_font1);

   _init_buffer();

   return 1;
}

/*! \fn void VideoClose (void) 
    \brief Funcion global que libera y finaliza el sistema de video de la Wii.

*/
void VideoClose (void) 
{

    _clear_buffer();

	WPAD_Shutdown();
        GRRLIB_VSync ();
	GRRLIB_Stop();
        FreeImg(&console_fnt);

    _close_buffer();

}

/*! \fn void FillScreen ( int Updated )
    \brief Funcion global que convierte el buffer lineal del CPC a textura de Wii.

    \param Updated Indica si se debe refrescar la textura de wii, al completar el trabajo del CPC.

*/
void FillScreen ( int Updated )
{

  int h, w;
  char *ra = NULL;


 if(Updated){
  /*
   *  CONVIERTE LINEAL RGB565 A TEXTURA 4x4 RGB565
   */
  long long int *dst = (long long int *) texturemem;
  long long int *src1 = (long long int *) pix;
  long long int *src2 = (long long int *) (pix + (WRES * 2));
  long long int *src3 = (long long int *) (pix + ((WRES * 2) * 2));
  long long int *src4 = (long long int *) (pix + ((WRES * 2) * 3));

  int rowpitch = ((WRES * 2) >> 3) * 3;
  int rowadjust = ( (WRES * 2) % 8 ) * 4;

  //DCFlushRange (pix, WRES * HRES * 4);

  for (h = 0; h < HRES; h += 4)
  {
      for (w = 0; w < (WRES >> 2); w++)
      {
          *dst++ = *src1++; *dst++ = *src2++;
          *dst++ = *src3++; *dst++ = *src4++;
      }

      src1 += rowpitch; src2 += rowpitch;
      src3 += rowpitch; src4 += rowpitch;

      if ( rowadjust )
      {
          ra = (char *)src1; src1 = (long long int *)(ra + rowadjust);
          ra = (char *)src2; src2 = (long long int *)(ra + rowadjust);
          ra = (char *)src3; src3 = (long long int *)(ra + rowadjust);
          ra = (char *)src4; src4 = (long long int *)(ra + rowadjust);
      }
  }

  DCFlushRange(texturemem, texturesize);

 }


  GRRLIB_DrawBuffer(screen_x, -screen_y, WRES, HRES, texturemem, 0, cpc_zoom, cpc_zoom, 255 ); //0.833 //1.667

}

/*! \fn u32 GetXYScreen (f32 x, f32 y)
    \brief Devulve el color del pixel de la posicion indicada en el buffer lineal del CPC.

    \param x Posicion X.
    \param y Posicion Y.

    \return el color del pixel o 0 si el rango dado es erroneo.

*/
u32 GetXYScreen (f32 x, f32 y)
{

  int tx = x / cpc_zoom; //a 384
  int ty = y / cpc_zoom; //a 272

  if(! ((tx > 0) && (ty > 0) && (tx <= WRES) && (ty <= HRES)) )
   return 0;

  u16 * src = (u16 *) pix ;

  src += (ty * WRES);
  src += tx;

  return *src; 
}


/*! \fn void CleanScreen (void)
    \brief Funcion global que limpia la pantalla de la Wii.

*/
void CleanScreen (void)
{

   //GRRLIB_Rectangle(0, 0, rmode->fbWidth, rmode->xfbHeight, 0xB2000000, 1);
   GRRLIB_FillScreen(0XFF000000);

}


/*! \fn void UpdateScreen (void)
    \brief Funcion global que muestra la textura de Wii.

    \todo por el momento no espera el refresco, enlentece mucho el cursor, se podria trabajar mas aqui.

*/
void UpdateScreen (void)
{

   GRRLIB_Render();
  // GRRLIB_VSync (); //enlentece demasiado la gui durante la emulacion, por el momento off.

}


