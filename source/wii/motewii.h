/*! \file motewii.h
 *  \brief Interfaces publicos de la libreria de Control.
 *         Libreria de Wii.
 *
 *  \version 0.9
 *
 *  Wii/GC Control driver
 *
 *   Pituka - Nintendo Wii/Gamecube Port
 *  (c) Copyright 2008-2009 David Colmenero (aka D_Skywalk)
 */

#ifndef motewii_h
#define motewii_h


/** \brief Estructura de Boton
 *  Esta es una estructura que administra el control del CPC y de Wii.
 */
typedef struct emubutton
{
  unsigned int wiival;  /**< Valor del Boton de Wii */
  unsigned char cpcval; /**< Valor de la tecla del CPC */
} t_emubutton;


/** \brief Estructura del Wiimote
 *  Esta es una estructura que controla el Wiimote y su emulacion.
 */
typedef struct mote
{
  t_emubutton pad [MAX_CPCBUTTONS]; /**< Botones a comprobar durante la emulacion */
  unsigned int pressed; /**< Teclas activas para la emulacion */
  unsigned int bDown; /**< Botones del Wiimote que acaban de ser pulsados */
  unsigned int bHeld; /**< Botones del Wiimote que se mantienen pulsados */
  unsigned int bUp; /**< Botones del Wiimote que acaban de ser soltados */ 
  WPADData *   pData; /**< Puntero a la Estructura que controla el Wiimote */
}t_mote;

/** \brief Wiimotes a controlar
 *  Controles que se manejan durante la emulacion.
 */
typedef struct wiipads
{
  t_mote wpad1; /**< Wiimote del jugador 1 */
  t_mote wpad2; /**< Wiimote del jugador 2 */
}t_wiipads;


void poll_wiimote(void);
void poll_pads(void);

int WiimoteSetup(int channel);
void WiimoteInit( void);

void WiimoteSaveKeys(void);
void WiimoteLoadKeys(void);

#define GUNSTICK_USE 1
#define GUNSTICK_CURSOR_FIXED_X 30 
#define GUNSTICK_CURSOR_FIXED_Y 18 

#define WII_GUNSTICK_SLEEP    0
#define WII_GUNSTICK_SHOOT    1
#define WII_GUNSTICK_XYGET    2
#define WII_GUNSTICK_SSEND    3
#define WII_GUNSTICK_PREPARE  4

#define WII_GUNSTICK_TIMER    500
#define WII_GUNSTICK_HITCOLOR 0xc658
#define WII_GUNSTICK_HITGREEN 0x4a0

#define WII_GUNSTICK_MINCOLOR 0xc300 // try with min color


#define WPAD_EXTRA_JOY_UP				0x2000
#define WPAD_EXTRA_JOY_DOWN				0x4000
#define WPAD_EXTRA_JOY_LEFT				0x0020
#define WPAD_EXTRA_JOY_RIGHT				0x0040

#define WIIMOTE_ONLY_BUTTONS 10


void WiimoteSetupGun( int Init );
unsigned char Wiimote_CheckGun (void);
void ShowLed(int val);

#endif

