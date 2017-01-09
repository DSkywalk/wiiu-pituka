/*! \file menu_db.h
 *  \brief Base de datos de los Botones del menu.
 *         Libreria de Wii.
 *
 *  \version 0.1
 *
 *  Wii/GC Menu
 *
 *   Pituka - Nintendo Wii/Gamecube Port
 *  (c) Copyright 2008-2009 David Colmenero (aka D_Skywalk)
 */


#ifndef menudbwii_h
#define menudbwii_h

#define MAXBUTTONS 32
#define MAXSCREEN 6
#define MAXGAMES 24
#define MAXLOADEDGAMES 30
#define NAMEMAX 255

#define ACT_FILE 0
#define ACT_DIR 1
#define ACT_BUTTON 2

/*
 *  BOTONES ESTATICOS DE SECCIONES.
 */

typedef struct
{
  char label[NAMEMAX];
  enum button_type btype;
  int px;
  int py;
  int action;
}sButtonEntry;


typedef struct
{
  int section;
  int nbuttons;
  sButtonEntry buttons[MAXBUTTONS];

}sMenuEntry;

#define ACTION_NULL 0
#define ACTION_OPT_KEY 1
#define ACTION_OPT_GUN 2
#define ACTION_OPT_GREEN 3
#define ACTION_OPT_RELOAD 4
#define ACTION_OPT_DEBUG 5

sMenuEntry menuDB[]  =   {	{ 	0, 3,
					{
						{ "Modo Local", ButtonBig,(640 / 2), 95, 0 },
						{ "Modo Internet", ButtonBig,(640 / 2), 185, 0 },
						{ "Salir", ButtonBig,(640 / 2), 275, 0 },
					}
				},

				{ 	1, 6,	
					{
						{ "Aventura", ButtonNormal,(640 / 2) - (640 / 5), 95, 0 },
						{ "Deportivo", ButtonNormal,(640 / 2) - (640 / 5), 185, 0 },
						{ " Simulación", ButtonNormal,(640 / 2) - (640 / 5), 275, 0 },
						{ "  Acción", ButtonNormal,(640 / 2) + (640 / 5), 95, 0 },
						{ "Estrategia", ButtonNormal,(640 / 2) + (640 / 5), 185, 0 },
						{ "  Otros", ButtonNormal,(640 / 2) + (640 / 5), 275, 0 }
				        }
				},

				{ 	2, 3,	
					{
						{ "Debug Mode", ButtonBig,(640 / 2), 95, ACTION_OPT_DEBUG },
						{ "Monitor Green", ButtonBig,(640 / 2), 185, ACTION_OPT_GREEN },
						{ "Reload RomList", ButtonBig,(640 / 2), 275, ACTION_OPT_RELOAD },
				        }
				},
			};


#endif



