/*! \file keybwii.c
 *  \brief Keyboard Driver.
 *         Wii Library.
 *
 *  \version 0.6
 *
 * Keyboard USB / KOS - Simple Driver
 *
 *   Pituka - Nintendo Wii/Gamecube Port
 *  (c) Copyright 2008-2009 David Colmenero (aka D_Skywalk)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <math.h>
#include <gccore.h>
#include <wiiuse/wpad.h>
#include <wiikeyboard/keyboard.h>

#include "../global.h"
#include "videowii.h"
#include "motewii.h"
#include "menu/elements.h"

#include "keybwii.h"

extern t_element cursor;
extern t_img keyb;
extern t_element keyb_help;

/* CPC */
extern Bitu8 keyboard_matrix[16];
extern Bitu8 bit_values[8];
extern Bitu8 keyboard_translation[320];

/*  WII VIDEO  */
extern float screen_x;
extern float screen_y;
extern float cpc_zoom;

extern WiituKa_Status WiiStatus;


                        //X INIT
int kc_pos [6][2] = { {   0,   0 }, //null zone
		      { -12, 568 },
                      {  10, 599 },
                      {  19, 566 },
                      {  34, 539 },
                      {  34, 577 }
                    };       //X END

unsigned char keyb_array [6][19] = 
		    {
		      { 0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff },
		      { 0x82,0x80,0x81,0x71,0x70,0x61,0x60,0x51,0x50,0x41,0x40,0x31,0x30,0x97,0x97,0x12,0x13,0x03,0xff },
		      { 0x84,0x83,0x73,0x72,0x62,0x63,0x53,0x52,0x43,0x42,0x33,0x32,0x21,0x22,0xff,0x24,0x14,0x04,0xff },
		      { 0x86,0x85,0x74,0x75,0x65,0x64,0x54,0x55,0x45,0x44,0x35,0x23,0x22,0x22,0xff,0x15,0x16,0x05,0xff },
		      { 0x25,0x87,0x77,0x76,0x67,0x66,0x56,0x46,0x47,0x37,0x36,0x25,0x25,0x25,0xff,0x17,0x00,0x07,0xff },
		      { 0x27,0x11,0x11,0x57,0x57,0x57,0x57,0x57,0x57,0x57,0x06,0x06,0x06,0x06,0xff,0x10,0x02,0x01,0xff }
                    };


/*! \fn int KeyboardInit (void)
    \brief Inicializa el teclado USB.

    \return el valor devuelto por la inicializacion del driver USB.
*/
int KeyboardInit (void)
{
  s32 ret = 0;

  ret = KEYBOARD_Init(NULL);
  
  return ret;

}

/*! \fn void poll_keyboard (void)
    \brief Transforma una tecla pulsada del teclado USB a tecla emulada en CPC usando keyboard_matrix.

*/
void poll_keyboard (void)
{

        Bitu8 cpc_key;
	keyboard_event event;

		if (! KEYBOARD_GetEvent(&event))
			return;
		switch (event.type)
		{

			case KEYBOARD_CONNECTED:
				WiiStatus.Dev_Keyb = 1;
			break;
			case KEYBOARD_DISCONNECTED:
				WiiStatus.Dev_Keyb = 0;
			break;

			case KEYBOARD_PRESSED: 

                                cpc_key = keyboard_translation[event.keycode]; // translate the PC key to a CPC key
                                if (cpc_key != 0xff) {
                                        keyboard_matrix[cpc_key >> 4] &= ~bit_values[cpc_key & 7]; // key is being held down
                                }
                                   
			break;
			case KEYBOARD_RELEASED:
                                cpc_key = keyboard_translation[event.keycode]; // translate the PC key to a CPC key
                                if (cpc_key != 0xff) {
                                       keyboard_matrix[cpc_key >> 4] |= bit_values[cpc_key & 7]; // key has been released
                                }
			break;
		}

}

/*! \fn void KeyboardShow (void)
    \brief Muestra el teclado en pantalla (KOS) y la ayuda de bindeo.

*/
void KeyboardShow (void)
{
  Element_SetXY(&keyb_help, 24, 304-screen_y);

  Element_Paint( &keyb_help );
  DrawTexture(&keyb, 0, 342-screen_y, 0, 0.987, 0.987, 255); //294
}


/*
 [35][70]...
 [70]
 ...
*/

#define KEYB_Y 248
#define KEY_GFX_WIDTH 35.6

/*! \fn unsigned char KeyboardCheck (int state)
    \brief Comprueba la tecla pulsada en el teclado en pantalla (KOS) y la envia.

    \param state si vale KEY_GET devuelve el valor de la tecla para su bindeo a un boton
                 si vale KEY_DOWN/KEY_UP se envia a la matriz de teclado del CPC.
*/
unsigned char KeyboardCheck (int state)
{
   int kc_px = cursor.Left;
   int kc_py = cursor.Top;

   unsigned char cpc_key = 0xff;
   static unsigned char key_send = 0xff;
   static unsigned char key_mod = 0xff;
   
   if(!state){
        keyboard_matrix[key_send >> 4] |= bit_values[key_send & 7]; // key has been released
	key_send = 0xff; //se prepara para el futuro
	return 0xff;
   }

   kc_py = (int) ((double) ( kc_py - KEYB_Y) / KEY_GFX_WIDTH); 

   if(kc_py > 0 && kc_py < 6) //cursor is on keyboard
   {
       if(kc_px > 510 )
          kc_px = (int) ((kc_px - 510) / KEY_GFX_WIDTH) + 15;
       else
       	  kc_px = (int) ((double) ( kc_px - (kc_pos[kc_py][0])) / KEY_GFX_WIDTH);
   }else{
       kc_px = -10;
       return 0xff;
   }

   if ( kc_px > -10 )
   {
       if ( kc_px < 0)
           kc_px = 0;
       if ( kc_py > 15)
           kc_py = 15;
       
       if(state == KEY_GET)
	   return keyb_array[kc_py][kc_px];
       else
	   cpc_key = keyb_array[kc_py][kc_px];
       
   }

   if ((key_send != cpc_key) && (key_send == 0xff)){ //buffer vacio
       if(cpc_key == 0x25 || cpc_key == 0x27){
	   if(key_mod != 0xff){
	    	keyboard_matrix[key_mod >> 4] |= bit_values[key_mod & 7];
	    	key_mod = 0xff;
	   }else{
	   	key_mod = cpc_key;
	   	keyboard_matrix[key_mod >> 4] &= ~bit_values[key_mod & 7];
	   }
       }else{
       
       	   key_send = cpc_key;
           keyboard_matrix[cpc_key >> 4] &= ~bit_values[cpc_key & 7]; // key is being held down
       }
   }

   return cpc_key;
   
}

