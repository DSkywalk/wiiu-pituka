/*! \file menu.h
 *  \brief Interfaz del menu para la Gui.
 *         Libreria de Wii.
 *
 *  \version 0.2
 *
 *  Wii/GC Sistema de Menu
 *
 *   Pituka - Nintendo Wii/Gamecube Port
 *  (c) Copyright 2008-2009 David Colmenero (aka D_Skywalk)
 */

#ifndef menuwii_h
#define menuwii_h

#define MENU_NULL -100
#define MENU_FILE -1
#define MENU_OPTS 0
#define MENU_GENR 1


void ShowSplash (void);
void ShowMenu (int nMenu);
void MenuInit(void);
void MenuClose(void);
void ShowCommon ( void);
void ShowWait (void);

#endif
