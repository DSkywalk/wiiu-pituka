/*! \file languages.h
 *  \brief Interfaz de Traducciones para la Gui.
 *         Libreria de Wii.
 *
 *  \version 0.0
 *
 *  Wii/GC Traduccion
 *
 *   Pituka - Nintendo Wii/Gamecube Port
 *  (c) Copyright 2008-2009 David Colmenero (aka D_Skywalk)
 */


#ifndef language_h
#define language_h


#define MAX_LANGS 2

#define LANG_EN 0
#define LANG_ES 1

//cadenas

//en
#define LNG_EN_DOWNLOAD "Downloading %s."
#define LNG_EN_KNOWCPC "I know the CPC Systems"

//es
#define LNG_ES_DOWNLOAD "Bajando fichero %s"
#define LNG_ES_KNOWCPC "Conozco el Ordenador CPC"


char locales = { LANG_EN {
                   LNG_EN_DOWNLOAD, LNG_EN_KNOWCPC 
                 },
                 LANG_ES {
                   LNG_EN_DOWNLOAD, LNG_EN_KNOWCPC 
                 }
               };


#endif
