/*! \file wiixml.h
 *  \brief Interfaz publico de sistema XML.
 *         Libreria de Wii.
 *
 *  \version 0.4
 *
 *  Wii/GC Interfaz XML.
 *
 *   Pituka - Nintendo Wii/Gamecube Port
 *  (c) Copyright 2008-2009 David Colmenero (aka D_Skywalk)
 */

#ifndef wiixml_h
#define wiixml_h

#define DEFAULT_XMLVER 0x101
#define DEFAULT_XMLGAMELIST 0x00
#define DEFAULT_WIIBUILD  92

bool XML_savePublic(char * filename);
bool XML_loadPublic(char * filename);

int XML_checkUpdates(char * xmldata);

#endif








