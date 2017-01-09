/*! \file elements.h
 *  \brief Interfaz publico de objetos usables para la Gui.
 *         Libreria de Wii.
 *
 *  \version 0.5
 *
 *  Wii/GC Interfaz
 *
 *   Pituka - Nintendo Wii/Gamecube Port
 *  (c) Copyright 2008-2009 David Colmenero (aka D_Skywalk)
 */

#ifndef elements_h
#define elements_h

typedef struct {
        t_img * Imgs;
        int MaxImgs;
	int UsedImgs;
        int CurrImg;
	float Left;
	float Top;
	float Angle;
	bool Visible;
} t_element;

enum button_type {ButtonFine, ButtonNormal, ButtonBig, ButtonOPTc, ButtonOPTx, ButtonNode};

typedef struct {
        t_element Obj;
        enum button_type btype;
	bool Selected;
	char Caption [50 + 1];
	unsigned int TextWidth;
	unsigned int TextHeight;
	unsigned int TextTop;
	unsigned int TextLeft;

	char Caption_Ex1 [80 + 1];
	char Caption_Ex2 [80 + 1];
        unsigned int TextMod_Ex1;
        unsigned int TextMod_Ex2;

	u32 TextColor;
        int fontsize;
} t_button;

void Element_Init(t_element * element);
void Element_Free(t_element * element);
bool Element_allocImgs(t_element * element, int nImages);
bool Element_LoadImg( t_element * cursor, const unsigned char img [] );
bool Element_SelectImg( t_element * element, int desired );
bool Element_IsInside( t_element * element, float x, float y );
void Element_SetXY( t_element * element, float x, float y );
void Element_Paint( t_element * cursor );

bool Cursor_Init( t_element * cursor );
void Cursor_Free( t_element * cursor );
float Cursor_GetLeftCorrected( t_element * cursor );
float Cursor_GetTopCorrected( t_element * cursor );

bool Button_Init (t_button * button, const char * Text, const char * Text_Ex1, const char * Text_Ex2, enum button_type btype);
void Button_Free (t_button * button);

bool Button_SetCaption(t_button * button, const char * Text, const char * Text_Ex1, const char * Text_Ex2);

void Button_SetXY (t_button * button, const int x, const int y);
void Button_SetSelected (t_button * button, bool selected);
void Button_SetTextColor(t_button * button, u32 tColor);

bool Button_IsInside( t_button * button, float x, float y );
void Button_Paint(t_button * button);

#endif
