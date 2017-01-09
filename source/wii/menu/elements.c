/*! \file elements.c
 *  \brief Gui-Objects Driver.
 *         Wii Library.
 *
 *  \version 0.5
 *
 *  Wii/GC Interfaz
 *
 *   Pituka - Nintendo Wii/Gamecube Port
 *  (c) Copyright 2008-2009 David Colmenero (aka D_Skywalk)
 */

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include <gccore.h>

#include "../videowii.h"
#include "elements.h"
#include "../grrlib/GRRLIB.h"

#include "../images/gfx_defines.h"

#define DEFAULTFNT_CHRCOLS 16
#define DEFAULTFNT_CHRROWS 16
#define DEFAULTFNT_CHRSPACE -12

/*! \fn void Element_Init (t_element * element)
    \brief Limpia la estructura element.

    \param element estructura a ser limpiada, DEBE ESTAR LIBERADA.
    \note La funcion no comprueba si el buffer esta liberado o no.
*/
void Element_Init (t_element * element)
{
	element->Left = 0;
	element->Top = 0;
	element->Angle = 0.0;
        element->MaxImgs = 0;
        element->UsedImgs = 0;
        element->CurrImg = -1;
	element->Imgs = NULL;

}

/*! \fn bool Element_allocImgs(t_element * element, int nImages)
    \brief Inicializa la estructura element y reserva el espacio para cargar imagenes.

    \param element estructura a inicializar, DEBE ESTAR LIBERADA.
    \return devuelve true si todo termino correctamente.
*/
bool Element_allocImgs(t_element * element, int nImages)
{
     if(element->Imgs != NULL || nImages < 1)
        return false;

     element->Imgs = malloc( sizeof( t_img ) * nImages );
     element->MaxImgs = nImages;
     element->UsedImgs = 0;
     element->CurrImg = -1;

     element->Left = 0;
     element->Top = 0;
     element->Angle = 0.0;

     return true;
}

/*! \fn bool Element_FreeImg( t_element * element )
    \brief Libera la ultima imagen cargada.

    \param element estructura a usar, DEBE HABER IMAGENES CARGADAS.
    \return true si se libero correctamente una imagen.
*/
bool Element_FreeImg( t_element * element )
{

    if((element->UsedImgs-1) <  0)
	return false;

    element->UsedImgs--;

    FreeImg(&element->Imgs[element->UsedImgs]);

    return true;
}

/*! \fn bool Element_FreeImg( t_element * element )
    \brief Libera TODAS las imagenes cargadas y el espacio reservado para ellas, para finalmente inicializar la estructura element.

    \param element estructura a liberar, DEBE ESTAR INICIALIZADA.
*/
void Element_Free(t_element * element)
{

    if(element->Imgs == NULL)
	return;

     //Liberamos las texturas
    while(element->UsedImgs)
    {
       	Element_FreeImg(element);
    }

    free(element->Imgs);
    Element_Init(element);

}

/*! \fn bool Element_LoadImg( t_element * element, const unsigned char img [] )
    \brief Carga una imagen a la estructura element.

    \param element estructura a usar, DEBE ESTAR INICIALIZADA.
    \param img buffer PNG de la imagen a cargar.

    \return true si todo termino correctamente.
*/
bool Element_LoadImg( t_element * element, const unsigned char img [] )
{

    if(element->UsedImgs < element->MaxImgs){

  	if( LoadTexture(&element->Imgs[element->UsedImgs], img) )
  	{
		if(!element->UsedImgs)
			element->CurrImg = 0; //seleccionamos la primera por defecto al cargar por primera vez
                element->UsedImgs++;
		return true;
	}

    }
    return false;

}

/*! \fn bool Element_LoadFnt( t_element * element, const unsigned char fnt [], const char * Text, int TextSize, u32 TextColor )
    \brief Renderizar un texto como imagen en la estructura element.

    \param element estructura a usar, DEBE ESTAR INICIALIZADA.
    \param fnt buffer TTF de la fuente a renderizar (no usado aun).
    \param Text Cadena a renderizar.
    \param TextSize Tamaño de la cadena.
    \param TextColor Color requerido para el renderizado.

    \return true si todo termino correctamente.
*/
bool Element_LoadFnt( t_element * element, const unsigned char fnt [], const char * Text, int TextSize, u32 TextColor )
{

    if(element->UsedImgs < element->MaxImgs){

  	if( LoadFont(&element->Imgs[element->UsedImgs], fnt, Text, TextSize, TextColor) )
  	{
		if(!element->UsedImgs)
			element->CurrImg = 0; //seleccionamos la primera por defecto al cargar por primera vez
                element->UsedImgs++;
		return true;
	}

    }

    return false;

}

/*! \fn bool Element_SelectImg( t_element * element, int desired )
    \brief Selecciona una de las imagenes cargadas de la estructura element.

    \param element estructura a usar, DEBE ESTAR INICIALIZADA.
    \param desired numero de la imagen a seleccionar.

    \return true si se pudo seleccionar correctamente.
*/
bool Element_SelectImg( t_element * element, int desired )
{
	if( desired < 1 || desired > element->UsedImgs ) 		// sino esta seleccionada o esta mal seleccionada,
	   return false;

	element->CurrImg = desired - 1;

	return true;
}


/*! \fn bool Element_IsInside( t_element * element, float x, float y )
    \brief Comprueba si el elemento (la imagen) se encuentra dentro de las coordenadas dadas.

    \param element estructura a comprobar, DEBE ESTAR INICIALIZADA.
    \param x posicion del pixel en X.
    \param y posicion del pixel en Y.

    \return true si la posicion esta ocupada por la imagen.
*/
bool Element_IsInside( t_element * element, float x, float y )
{

	if( element->UsedImgs < 1 ) // sino hay imagenes devuelve false siempre
		return false;

	if (x > element->Left && x < (element->Left + element->Imgs[element->CurrImg].Width) && y > element->Top && y < (element->Top + element->Imgs[element->CurrImg].Height))
		return true;
	return false;
}


/*! \fn void Element_SetXY( t_element * element, float x, float y )
    \brief Posiciona la estructura element en las coordenadas dadas.

    \param element estructura a posicionar, DEBE ESTAR INICIALIZADA.
    \param x posicion en X.
    \param y posicion en Y.

*/
void Element_SetXY( t_element * element, float x, float y )
{
	element->Left = x;
	element->Top = y;
}

/*! \fn void Element_Paint( t_element * element )
    \brief Dibuja la estructura element.

    \param element estructura a posicionar, DEBE ESTAR INICIALIZADA.

*/
void Element_Paint( t_element * element )
{

	if( element->UsedImgs < 1 ) // sino hay imagenes termina
		return;

	DrawTexture(&element->Imgs[element->CurrImg], element->Left, element->Top, element->Angle, 1, 1, 255);
}



/**
 *  CURSOR CODE
 */


#define IMGCURSOR 3

#define IMGCURSOR_OFF 0
#define IMGCURSOR_ON 1
#define IMGCURSOR_SHOOT 2

/*! \fn bool Cursor_Init( t_element * cursor )
    \brief Inicializa el cursor.

    \param element estructura a ser inicializada como cursor, DEBE ESTAR LIBERADA.
    \note La funcion no comprueba si el buffer esta liberado o no.
*/
bool Cursor_Init( t_element * cursor )
{
	Element_Init(cursor);
	if(!Element_allocImgs(cursor, IMGCURSOR))
		return false;
	if(!Element_LoadImg( cursor, cursor_open_png ))
		return false;
	if(!Element_LoadImg( cursor, cursor_point_png ))
		return false;
	if(!Element_LoadImg( cursor, cursor_gun_png ))
		return false;
	Element_SelectImg( cursor, 1 ); //por defecto abierta

	return true;

}

/*! \fn void Cursor_Free( t_element * cursor )
    \brief Libera toda la memoria usada por el cursor.

    \param element estructura a ser liberada, DEBE ESTAR INICIALIZADA.
*/
void Cursor_Free( t_element * cursor )
{
	Element_Free(cursor);
        
}


/*! \fn float Cursor_GetLeftCorrected( t_element * cursor )
    \brief Devuelve la posicion en X del cursor dependiendo de su angulo actual.

    \param element estructura a ser usada, DEBE ESTAR INICIALIZADA.
    \return la posicion corregida en X.

    \note no funciona correctamente aun... need debug!!!
*/
float Cursor_GetLeftCorrected( t_element * cursor )
{
	return cursor->Left*cos(cursor->Angle) - cursor->Top*sin(cursor->Angle);
}

/*! \fn float Cursor_GetTopCorrected( t_element * cursor )
    \brief Devuelve la posicion en Y del cursor dependiendo de su angulo actual.

    \param element estructura a ser usada, DEBE ESTAR INICIALIZADA.
    \return la posicion corregida en Y.

    \note no funciona correctamente aun... need debug!!!
*/
float Cursor_GetTopCorrected( t_element * cursor )
{
	return cursor->Left*sin(cursor->Angle) + cursor->Top*cos(cursor->Angle);
}



/**
 *  OBJECTS/BUTTONS CODE
 */

int fontsize = 0;

#define IMGOBJECTS 3

#define IMGOBJECT_OFF 0
#define IMGOBJECT_ON 1
#define IMGOBJECT_FONT 2

#define FONTCOLOUR 0xFFFFFF
#define DEFAULT_FONTSIZE 24

#define LITTLE_FONTSIZE_MOD 10
#define LITTLE_FONTPOSI_MOD 9


/*! \fn bool Button_SetCaption(t_button * button, const char * Text, const char * Text_Ex1, const char * Text_Ex2)
    \brief Establece los textos y los posiciona dentro del boton dado.

    \param button boton a ser usado, DEBE ESTAR INICIALIZADO Y CON IMAGENES CARGADAS.
    \param Text Texto base a ser renderizado en el boton.
    \param Text_Ex1 Texto extra y opcional a ser renderizado en el boton (usado en nodes para Year).
    \param Text_Ex2 Otro texto extra y opcional a ser renderizado en el boton (usado en nodes para Company, etc...).

    \return true si las operaciones terminaron correctamente.

*/
bool Button_SetCaption(t_button * button, const char * Text, const char * Text_Ex1, const char * Text_Ex2)
{
   int count = 0;
   
   if(strlen(button->Caption) > 0 )
	count++;

   if(button->btype == ButtonNode)
   {
        if(strlen(button->Caption_Ex1) > 0 )
	    count++;
        if(strlen(button->Caption_Ex2) > 0 )
	    count++;
   }

   strncpy(button->Caption,  Text /*tmp*/, 50);

   if(button->btype == ButtonNode)
   {
	strncpy(button->Caption_Ex1,  Text_Ex1 , 80);
	strncpy(button->Caption_Ex2,  Text_Ex2 , 80);
   }


   while( count-- ) //liberamos las necesarias
        Element_FreeImg( &button->Obj );


   if(strlen(Text) > 0)
   {
	if(!Element_LoadFnt( &button->Obj, NULL, Text, button->fontsize, FONTCOLOUR ))
		return false;
   }

   if(button->btype == ButtonNode)
   {
	if(strlen(Text_Ex1) > 0)
   	{
		if(!Element_LoadFnt( &button->Obj, NULL, Text_Ex1, button->fontsize, FONTCOLOUR ))
			return false;
   	}


   	if(strlen(Text_Ex2) > 0)
   	{
		if(!Element_LoadFnt( &button->Obj, NULL, Text_Ex2, button->fontsize, FONTCOLOUR ))
			return false;
   	}
  }

  return true;

}

/*! \fn void Button_SetXY (t_button * button, const int x, const int y)
    \brief posiciona el boton y recoloca sus textos segun la nueva posicion dada.

    \param button boton a ser usado, DEBE ESTAR INICIALIZADO Y CON IMAGENES CARGADAS.
    \param x nueva posicion en X.
    \param y nueva posicion en Y.

*/
void Button_SetXY (t_button * button, const int x, const int y)
{
   int tWidth = 0;
   int tWidth_Ex = 0;

   button->Obj.Top = y;
   button->Obj.Left = x - (button->Obj.Imgs[IMGOBJECT_OFF].Width / 2);

   if(strlen(button->Caption) > 0)
   {
   	tWidth = button->TextWidth * strlen(button->Caption);
   	tWidth_Ex = button->TextWidth * strlen(button->Caption_Ex2) * 2;


	switch(button->btype) //TODO: ACTUALIZA CAPTION ?
	{
	    case ButtonNode:
		button->TextTop  = button->Obj.Top + ((button->Obj.Imgs[IMGOBJECT_OFF].Height / 2) - button->TextHeight) - LITTLE_FONTPOSI_MOD;
		button->TextLeft = button->Obj.Left + 77;

		button->TextMod_Ex1 = button->TextTop + (button->TextHeight * 2);
		button->TextMod_Ex2 = (button->Obj.Left + (button->Obj.Imgs[IMGOBJECT_OFF].Width - tWidth_Ex)) - LITTLE_FONTPOSI_MOD;

                break;

	    case ButtonFine:
		button->TextTop  = button->Obj.Top + ((button->Obj.Imgs[IMGOBJECT_OFF].Height / 2) - button->TextHeight) - LITTLE_FONTPOSI_MOD;
		button->TextLeft = button->Obj.Left + 80;
	
		break;

	    default:
		button->TextTop  = button->Obj.Top + ((button->Obj.Imgs[IMGOBJECT_OFF].Height / 2) - button->TextHeight);
		button->TextLeft = button->Obj.Left + ((button->Obj.Imgs[IMGOBJECT_OFF].Width / 2) - (tWidth / 2));
	}
   }

}

/*! \fn bool Button_Init (t_button * button, const char * Text, const char * Text_Ex1, const char * Text_Ex2, enum button_type btype)
    \brief Inicia, reserva y compone el boton dado y sus textos.

    \param button boton a ser usado, DEBE ESTAR LIBERADO.
    \param Text Texto base a ser renderizado en el boton.
    \param Text_Ex1 Texto extra y opcional a ser renderizado en el boton (usado en nodes para Year).
    \param Text_Ex2 Otro texto extra y opcional a ser renderizado en el boton (usado en nodes para Company, etc...).
    \param btype tipo de boton a ser Inicializado.

    \return true si las operaciones terminaron correctamente.

*/
bool Button_Init (t_button * button, const char * Text, const char * Text_Ex1, const char * Text_Ex2, enum button_type btype)
{
	int font_modifier = 0;
        int alloc = 3;

	button->btype = btype;

	Element_Init(&button->Obj);

        if(button->btype == ButtonNode)
          alloc = 5;

	if(!Element_allocImgs(&button->Obj, alloc))
		return false;


	switch(button->btype)
	{
		case ButtonNode:
		case ButtonFine:
			if(!Element_LoadImg( &button->Obj, button_romlist_normal_png )) // primero off
				return false;
			if(!Element_LoadImg( &button->Obj, button_romlist_over_png )) // on
				return false;

			font_modifier = LITTLE_FONTSIZE_MOD;
			break;

		case ButtonBig:
			if(!Element_LoadImg( &button->Obj, button_long_normal_png )) // primero off
				return false;
			if(!Element_LoadImg( &button->Obj, button_long_over_png )) // on
				return false;
			break;

		case ButtonOPTc:
			if(!Element_LoadImg( &button->Obj, button_gui_config_png ))
				return false;
			break;

		case ButtonOPTx:
			if(!Element_LoadImg( &button->Obj, button_gui_exit_png ))
				return false;
			break;

		default: //cargamos normal

			if(!Element_LoadImg( &button->Obj, button_base_normal_png )) // primero off
				return false;
			if(!Element_LoadImg( &button->Obj, button_base_over_png )) // on
				return false;
	}

	button->fontsize = DEFAULT_FONTSIZE - font_modifier;

        button->Caption[0] = 0;
        button->Caption_Ex1[0] = 0;
        button->Caption_Ex2[0] = 0;

        Button_SetCaption(button, Text, Text_Ex1, Text_Ex2);
        button->TextColor = FONTCOLOUR;
        button->TextWidth = ( button->fontsize + 2 ) / 2; //fix font
        button->TextHeight = ( button->fontsize / 4 ) + (button->fontsize / 2); //fix font

	button->Selected = false;
	Element_SelectImg( &button->Obj, 1 ); //por defecto off (no es la 0)

	return true;
}


/*! \fn void Button_Free (t_button * button)
    \brief Libera e incializa complementamente el boton dado.

    \param button boton a ser liberado, DEBE ESTAR INICIALIZADO Y CON IMAGENES CARGADAS.

*/
void Button_Free (t_button * button)
{
	Element_Free(&button->Obj);
}

/*! \fn void Button_SetSelected (t_button * button, bool selected) 
    \brief Selecciona la imagen de SELECCIONADO del boton dado.

    \param button boton a ser usado, DEBE ESTAR INICIALIZADO Y CON IMAGENES CARGADAS.
    \param selected true seleccionado y false imagen de "sin seleccion".

*/
void Button_SetSelected (t_button * button, bool selected) 
{
        Element_SelectImg( &button->Obj, selected + 1 );
	button->Selected = selected;
}

/*! \fn void Button_SetSelected (t_button * button, bool selected) 
    \brief Guarda el color deseado para la fuente del boton.

    \param button boton a ser usado, DEBE ESTAR INICIALIZADO.
    \param tColor Color a usar al renderizar el texto.

*/
void Button_SetTextColor(t_button * button, u32 tColor)
{
	button->TextColor = tColor;
}


/*! \fn void Button_Paint(t_button * button)
    \brief Pinta el boton en pantalla.

    \param button boton a ser dibujado, DEBE ESTAR INICIALIZADO Y CON IMAGENES CARGADAS.
    \todo Optimizar intentando no realizar un strlen cada vez que pintamos las imagenes de los textos en pantalla.

*/
void Button_Paint(t_button * button)
{
   int nTexture = 0;

   Element_Paint( &button->Obj );

   if(strlen(button->Caption) > 0) // no pintar sino hay texto?
   {
        DrawTexture(&button->Obj.Imgs[IMGOBJECT_FONT], button->TextLeft, button->TextTop, 0, 1, 1, 255); 
	nTexture++; //preparamos siguiente texto (IMG RENDER FONT)
   }

   if(button->btype == ButtonNode)
   {
   	if(strlen(button->Caption_Ex1) > 0) 
   	{
        	DrawTexture(&button->Obj.Imgs[IMGOBJECT_FONT + nTexture], button->TextLeft+10, button->TextMod_Ex1, 0, 1, 1, 255); 
		nTexture++; //siguiente texto
   	}

   	if(strlen(button->Caption_Ex2) > 0)
  	{
        	DrawTexture(&button->Obj.Imgs[IMGOBJECT_FONT + nTexture], button->TextMod_Ex2, button->TextMod_Ex1, 0, 1, 1, 255); 
   	}
   }

}

/*! \fn bool Button_IsInside( t_button * button, float x, float y )
    \brief Comprueba si el boton se encuentra dentro de las coordenadas dadas.

    \param button boton a comprobar, DEBE ESTAR INICIALIZADO Y CON IMAGENES CARGADAS.
    \param x posicion del pixel en X.
    \param y posicion del pixel en Y.

    \return true si la posicion esta ocupado por el boton.
*/
bool Button_IsInside( t_button * button, float x, float y )
{
	return Element_IsInside(&button->Obj, x, y);
}

