/*! \file explorer.h
 *  \brief Interfaz publico del sistema de ficheros.
 *         Libreria de Wii.
 *
 *  \version 0.6
 *
 *  Wii/GC Filesystem
 *
 *   Pituka - Nintendo Wii/Gamecube Port
 *  (c) Copyright 2008-2009 David Colmenero (aka D_Skywalk)
 */

#ifndef explorer_h
#define explorer_h

#define FSNAMEMAX 255 + 1

typedef struct {
	char title   [128];
	char genre   [128];
        char company [28];
        char lang    [28];
	char year    [5];
}t_infnode;


typedef struct fslist {
	t_WiiRom gfile;
        t_infnode game;
        bool favorite;
	unsigned char binds[MAX_CPCBUTTONS * 2];

	struct fslist * fnext;
}t_fslist;

void FileList_Init (t_fslist * filenames);
void FileList_rClean(t_fslist * filenames);
void FileList_OrderbyName(t_fslist * filenames);

bool Explorer_LoadFilelist(char * path); //tambien cambia current_path (directorio donde leer R00MS!)

bool Explorer_XMLsave (char * path, t_fslist * filenames);

#endif

