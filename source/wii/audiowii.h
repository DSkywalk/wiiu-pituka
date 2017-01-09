/*! \file audiowii.h
 *  \brief Interfaces publicos de la libreria de Sonido.
 *         Libreria de Wii.
 *
 *  \version 0.4
 *
 *  Audio para 48Khz/16bit/Stereo
 *
 *   Pituka - Nintendo Wii/Gamecube Port
 *  (c) Copyright 2008-2009 David Colmenero (aka D_Skywalk)
 */

/**
 Versiones
 - Mirar SVN
 ...
 - 03/10/07 11:55 - Eliminado bug de copiado. 
 - 30/09/07 17:55 - Primera version estable (0.3)
 - 12/09/07 22:29 - Inicio de la libreria
 */


void SoundInit (void);
int SoundSetup (void);
void StopSound ( int val );
void SoundClose(void);




