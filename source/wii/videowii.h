/*! \file videowii.h
 *  \brief Interfaces publicos de la libreria de Video.
 *         Libreria de Wii.
 *
 *  \version 0.8
 *
 *  Video para 640x480 (16bits)
 *
 *   Pituka - Nintendo Wii/Gamecube Port
 *  (c) Copyright 2008-2009 David Colmenero (aka D_Skywalk)
 */

/**
 Versiones
 - Mirar SVN
 ...
 - 12/09/07 23:12 - Inicio de la libreria
 */


#ifndef videowii_h
#define videowii_h


/** \brief Estructura de Imagenes
 *  Esta es una estructura general que contiene las imagenes del emulador y usarlas para el render en Wii.
 * \note Solo soporta PNG.
 */
typedef struct{
  GXTexObj texObj;  /**< Textura GX, no usando aun, podria usarse para ganar optimizacion */
  void * buffer; /**< El buffer que contiene la Textura usable por Wii */

  int Width; /**< ancho de la imagen */
  int Height; /**< alto de la imagen */
} t_img;


int VideoInit (void);
void VideoClose (void);

void CleanScreen (void);

void UpdateScreen (void);
 
void FillScreen ( int Updated );
unsigned int GetXYScreen (f32 x, f32 y);

/*
  VIDEO UTILS
*/
void InitImg (t_img * img);
void FreeImg (t_img * img);

bool LoadFont (t_img * img, const unsigned char my_fnt[], const char * Text, int TextSize, u32 TextColor);
bool LoadTexture (t_img * img, const unsigned char my_png[]);
void DrawTexture (t_img * img, f32 xpos, f32 ypos, float degrees, float scaleX, f32 scaleY, u8 alpha );
void PrintW (f32 xpos, f32 ypos, const char *text,...);

#endif
