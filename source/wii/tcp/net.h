/*! \file net.h
 *  \brief Interfaz publico red.
 *         Libreria de Wii.
 *
 *  \version 0.2
 *
 *  Sistema de Red para Wii
 *
 *   Pituka - Nintendo Wii/Gamecube Port
 *  (c) Copyright 2008-2009 David Colmenero (aka D_Skywalk)
 */

#ifndef netwii_h
#define netwii_h


// -----------------------------------------------------------
// ENUMS
// -----------------------------------------------------------

enum
{
    	TCP_INIT	=	0,
	TCP_REQUEST1	=	1,
	TCP_REQUEST2	=	2,
	TCP_CANCEL	=	3,
	TCP_ERROR	=	4,
	TCP_RETRY	=	5,
	TCP_END		=	6
};

int net_get_state(void);

int net_get_buffersize(void);
void * net_get_filebuffer(void);
char * net_get_charbuffer(void);

int net_init_thread(void);
int net_start_thread(char * getUrl, int mode, int retry);
int net_stop_thread(void);

#endif
