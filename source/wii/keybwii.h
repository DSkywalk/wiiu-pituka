/*! \file keybwii.h
 *  \brief Interfaz publico del manejo del teclado.
 *         Libreria de Wii.
 *
 *  \version 0.6
 *
 *  Wii/GC Interfaz para teclado en pantalla y USB.
 *
 *   Pituka - Nintendo Wii/Gamecube Port
 *  (c) Copyright 2008-2009 David Colmenero (aka D_Skywalk)
 */


#ifndef keybwii_h
#define keybwii_h




void poll_keyboard (void);

int KeyboardInit (void);
void KeyboardShow (void);

#define KEY_DOWN 1
#define KEY_UP 0
#define KEY_GET -1

unsigned char KeyboardCheck (int state);


#endif


