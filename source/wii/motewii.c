/*! \file motewii.c
 *  \brief Control Driver.
 *         Wii Library.
 *
 *  \version 0.9
 *
 *  Wii/GC Control driver
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

#include "../global.h"

#include "videowii.h"
#include "keybwii.h"
#include "menu/elements.h"
#include "menu/explorer.h"
#include "wpad_leds/wpad_leds.h"

#include "motewii.h"

extern GXRModeObj *rmode;

extern t_element cursor;
extern t_element keyb_help;
extern t_CPC CPC;

#define CORRECCION_MOTE_X     -180  //para que salga de la pantalla
#define CORRECCION_MOTE_Y     -280  //para que salga de la pantalla
#define JOY_ZONECAL             64
#define PI                      3.14159265f



extern Wii_gun gunstick;
extern t_fslist * current_game;

extern WiituKa_Status WiiStatus;
extern xmlWiiCFG WiitukaXML;

/*  WII VIDEO  */
extern float screen_x;
extern float screen_y;
extern float cpc_zoom;

unsigned int PusshedWPAD = 0;
bool WpadOnScreen = false;
extern unsigned char kc_cpcKey;
unsigned int kbs_ticks = 0;

/* CPC */
extern Bitu8 keyboard_matrix[16];
extern Bitu8 bit_values[8];
extern unsigned char spool;
extern char spool_cad[256];


/* CONTROL GLOBALS */
unsigned char kbind = 0xff;
inline bool __gunstick_checkHit (void);
inline void _wiimoteBind (t_mote * wiipad);

u8 current_buttons = WIIMOTE_ONLY_BUTTONS;

const unsigned char buttons_def[MAX_CPCBUTTONS * 2] = {
    0x92, 0x93, 0x91, 0x90, 0x95, 0x94, 0xff, 0xff, 0xff, 0xff, 
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0x10, 0x01, 0x02, 0x00, 0x57, 0x85, 0xff, 0xff, 0xff, 0xff, 
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};

const unsigned int classic_binds[8][2] = 
{
    { WPAD_CLASSIC_BUTTON_A, WPAD_BUTTON_2},
    { WPAD_CLASSIC_BUTTON_B, WPAD_BUTTON_1},  
    { WPAD_CLASSIC_BUTTON_DOWN, WPAD_BUTTON_LEFT}, 
    { WPAD_CLASSIC_BUTTON_UP, WPAD_BUTTON_RIGHT}, 
    { WPAD_CLASSIC_BUTTON_RIGHT, WPAD_BUTTON_DOWN}, 
    { WPAD_CLASSIC_BUTTON_LEFT, WPAD_BUTTON_UP},
    { WPAD_CLASSIC_BUTTON_PLUS, WPAD_BUTTON_PLUS},
    { WPAD_CLASSIC_BUTTON_MINUS, WPAD_BUTTON_MINUS},
};

const unsigned int gc_binds[8][2] = 
{
    { (PAD_BUTTON_B << 8), WPAD_BUTTON_2},
    { (PAD_BUTTON_X << 8), WPAD_BUTTON_1},  
    { (PAD_BUTTON_DOWN << 8), WPAD_BUTTON_LEFT}, 
    { (PAD_BUTTON_UP << 8), WPAD_BUTTON_RIGHT}, 
    { (PAD_BUTTON_RIGHT << 8), WPAD_BUTTON_DOWN}, 
    { (PAD_BUTTON_LEFT << 8), WPAD_BUTTON_UP},
    { (PAD_BUTTON_START << 8), WPAD_BUTTON_PLUS},
    { (PAD_TRIGGER_Z << 8), WPAD_BUTTON_MINUS},
};

t_wiipads controls ={ 
    //UP es LEFT por que el mando esta horizontal
    { {    
        { WPAD_BUTTON_UP                                , 0x92 },
        { WPAD_BUTTON_DOWN                              , 0x93 },
        { WPAD_BUTTON_LEFT                              , 0x91 },
        { WPAD_BUTTON_RIGHT                             , 0x90 },

        { WPAD_BUTTON_1                                 , 0x95 },
        { WPAD_BUTTON_2                                 , 0x94 },

        { WPAD_BUTTON_PLUS                              , 0xff },
        { WPAD_BUTTON_MINUS                             , 0xff },

        { WPAD_BUTTON_A                                 , 0xff },
        { WPAD_BUTTON_B                                 , 0xff },

        //classic support
        { WPAD_CLASSIC_BUTTON_X                         , 0xff },
        { WPAD_CLASSIC_BUTTON_Y                         , 0xff },
        { WPAD_CLASSIC_BUTTON_ZR                        , 0xff },
        { WPAD_CLASSIC_BUTTON_ZL                        , 0xff },
        { WPAD_CLASSIC_BUTTON_FULL_R                    , 0xff },
        { WPAD_CLASSIC_BUTTON_FULL_L                    , 0xff },

        { WPAD_EXTRA_JOY_UP                             , 0xff },
        { WPAD_EXTRA_JOY_DOWN                           , 0xff },
        { WPAD_EXTRA_JOY_LEFT                           , 0xff },
        { WPAD_EXTRA_JOY_RIGHT                          , 0xff },

        //gc support
        {(PAD_BUTTON_A << 8)                            , 0xff },
        {(PAD_BUTTON_Y << 8)                            , 0xff },
        {(PAD_TRIGGER_L << 8)                           , 0xff },
        {(PAD_TRIGGER_R << 8)                           , 0xff },

      }, 0, 0, 0, 0, NULL },

    { {
        { WPAD_BUTTON_UP                                , 0x10 },
        { WPAD_BUTTON_DOWN                              , 0x01 },
        { WPAD_BUTTON_LEFT                              , 0x02 },
        { WPAD_BUTTON_RIGHT                             , 0x00 },

        { WPAD_BUTTON_1                                 , 0x57 },
        { WPAD_BUTTON_2                                 , 0x85 },

        { WPAD_BUTTON_PLUS                              , 0xff },
        { WPAD_BUTTON_MINUS                             , 0xff },

        { WPAD_BUTTON_A                                 , 0xff },
        { WPAD_BUTTON_B                                 , 0xff },

        //classic support
        { WPAD_CLASSIC_BUTTON_X                         , 0xff },
        { WPAD_CLASSIC_BUTTON_Y                         , 0xff },
        { WPAD_CLASSIC_BUTTON_ZR                        , 0xff },
        { WPAD_CLASSIC_BUTTON_ZL                        , 0xff },
        { WPAD_CLASSIC_BUTTON_FULL_R                    , 0xff },
        { WPAD_CLASSIC_BUTTON_FULL_L                    , 0xff },
        { WPAD_EXTRA_JOY_UP                             , 0xff },
        { WPAD_EXTRA_JOY_DOWN                           , 0xff },
        { WPAD_EXTRA_JOY_LEFT                           , 0xff },
        { WPAD_EXTRA_JOY_RIGHT                          , 0xff },

        //gc support
        {(PAD_BUTTON_A << 8)                            , 0xff },
        {(PAD_BUTTON_Y << 8)                            , 0xff },
        {(PAD_TRIGGER_L << 8)                           , 0xff },
        {(PAD_TRIGGER_R << 8)                           , 0xff },
      }, 0, 0, 0, 0, NULL },
};

/*! \fn void __poll_wiitojoy (t_mote * wiipad)
    \brief Lee la posicion del control y la envia a la matriz de teclado del CPC.

    \param wiipad control a comprobar.
*/
void __poll_wiitojoy (t_mote * wiipad)
{
    Bitu8 cpc_key;
    register int n;
     
    if(wiipad->bHeld == wiipad->pressed) //si son igual no hay nada que refrescar.
        return;

    if(kbind != 0xff)
        _wiimoteBind(wiipad);

    for(n = 0; n < current_buttons; n++)
    {
        if( (wiipad->bHeld & wiipad->pad[n].wiival) != (wiipad->pressed & wiipad->pad[n].wiival) )
        {
            cpc_key = wiipad->pad[n].cpcval;

            if (cpc_key != 0xff)
            {
                if((wiipad->bHeld & wiipad->pad[n].wiival))
                { 
                    keyboard_matrix[cpc_key >> 4] &= ~bit_values[cpc_key & 7]; // tecla pulsada
                    wiipad->pressed |= wiipad->pad[n].wiival;
                }
                else
                {
                    keyboard_matrix[cpc_key >> 4] |= bit_values[cpc_key & 7];  // tecla soltada
                    wiipad->pressed &= ~wiipad->pad[n].wiival;
                }
                return;
            }
        }
    }
}

/*! \fn inline void _wiimoteBind (t_mote * wiipad)
    \brief Guarda el valor del teclado del CPC dado en la posicion del control pulsado.

    \param wiipad control a usar.
    \note cambia el valor de SaveKEY para que al salir del juego las teclas sean guardadas en la gamelist.
*/
inline void _wiimoteBind (t_mote * wiipad)
{
    int n;

    for(n = 0; n < current_buttons; n++)
    {
        if(wiipad->bHeld & wiipad->pad[n].wiival)
        {
            wiipad->pad[n].cpcval = kbind;

            kbind = 0xff;
            Element_SelectImg( &keyb_help, 1 ); //keyb help basic
            WiiStatus.SaveKEY = 1; //need save this keys!
            return;
        }
    }
}

/*! \fn inline void _wiimoteCheck (void)
    \brief Comprueba si el wiimote principal esta apuntando a la pantalla o no.

    \note Solo comprueba cuando la emulacion no esta pausada.
*/
inline void _wiimoteCheck (void)
{
    if ( (!CPC.paused) && (cursor.Left > 0 && cursor.Left < 600) && (cursor.Top > 0 && cursor.Top < 440))
    {
        if(!WpadOnScreen)
            kbs_ticks = GetTicks();

        WpadOnScreen = true;

    }else if ((!CPC.paused) && ((cursor.Left < -60 || cursor.Left > 640) || (cursor.Top < -60 || cursor.Top > 480)))
    {
        WpadOnScreen = false;
    }
}

inline void _padGamecube (t_mote * wiipad, int channel)
{
    signed char pad_x = 0;
    signed char pad_y = 0;
    float t;

    //PAD_SubStickX(channel) <> 70
    //PAD_SubStickY(channel) <> 70
    //PAD_TriggerL(channel) > 18
    //PAD_TriggerR(channel) > 18

    register int n;

    for(n = 0; n < 8; n++)
    {
        if(wiipad->bHeld & gc_binds[n][0])
        {
            wiipad->bHeld |= gc_binds[n][1];
        }
    }

    pad_x = PAD_StickX(channel);
    pad_y = PAD_StickY(channel);

	/***
	Gamecube Joystick input from snes9x-gx
	***/
	// Is XY inside the "zone"?
	if (pad_x * pad_x + pad_y * pad_y > JOY_ZONECAL * JOY_ZONECAL)
	{
		/*** we don't want division by zero ***/
		if (pad_x > 0 && pad_y == 0)
			wiipad->bHeld |= (PAD_BUTTON_RIGHT << 8);
		if (pad_x < 0 && pad_y == 0)
			wiipad->bHeld |= (PAD_BUTTON_LEFT << 8);
		if (pad_x == 0 && pad_y > 0)
			wiipad->bHeld |= (PAD_BUTTON_UP << 8);
		if (pad_x == 0 && pad_y < 0)
			wiipad->bHeld |= (PAD_BUTTON_DOWN << 8);

		if (pad_x != 0 && pad_y != 0)
		{
			/*** Recalc left / right ***/
			t = (float) pad_y / pad_x;
			if (t >= -2.41421356237 && t < 2.41421356237)
			{
				if (pad_x >= 0)
					wiipad->bHeld |= (PAD_BUTTON_RIGHT << 8);
				else
					wiipad->bHeld |= (PAD_BUTTON_LEFT << 8);
			}

			/*** Recalc up / down ***/
			t = (float) pad_x / pad_y;
			if (t >= -2.41421356237 && t < 2.41421356237)
			{
				if (pad_y >= 0)
					wiipad->bHeld |= (PAD_BUTTON_UP << 8);
				else
					wiipad->bHeld |= (PAD_BUTTON_DOWN << 8);
			}
		}
	}
}

/*! \fn inline void _wiimoteClassic (t_mote * wiipad)
    \brief Comprueba el estado del control classic y actualiza su bHeld.

    \param wiipad control a usar.
*/
inline void _wiimoteClassic (t_mote * wiipad)
{
    double val;
    signed char wm_ax = 0;
    signed char wm_ay = 0;
    float mag = 0.0;
    float ang = 0.0;
    float t;

    register int n;

    for(n = 0; n < 8; n++)
    {
        if(wiipad->bHeld & classic_binds[n][0])
        {
            wiipad->bHeld |= classic_binds[n][1];
        }
    }

    for(n = 0; n < 2; n++)
    {
        if(n)
        {
            /* LEFT JOY, taken from snes9x-gx */
            mag = wiipad->pData->exp.classic.ljs.mag;
            ang = wiipad->pData->exp.classic.ljs.ang;
        }
        else
        {
            mag = wiipad->pData->exp.classic.rjs.mag;
            ang = wiipad->pData->exp.classic.rjs.ang;
        }

        /* calculate x/y value (angle need to be converted into radian) */
        if (mag > 1.0)
            mag = 1.0;
        else if (mag < -1.0) 
            mag = -1.0;

        val = mag * sin((PI * ang)/180.0f);
        wm_ax = (s8)(val * 128.0f);

        val = mag * cos((PI * ang)/180.0f);
        wm_ay = (s8)(val * 128.0f);

        // Is XY inside the "zone"?
        if (wm_ax * wm_ax + wm_ay * wm_ay > JOY_ZONECAL * JOY_ZONECAL)
        {

            if (wm_ax > 0 && wm_ay == 0)
                wiipad->bHeld |= (n) ? WPAD_BUTTON_DOWN : WPAD_EXTRA_JOY_RIGHT;
            if (wm_ax < 0 && wm_ay == 0)
                wiipad->bHeld |= (n) ? WPAD_BUTTON_UP : WPAD_EXTRA_JOY_LEFT;
            if (wm_ax == 0 && wm_ay > 0)
                wiipad->bHeld |= (n) ? WPAD_BUTTON_RIGHT : WPAD_EXTRA_JOY_UP;
            if (wm_ax == 0 && wm_ay < 0)
                wiipad->bHeld |= (n) ? WPAD_BUTTON_LEFT : WPAD_EXTRA_JOY_DOWN;

            if (wm_ax != 0 && wm_ay != 0)
            {
                /*** Recalc left / right ***/
                t = (float) wm_ay / wm_ax;
                if (t >= -2.41421356237 && t < 2.41421356237)
                {
                    if (wm_ax >= 0)
                        wiipad->bHeld |= (n) ? WPAD_BUTTON_DOWN : WPAD_EXTRA_JOY_RIGHT;
                    else
                        wiipad->bHeld |= (n) ? WPAD_BUTTON_UP : WPAD_EXTRA_JOY_LEFT;
                }

                /*** Recalc up / down ***/
                t = (float) wm_ax / wm_ay;
                if (t >= -2.41421356237 && t < 2.41421356237)
                {
                    if (wm_ay >= 0)
                        wiipad->bHeld |= (n) ? WPAD_BUTTON_RIGHT : WPAD_EXTRA_JOY_UP;
                    else
                        wiipad->bHeld |= (n) ? WPAD_BUTTON_LEFT : WPAD_EXTRA_JOY_DOWN;
                }
            }
        }
    }

    if(current_buttons != MAX_CPCBUTTONS)
        current_buttons = MAX_CPCBUTTONS;

}

void poll_pads(void)
{
    switch(WiiStatus.padsConnected )
    {
        default:
        case 2:
            controls.wpad2.bDown = (PAD_ButtonsDown(PAD_CHAN1) << 8);
            controls.wpad2.bHeld = (PAD_ButtonsHeld(PAD_CHAN1) << 8);
            _padGamecube(&controls.wpad2, PAD_CHAN1);

            if (controls.wpad2.bDown & (PAD_BUTTON_START << 8))
                emu_paused (CPC.paused ^ 1);

        case 1:
            controls.wpad1.bDown = (PAD_ButtonsDown(PAD_CHAN0) << 8);
            controls.wpad1.bHeld = (PAD_ButtonsHeld(PAD_CHAN0) << 8);
            _padGamecube(&controls.wpad1, PAD_CHAN0);
        
            if (controls.wpad1.bDown & (PAD_BUTTON_START << 8))
                emu_paused (CPC.paused ^ 1);

        case 0: break;
    }

}


/*! \fn void poll_wiimote(void)
    \brief Funcion global que actualiza el teclado del CPC con los controles de la Wii.

    \todo quizas añadir el control del cursor tambien al segundo wiimote.
*/
void poll_wiimote(void)
{
    static unsigned char bindto = 0xff;

    WPAD_ScanPads();
    
    switch(WiiStatus.nWiimotes)
    {
        case 2:
            controls.wpad2.pData = WPAD_Data(WPAD_CHAN_1);
            controls.wpad2.bDown = WPAD_ButtonsDown(WPAD_CHAN_1); //solo cuando se pulsa y luego vuelve a ser 0
            controls.wpad2.bHeld = WPAD_ButtonsHeld(WPAD_CHAN_1);
        
            if (controls.wpad2.bDown & WPAD_BUTTON_HOME)
                emu_paused (CPC.paused ^ 1);

            if(controls.wpad2.pData->exp.type == WPAD_EXP_CLASSIC)
                _wiimoteClassic(&controls.wpad2);

            __poll_wiitojoy(&controls.wpad2);
    
        case 1:
            controls.wpad1.pData = WPAD_Data(WPAD_CHAN_0);
            controls.wpad1.bDown = WPAD_ButtonsDown(WPAD_CHAN_0); //solo cuando se pulsa y luego vuelve a ser 0
            controls.wpad1.bHeld = WPAD_ButtonsHeld(WPAD_CHAN_0); //se mantiene mientras el usuario continue pulsando
            controls.wpad1.bUp   = WPAD_ButtonsUp(WPAD_CHAN_0);

            if(controls.wpad1.pData != NULL)
            {
                if(controls.wpad1.pData->ir.smooth_valid)
                {
                    cursor.Left = (controls.wpad1.pData->ir.sx + CORRECCION_MOTE_X);
                    cursor.Top = (controls.wpad1.pData->ir.sy + CORRECCION_MOTE_Y);
                    cursor.Angle = controls.wpad1.pData->ir.angle;

                    _wiimoteCheck();

                }else if(controls.wpad1.pData->ir.valid)
                {
                    cursor.Left = (controls.wpad1.pData->ir.x + CORRECCION_MOTE_X);
                    cursor.Top = (controls.wpad1.pData->ir.y + CORRECCION_MOTE_Y);
                    cursor.Angle = controls.wpad1.pData->ir.angle;

                    _wiimoteCheck();
                }
                else
                    WpadOnScreen = false;
            }

            if (controls.wpad1.bDown & WPAD_BUTTON_HOME)
            {
                if(WiiStatus.Gunstick)
                {
                    spool = 1;
                    sprintf(spool_cad, " GUNSTICK MODE: FINISHED...");
                    WiimoteSetupGun(0);
                    WiiStatus.Gunstick = 0;
                }
                else
                    emu_paused (CPC.paused ^ 1);

            }

            if( (!WiiStatus.Gunstick) ) 
            {
                if(screen_y)
                {
                    if ((controls.wpad1.bDown & WPAD_BUTTON_A)&&(controls.wpad1.bDown & WPAD_BUTTON_B))
                    {
                        spool = 1;
                        sprintf(spool_cad, " BINDS CLEANED...");

                        int n;
                        for(n = 0; n < MAX_CPCBUTTONS; n++)
                        {
                            controls.wpad1.pad[n].cpcval = buttons_def[n];
                            controls.wpad2.pad[n].cpcval = buttons_def[n + MAX_CPCBUTTONS];
                        }
                    }

                    if ((controls.wpad1.bDown & WPAD_BUTTON_A))
                    {
                        KeyboardCheck(KEY_DOWN); //comprueba tecla
                    }else if(controls.wpad1.bUp & WPAD_BUTTON_A)
                    {
                        KeyboardCheck(KEY_UP); //inicializa
                    }

                    if ((controls.wpad1.bDown & WPAD_BUTTON_B))
                    {
                        bindto = KeyboardCheck(KEY_GET);
                    }else if((controls.wpad1.bUp & WPAD_BUTTON_B) && (bindto != 0xff))
                    {
                        kbind = bindto;
                        bindto = 0xff;
                        Element_SelectImg( &keyb_help, 2 ); //ayuda "to bind"
                    }
                }
            }
            else
            {
                if (controls.wpad1.bDown & ( WPAD_BUTTON_B)){ // WPAD_BUTTON_A - solo al disparar (bDown)
                    controls.wpad1.bHeld |= WPAD_BUTTON_2; // 2 - gunstick - 1 - phaser
                    gunstick.state = WII_GUNSTICK_SHOOT; //prepare detect...
                }
            }

            if(controls.wpad1.pData->exp.type == WPAD_EXP_CLASSIC)
                _wiimoteClassic(&controls.wpad1);

            __poll_wiitojoy(&controls.wpad1);
            break;
    }

}

/*! \fn int WiimoteSetup (int channel)
    \brief Configura un canal para poder hacer uso del correspondiente wiimote.

    \param channel Canal a inicializar.
    \return Retorna el valor devuelto al ser inicializado el canal.
*/
int WiimoteSetup (int channel)
{
    u32 wDev;
    int error = WPAD_ERR_NONE;
    error = WPAD_Probe(channel, &wDev);

    if ( error == WPAD_ERR_NONE)
    {
        WPAD_SetVRes(channel, rmode->fbWidth, rmode->xfbHeight);
        WPAD_SetDataFormat(channel, WPAD_FMT_BTNS_ACC_IR);
    }

    return error;
}

/*! \fn void WiimoteInit( void)
    \brief Inicializa el sistema de wiimotes de la Wii.

*/
void WiimoteInit( void)
{
    PAD_Init(); //GC Pads
    WPAD_Init();
    WPAD_SetIdleTimeout(120); // Wiimote shutdown ...

    WPAD_ScanPads();
    PAD_ScanPads();

    //configuramos por defecto un wiimote
    WPAD_SetVRes(WPAD_CHAN_0, rmode->fbWidth, rmode->xfbHeight);
    WPAD_SetDataFormat(WPAD_CHAN_0, WPAD_FMT_BTNS_ACC_IR);
    WiiStatus.nWiimotes = 1;

    current_buttons = WIIMOTE_ONLY_BUTTONS;
}


/*! \fn void WiimoteSaveKeys (void)
    \brief Guarda el contenido de los binds en la gamelist.
 
    \note Si la gamelist (current_game) tiene un valor erroneo, sale sin modificar nada.
*/
void WiimoteSaveKeys (void)
{
    if(current_game == NULL)
        return;

    int n;
    for(n = 0; n < MAX_CPCBUTTONS; n++)
    {
        current_game->binds[n] = controls.wpad1.pad[n].cpcval;
        current_game->binds[n + MAX_CPCBUTTONS] = controls.wpad2.pad[n].cpcval;
    }

}

/*! \fn void WiimoteLoadKeys (void)
    \brief Lee el contenido de los binds de la gamelist y actualiza los binds de los pads.
 
    \note Si la gamelist (current_game) tiene un valor erroneo, sale sin modificar nada.
*/
void WiimoteLoadKeys (void)
{

    if(current_game == NULL)
        return;

    int n;
    for(n = 0; n < MAX_CPCBUTTONS; n++)
    {
        controls.wpad1.pad[n].cpcval = current_game->binds[n];
        controls.wpad2.pad[n].cpcval = current_game->binds[n + MAX_CPCBUTTONS];
    }


}

/*! \fn void WiimoteSetupGun( int Init)
    \brief Configura el cursor que debe ser mostrado dependiendo del estado requerido por Init, 
    esta funcion es llamada cada vez que se activa/desactiva la emulacion.
 
    \todo Actualizar el valor del color del blanco requerido por la gunstick cada vez que se vuelva a la emulacion (cambia segun el gamma).
      Guardar Status.gunstick desde aqui y poner un valor correcto, y sacar de la pausa del main...
*/
void WiimoteSetupGun( int Init)
{
    if(Init){
        Element_SelectImg( &cursor, 3 ); //cursor de pistola
    } else {
        Element_SelectImg( &cursor, 1 ); //hand open
    }

    if(!WiitukaXML.scrtube)
        gunstick.hcolor = WII_GUNSTICK_HITCOLOR; //WII_GUNSTICK_MINCOLOR;
    else
        gunstick.hcolor = WII_GUNSTICK_HITGREEN;


}

/*! \fn unsigned char Wiimote_CheckGun (void)
    \brief Comprueba el estado de la emulacion del Gunstick y activa sus resortes.
 
    \return 0xff, si fallo o 0xfd (joy abajo) si hubo acierto.
*/
unsigned char Wiimote_CheckGun (void)
{
    if(gunstick.state == WII_GUNSTICK_SLEEP)
        return 0xff; //nada
    
    if(gunstick.state == WII_GUNSTICK_SHOOT) {
        gunstick.timer = GetTicks(); //cogemos el timer
        gunstick.state = WII_GUNSTICK_XYGET;
    }
    else if((GetTicks()-gunstick.timer) > WII_GUNSTICK_TIMER) //si han pasado...
        gunstick.state = WII_GUNSTICK_SLEEP;
    else if(__gunstick_checkHit())
        return 0xfd; //joy arriba phaser fe / gunstick fd

    return 0xff; //nada
}

/*! \fn bool __gunstick_checkHit (void)
    \brief Guarda la posicion de la mirilla y comprueba el color del pixel al que se disparo.
 
    \return true, si hubo acierto.
*/
inline bool __gunstick_checkHit (void)
{
    unsigned int gcolor;
    
    if(gunstick.state == WII_GUNSTICK_XYGET)
    {
        gunstick.x = cursor.Left + GUNSTICK_CURSOR_FIXED_X;
        gunstick.y = cursor.Top + GUNSTICK_CURSOR_FIXED_Y;
        gunstick.state = WII_GUNSTICK_SSEND;
    }

    gcolor = GetXYScreen (gunstick.x, gunstick.y);

    if(!gcolor)
        gunstick.state = WII_GUNSTICK_PREPARE;

    //if(gcolor > gunstick.hcolor ){ // blanco? - todo!
    if((gunstick.state != WII_GUNSTICK_PREPARE) && 
       (gcolor == gunstick.hcolor)) {
        spool = 1;
        sprintf(spool_cad, "get: %x", gcolor);
        return true;
    }

    return false;
}

/*! \fn void ShowLed(int val)
    \brief Activa el led 4 del Wiimote principal, para indicar que la disquetera del CPC esta en funcionamiento.


    \param val encender o apagar led.
*/
void ShowLed(int val)
{
    static int old_val = -1;

    if(old_val != val){
        old_val = val;
        WPAD_SetLeds(WPAD_CHAN_0, WIIMOTE_LED_1 | (WIIMOTE_LED_4 * val)); //siempre enviamos 1, pero solo activamos LED_4 si val es 1.
    }

}


