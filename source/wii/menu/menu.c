/*! \file menu.c
 *  \brief Menu Driver.
 *         Wii Library.
 *
 *  \version 0.2
 *
 *  Wii/GC Sistema de Menu
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
#include <asndlib.h>
#include <mp3player.h>

#include "../../global.h"
#include "../../port/dskutils.h"
#include "../videowii.h"
#include "../motewii.h"
#include "../keybwii.h"
#include "../wiixml.h"
#include "elements.h"
#include "explorer.h"

#include "../images/gfx_defines.h"
#include "../sound/snd_defines.h"

#include "../tcp/net.h"
#include "../grrlib/GRRLIB.h"

#include "menu.h"
#include "menu_db.h"

extern t_CPC CPC;
extern int menuLoaded;
extern t_WiiRom globalRom;
extern WiituKa_Status WiiStatus;
extern xmlWiiCFG WiitukaXML;
extern char current_dev[8 + 1];


t_element cursor;
t_img fondo;
//t_img fondo_compo;
t_img keyb;
t_element keyb_help;
t_element b_floppy;
t_element b_gun;
t_element b_keyb;
t_element b_snap;
t_element b_snap_save;
t_element b_snap_load;

t_img f_devices[5];

#define FDEV_IMAGE_FAT 0
#define FDEV_IMAGE_NET 1
#define FDEV_IMAGE_WM1 2
#define FDEV_IMAGE_WM2 3
#define FDEV_IMAGE_KEY 4

t_fslist gamelist = { 
            {"", SU_NONE}, 
            { "", "", "", "", ""}, 0, 
            { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
              0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff  },
            NULL };

t_fslist * current_gamelist = &gamelist;
t_fslist * current_game = NULL;

sMenuEntry * menuCurrent;
sMenuEntry fileDB;

char debugt [1024] = "";

t_button renderBtn[MAXBUTTONS], BtnOptC, BtnOptX; //Botones estaticos.

extern t_wiipads controls;
extern Wii_gun gunstick;

extern int emuDone;

extern Pituka_SpoolKeys spoolkeys;
extern unsigned char spool;
extern char spool_cad[256];
extern unsigned char reiniciado;

/*  WII VIDEO  */
extern float screen_x;
extern float screen_y;
extern float cpc_zoom;

#define CHECK 3
#define PAINT 2
#define LOAD 1
#define UNLOAD 0

/* BUTTONS THREAD */
lwp_t animthreads[1];
int menuthread_callanimButtons( int btn, sMenuEntry * current );
void *menuthread_animButtons(void * arg);
void Menu_UpdateButtonsLabels ( int pbpos );
#define ANIM_NEXT_ROWS 640
#define ROWS_PER_SCREEN 6
int glistposition = 0;


int wii_inputmenu (void){

    GRRLIB_VSync();
    WPAD_ScanPads();

    unsigned int Buttons = WPAD_ButtonsDown(WPAD_CHAN_0);

    if((Buttons))
        return 1;
    else
        return 0;

}


bool WelcomeScreen (int command)
{
    switch(command)
    {
        case LOAD:
            if( !LoadTexture( &fondo, gui_splash_screen_png) )
            {
                fprintf( stderr, "Init Splash failed \n");
                return false;
            }
            break;

        case UNLOAD:
            //liberamos anterior.
            FreeImg(&fondo);
            break;
  }
  return true;
}

#define CURSOR_FIXED_X 20
#define CURSOR_FIXED_Y 10

void ButtonsupdateStatus(sMenuEntry * current)
{
    int n;

    if(current->buttons[0].btype == ButtonNode) //si es romlist
        return;

    for(n = 0; (n < current->nbuttons); n++)
    {
        switch(current->buttons[n].action)
        {
            case ACTION_OPT_DEBUG:
                if(WiitukaXML.cpcfps)
                    Element_SelectImg( &renderBtn[0].Obj, 2 );
                else
                    Element_SelectImg( &renderBtn[0].Obj, 1 );
                break;
            
            case ACTION_OPT_GREEN:
                if(WiitukaXML.scrtube)
                    Element_SelectImg( &renderBtn[1].Obj, 2 );
                else
                    Element_SelectImg( &renderBtn[1].Obj, 1 );
                break;

            case ACTION_OPT_RELOAD:
                if((WiiStatus.UpdateDIR == 1) || (WiiStatus.UpdateXMLNET == 1))
                    Element_SelectImg( &renderBtn[2].Obj, 2 );
        }
    }
}

void ButtonsdoAction(int action)
{
    switch(action)
    {
        case ACTION_OPT_DEBUG:
            WiitukaXML.cpcfps ^= 1;
            CPC.scr_fps ^= 1;
            break;

        case ACTION_OPT_GREEN:
            WiitukaXML.scrtube ^= 1;
            WiiStatus.SaveXML = 1;
            video_set_palette();
            break;

        case ACTION_OPT_RELOAD:
            if(WiiStatus.Dev_Fat)
                WiiStatus.UpdateDIR = 1;

            if(WiiStatus.Dev_Net)
                WiiStatus.UpdateXMLNET = 1;
            break;
   }
}

bool ButtonsCommon (int command, sMenuEntry * current)
{
    int n = 0;
    bool select = false;
    t_fslist * gnode = current_gamelist; 

    for(n = 0; (n < current->nbuttons); n++)
    { //lista de r00ms y botones

        switch (command)
        {
            case LOAD:
                Button_Init(&renderBtn[n], current->buttons[n].label, "", "", current->buttons[n].btype);
                Button_SetXY(&renderBtn[n], current->buttons[n].px, current->buttons[n].py);
                break;

            case UNLOAD:
                Button_Free(&renderBtn[n]);
                break;

            case CHECK:
                if(current->buttons[0].btype == ButtonNode)
                {
                    if(gnode->fnext == NULL)
                        break;

                    if( Button_IsInside(&renderBtn[n], cursor.Left + CURSOR_FIXED_X, cursor.Top + CURSOR_FIXED_Y) )
                    {
                        select = true;
                        Element_SelectImg( &renderBtn[n].Obj, 2 );
                        if(controls.wpad1.bDown & WPAD_BUTTON_A)
                        {
                            WiiStatus.LoadDISK = 1;
                            emulator_reset(true);
                        }

                        if(controls.wpad1.bDown & (WPAD_BUTTON_A | WPAD_BUTTON_B))
                        {
                            current_game = gnode;
                            memcpy(&globalRom, &gnode->gfile, sizeof(t_WiiRom));
                            WiiStatus.LoadDISK += 1;
                            emu_paused (0);
                            return true;
                        }
                    }
                    else
                        Element_SelectImg( &renderBtn[n].Obj, 1 ); //sin seleccionar

                    gnode = gnode->fnext;

                }
                else
                {
                    if( Button_IsInside(&renderBtn[n], cursor.Left + CURSOR_FIXED_X, cursor.Top + CURSOR_FIXED_Y) )
                    {
                        select = true;
                        if(controls.wpad1.bDown & WPAD_BUTTON_A)
                        {
                            ButtonsdoAction(current->buttons[n].action);
                            ButtonsupdateStatus(current);
                        }
                    }
                }
                break;

            case PAINT:
                if(current->buttons[0].btype == ButtonNode)
                {
                    if(gnode->fnext != NULL)
                        gnode = gnode->fnext;
                    else
                        break;
                }
                Button_Paint(&renderBtn[n]);
                break;
        }
    }

    switch (command) //boton de opciones, cerrar y extras.
    {
        case LOAD:
            Button_Init(&BtnOptC, "", "", "",  ButtonOPTc);
            Button_SetXY(&BtnOptC, 470, 25);
            Button_Init(&BtnOptX, "", "", "", ButtonOPTx);
            Button_SetXY(&BtnOptX, 530, 25);

            if(current->buttons[0].btype == ButtonNode){ //es un menu de roms, update it!
                Menu_UpdateButtonsLabels(-1);
                Menu_UpdateButtonsLabels(0);
                Menu_UpdateButtonsLabels(1);
                Menu_UpdateButtonsLabels(2);

                //actualizar setxy, sino fallan los primeros botones (year)
                for(n = 0; (n < current->nbuttons); n++)
                    Button_SetXY(&renderBtn[n], current->buttons[n].px, current->buttons[n].py);

                }
                else
                    ButtonsupdateStatus(current);
            break;

        case UNLOAD:
            Button_Free(&BtnOptC);
            Button_Free(&BtnOptX);
            break;

        case CHECK:
            if( Button_IsInside(&BtnOptX, cursor.Left + CURSOR_FIXED_X, cursor.Top + CURSOR_FIXED_Y) )
            {
                select = true;
                if(controls.wpad1.bDown & WPAD_BUTTON_A)
                {
                    emuDone = true;
                }
            }

            if( Button_IsInside(&BtnOptC, cursor.Left + CURSOR_FIXED_X, cursor.Top + CURSOR_FIXED_Y) )
            {
                select = true;
                if(controls.wpad1.bDown & WPAD_BUTTON_A)
                {
                    if(WiiStatus.CurrentMenu < 0)
                        WiiStatus.CurrentMenu = 2; //opt menu
                    else
                        WiiStatus.CurrentMenu = -1; //romlist
                }
            }
            break;

        case PAINT:
            Button_Paint(&BtnOptC); //pintamos las opciones junto con el primer boton
            Button_Paint(&BtnOptX);
            break;
    }

    //in pause search second wiimote and gc pads
    if(WiimoteSetup(WPAD_CHAN_1) == WPAD_ERR_NONE)
        WiiStatus.nWiimotes = 2;
    else
        WiiStatus.nWiimotes = 1;
        
    WiiStatus.padsConnected = PAD_ScanPads();


    if(current->buttons[0].btype == ButtonNode)
    {
        if((controls.wpad1.bDown & (WPAD_BUTTON_LEFT|WPAD_BUTTON_MINUS)) && (glistposition > 0))
            menuthread_callanimButtons(1, current);
        else if((controls.wpad1.bDown & (WPAD_BUTTON_RIGHT|WPAD_BUTTON_PLUS)) && ((glistposition + ROWS_PER_SCREEN) < (WiiStatus.nRoms)))
        {
            menuthread_callanimButtons(0, current);
            //if (MP3Player_IsPlaying()) { MP3Player_Stop(); }
        }
    }

    if(!(controls.wpad1.bHeld & WPAD_BUTTON_HOME)) //evita que al salir de la emulacion se pierda el icono de la gunstick
    {
        if(select) //cursor encima de boton?
            Element_SelectImg( &cursor, 2 );
        else
            Element_SelectImg( &cursor, 1 ); // nada...
    }

    return true;
}


void Menu_UpdateButtonsLabels ( int pbpos )
{
    int glist_goal = 0;
    int bpos = 0;
    int n = 0;

    char tmp[256];

    t_fslist * gnode = current_gamelist; //guarda la cabecera

    switch(pbpos)
    {
        case -1: // <<<
            if(glistposition < 0)
                return;
            break;

        case  2:  // >>> * 2
            glist_goal += ROWS_PER_SCREEN; //roms a saltar
            bpos += ROWS_PER_SCREEN; //boton a usar, a partir de...

        case  1:  // >>>
            glist_goal += ROWS_PER_SCREEN;
            bpos += ROWS_PER_SCREEN;

        case 0:  // ===
            glist_goal += ROWS_PER_SCREEN;
            bpos += ROWS_PER_SCREEN;
            break;

        default: return; //error?
    }


    while ((n++ < glist_goal)&&(gnode->fnext != NULL))
        gnode = gnode->fnext;

    for(n = 0;(n < ROWS_PER_SCREEN) && (gnode->fnext != NULL); n++, bpos++)
    {
        if(( strlen(gnode->game.company) && (strlen(gnode->game.company) < 20)) && (strlen(gnode->game.title) < 25))
            sprintf(tmp, "%s (%s) ", gnode->game.title, gnode->game.company);
        else
            sprintf(tmp, "%s ", gnode->game.title);

    #if 0 //only for debug
        if(gnode->gfile.location == 3)
            strcat(tmp, "- SD");
        else if(gnode->gfile.location == 4)
            strcat(tmp, "- WWW");
    #endif

        if(gnode->gfile.location == 0)
            strcat(tmp, "(NULL?)");

        Button_SetCaption(&renderBtn[bpos], tmp, gnode->game.genre, gnode->game.year);

        gnode = gnode->fnext;
    }
}

int Menu_InitList (void)
{
    int glist_goal = 0;
    int screen_cnt = 0;
    int n; 

    current_gamelist = &gamelist; //reload gamelist

    if(!glistposition)
    {
        glist_goal = 0;
        screen_cnt = 0;
    }else if( ((WiiStatus.nRoms / ROWS_PER_SCREEN) - (glistposition / ROWS_PER_SCREEN) ) >= 2)
    {
        glist_goal = (glistposition - ROWS_PER_SCREEN);
        screen_cnt = -ANIM_NEXT_ROWS; //muestra pagina 2
    }else
    {
        glist_goal = ( WiiStatus.nRoms - ROWS_PER_SCREEN*3);
        screen_cnt = -(ANIM_NEXT_ROWS * ((glistposition - glist_goal) / ROWS_PER_SCREEN));
    }

    for (n = 0; (n < glist_goal) && (current_gamelist->fnext != NULL); n++)
        current_gamelist = current_gamelist->fnext;

    return screen_cnt;
}

sMenuEntry * Menu_ExplorerList ( void )
{
    int n = 0, y = 40; 
    int screen_cnt = 0;

    t_fslist * gnode = &gamelist; //guarda la cabecera

    if(WiiStatus.UpdateXMLNET || WiiStatus.UpdateDIR)
    {
        sprintf(debugt,"Loading Rom List, Please Wait...");
        ShowWait();

        Explorer_LoadFilelist(CPC_ROMSDIR);
    }

    if(gnode->fnext == NULL)
        return &fileDB;

/***  ***/

    screen_cnt = Menu_InitList();

    do
    {
        strcpy(fileDB.buttons[n].label, gnode->game.title);
        fileDB.buttons[n].btype = ButtonNode;
        fileDB.buttons[n].px = (640 / 2) + screen_cnt;

        y += 50;
        fileDB.buttons[n].py = y;
        fileDB.buttons[n].action = 0;

        n++;
        gnode = gnode->fnext;

        if((n % ROWS_PER_SCREEN) == 0)
        {
            y = 40;
            screen_cnt += ANIM_NEXT_ROWS;
        }

    } while((gnode->fnext != NULL) && (n < MAXGAMES));

    fileDB.nbuttons = n;

    return &fileDB;
}

bool MenuCommon (int command, int section)
{
    //bool roms_local = false;

    switch(command)
    {
        case LOAD:
            if( !LoadTexture( &fondo, gui_background_png) )
            {
                fprintf( stderr, "Init Fondo failed \n");
                return false;
            }

            if( section < 0 ) {
                //if( (menuCurrent = Menu_ExplorerList()) != NULL)
                //    roms_local = true;
                menuCurrent = Menu_ExplorerList();
            }
            else
                menuCurrent = &menuDB[section];

            if( !LoadTexture( &keyb, gui_keyboard_png) )
            {
                fprintf( stderr, "Init Keyb-PNG failed \n");
                return false;
            }

            Element_Init(&keyb_help);
            if(!Element_allocImgs(&keyb_help, 2))
                return false;

            if(!Element_LoadImg( &keyb_help, gui_keyboard_help_normal_png )) // primero off
                return false;

            if(!Element_LoadImg( &keyb_help, gui_keyboard_help_tobind_png )) // on
                return false;

            Element_SelectImg( &keyb_help, 1 ); //por defecto, normal

            Element_Init(&b_floppy);
            if(!Element_allocImgs(&b_floppy, 1))
                return false;

            if(!Element_LoadImg( &b_floppy, button_emu_floppy_png ))
                return false;

            Element_SelectImg( &b_floppy, 1 );

            Element_Init(&b_gun);
            if(!Element_allocImgs(&b_gun, 1))
                return false;

            if(!Element_LoadImg( &b_gun, button_emu_gun_png ))
                return false;

            Element_SelectImg( &b_gun, 1 ); 

            Element_Init(&b_keyb);
            if(!Element_allocImgs(&b_keyb, 1))
                return false;

            if(!Element_LoadImg( &b_keyb, button_emu_keyb_png ))
                return false;

            Element_SelectImg( &b_keyb, 1 ); 

            Element_Init(&b_snap);
            if(!Element_allocImgs(&b_snap, 1))
                return false;

            if(!Element_LoadImg( &b_snap, button_emu_snap_png ))
                return false;

            Element_SelectImg( &b_snap, 1 ); 

            Element_Init(&b_snap_save);
            if(!Element_allocImgs(&b_snap_save, 1))
                return false;

            if(!Element_LoadImg( &b_snap_save, button_emu_snap_save_png ))
                return false;

            Element_SelectImg( &b_snap_save, 1 ); 

            Element_Init(&b_snap_load);
            if(!Element_allocImgs(&b_snap_load, 1))
                return false;

            if(!Element_LoadImg( &b_snap_load, button_emu_snap_load_png ))
                return false;

            Element_SelectImg( &b_snap_load, 1 ); 

            if( !LoadTexture( &f_devices[FDEV_IMAGE_FAT], devices_fat_png) )
                return false;

            if( !LoadTexture( &f_devices[FDEV_IMAGE_NET], devices_net_png) )
                return false;

            if( !LoadTexture( &f_devices[FDEV_IMAGE_WM1], devices_wm1_png) )
                return false;

            if( !LoadTexture( &f_devices[FDEV_IMAGE_WM2], devices_wm2_png) )
                return false;

            if( !LoadTexture( &f_devices[FDEV_IMAGE_KEY], devices_key_png) )
                return false;
            break;
    
        case UNLOAD:
            FreeImg(&fondo);
            FreeImg(&keyb);
            break;
    }

  return true;
}


void MenuFree(void)
{
    ButtonsCommon(UNLOAD,menuCurrent);
    MenuCommon (UNLOAD, menuLoaded); 
}

void MenuInit(void)
{
    Cursor_Init(&cursor);
}

void MenuClose(void)
{
    MenuFree();

    Cursor_Free(&cursor);

    //if (MP3Player_IsPlaying()) { MP3Player_Stop(); }

}

void ShowDevicesStatus(void)
{

    if(WiiStatus.Dev_Fat)
        DrawTexture(&f_devices[FDEV_IMAGE_FAT], 514, 396, 0, 1, 1, 255);

    if(WiiStatus.Dev_Net)
        DrawTexture(&f_devices[FDEV_IMAGE_NET], 488, 402, 0, 1, 1, 255);

    switch(WiiStatus.nWiimotes)
    {
        case 2:
            DrawTexture(&f_devices[FDEV_IMAGE_WM2], 364, 32, 0, 1, 1, 255);
        case 1:
            DrawTexture(&f_devices[FDEV_IMAGE_WM1], 377, 32, 0, 1, 1, 255);
    }

    if(WiiStatus.Dev_Keyb)
        DrawTexture(&f_devices[FDEV_IMAGE_KEY], 390, 34, 0, 1, 1, 255);

}


void ShowSplash (void)
{

    //TODO: add new layer

    WelcomeScreen (LOAD);
    bool ended = false;

    //DrawTexture(&fondo_compo, 0, 0, 0, 1, 1, 255);
    //UpdateScreen();

    //sleep(5);
    //MP3Player_Volume(200);

    while( !ended )
    {
        DrawTexture(&fondo, 125, 105, 0, 1.66, 1.66, 255);

        if(WiiStatus.Dev_Net < 0)
        {
            sprintf(debugt, "Wiituka Is Waiting for Lan IP using your DHCP server (%i)", WiiStatus.Dev_Net );
        }
        else
        {
            if(WiiStatus.UpdateXMLNET)
                sprintf(debugt, "Updating your romlist! Press any button to continue... f(%i)", WiiStatus.Dev_Fat);
            else if(!WiiStatus.Dev_Net)
                sprintf(debugt, "Wiituka Is Ready (offline), Press any button to Continue...");
            else
                sprintf(debugt, "Wiituka Is Ready, Press any button to Continue...");

            ended = wii_inputmenu();
        }

        PrintW (100, 320, debugt);
        UpdateScreen();
  }

  WelcomeScreen (UNLOAD);

}

void ShowWait (void)
{
    DrawTexture(&fondo, 0, 0, 0, 1, 1, 255);
    PrintW (100, 450, debugt);
    UpdateScreen();
    GRRLIB_VSync();
}

char music_string[255] = {""};
int  music_banner = 640; 
void PlayBgMusic (void)
{
#if 0
    static u32 music_timer = 0;

    //if (MP3Player_IsPlaying())
    if(ym_playing == 1)
    {
        music_timer = GetTicks();
        return;
    }

    if((GetTicks() - music_timer) > 15000)
    {
        u32 music_rand = (rand()%4) + 1;
        switch(music_rand)
        {
            case 1:
                //MP3Player_PlayBuffer(bckg_a_mp3, bckg_a_mp3_size, NULL);
                strcpy(music_string, "Music by Cesar Astudillo (Gominolas) of the game Titanic");
                break;

            case 2:

                //MP3Player_PlayBuffer(bckg_b_mp3, bckg_b_mp3_size, NULL);
                strcpy(music_string, "Music by Cerror - A beginning");
                break;

            case 3:
                //MP3Player_PlayBuffer(bckg_c_mp3, bckg_c_mp3_size, NULL);
                strcpy(music_string, "Music by Ultrasyd - Lost in Fractal Dimension");
                break;

            case 4:
                //MP3Player_PlayBuffer(bckg_d_mp3, bckg_d_mp3_size, NULL);
                strcpy(music_string, "Music by Fenyx Kell - Solarium");
                break;
        }
         
        music_banner = 640; 
   }
#endif   
 
}

void ShowMenu (int nMenu)
{

    if(nMenu != menuLoaded){
        if(menuLoaded != MENU_NULL)
            MenuFree();

        MenuCommon (LOAD, nMenu);
        ButtonsCommon(LOAD, menuCurrent);

        menuLoaded = nMenu;
    }

    DrawTexture(&fondo, 0, 0, 0, 1, 1, 255);

    sprintf(debugt,"DEBUG: ROM(%s) - f(%i) n(%i) ", globalRom.filename, WiiStatus.Dev_Fat, WiiStatus.nRoms);

    //sprintf(debugt,"HELP: (A) LOAD ROM + AUTORUN");
    PrintW (100, 400, debugt);
    sprintf(debugt,"      (B) ONLY LOAD ROM");
    PrintW (100, 410, debugt);
    sprintf(debugt,"Wiituka v0.98.9 - BETA" );
    PrintW (20, 450, debugt);

    if(music_banner > -640)
    {
        music_banner--;
        sprintf(debugt,"%s", music_string );
        PrintW (40+music_banner, 435, debugt);
    }

    ButtonsCommon(PAINT, menuCurrent);
    ButtonsCommon(CHECK, menuCurrent);

    ShowDevicesStatus();
    PlayBgMusic();
    Element_Paint(&cursor);
    GRRLIB_VSync();

}

/* * * EMU WII EXTRAS * * */

extern bool WpadOnScreen;

#define CONST_SCREEN_SUM 2
extern unsigned int kbs_ticks;
extern unsigned char kbind;

#define STATE_NONE -1
#define STATE_KEYB 1
#define STATE_SNAP 2
#define STATE_FLOPPY 3
#define STATE_GUN 4
#define STATE_SNAP_SAVE 5
#define STATE_SNAP_LOAD 6

int __checkEmuButton (void)
{
    if( Element_IsInside(&b_keyb, cursor.Left + CURSOR_FIXED_X, cursor.Top + CURSOR_FIXED_Y) )
    {
        if(controls.wpad1.bDown & WPAD_BUTTON_A)
        {
            return STATE_KEYB;
        }
    }else if( Element_IsInside(&b_snap, cursor.Left + CURSOR_FIXED_X, cursor.Top + CURSOR_FIXED_Y) )
    {
        if(controls.wpad1.bDown & WPAD_BUTTON_A)
        {
            return STATE_SNAP;
        }
    }else if( Element_IsInside(&b_floppy, cursor.Left + CURSOR_FIXED_X, cursor.Top + CURSOR_FIXED_Y) )
    {
        if(controls.wpad1.bDown & WPAD_BUTTON_A)
        {
            return STATE_FLOPPY;
        }
    }else if( Element_IsInside(&b_gun, cursor.Left + CURSOR_FIXED_X, cursor.Top + CURSOR_FIXED_Y) )
    {
        if(controls.wpad1.bDown & WPAD_BUTTON_A)
        {
            return STATE_GUN;
        }
    }

    if( Element_IsInside(&b_snap_save, cursor.Left + CURSOR_FIXED_X, cursor.Top + CURSOR_FIXED_Y) )
    {
        if(controls.wpad1.bDown & WPAD_BUTTON_A)
            return STATE_SNAP_SAVE;
            
    }else if( Element_IsInside(&b_snap_load, cursor.Left + CURSOR_FIXED_X, cursor.Top + CURSOR_FIXED_Y) ) {
        if(controls.wpad1.bDown & WPAD_BUTTON_A)
            return STATE_SNAP_LOAD;
    }

    return STATE_NONE;
}


//extern u32 gcolor;
void ShowEmuCommon ( void)
{
    static int mstate = STATE_NONE;
    static int p_buttons = 0;
    int snap_state = 0;

    if( WiiStatus.Gunstick == true)
    {
        Element_Paint(&cursor);
        //sprintf(debugt,"DEBUG: m(%i,%i) t(%i) s(%i) c(%x)", (int) ((gunstick.x)/cpc_zoom), (int) ((gunstick.y)/cpc_zoom), gunstick.timer, gunstick.state, gcolor);
        //PrintW (100, 450, debugt);

    }
    else
    {
        if( WpadOnScreen )
        {
            if(p_buttons > -50)
            {
                Element_SelectImg( &cursor, 2 );
                p_buttons -= CONST_SCREEN_SUM;
                Element_SetXY(&b_snap, 12 , 470 + p_buttons);
                Element_SetXY(&b_snap_save, 12 - 180 , 420);
                Element_SetXY(&b_snap_load, 72 - 180 , 420);

                Element_SetXY(&b_floppy, 72, 470 + p_buttons);
                Element_SetXY(&b_gun, 132, 470 + p_buttons);
                Element_SetXY(&b_keyb, 192, 470 + p_buttons);

            }

            switch (mstate)
            {
                case STATE_SNAP:
                    if(p_buttons > -180)
                    {
                        p_buttons -= CONST_SCREEN_SUM * 2;
                        Element_SetXY(&b_snap_save, 12 - 180 - p_buttons, 420);
                        Element_SetXY(&b_snap_load, 72 - 180 - p_buttons, 420);

                        Element_SetXY(&b_floppy, 72, 470 - p_buttons); //sacamos los botones de pantalla
                        Element_SetXY(&b_gun, 132, 470 - p_buttons);
                        Element_SetXY(&b_keyb, 192, 470 - p_buttons);
                        Element_SetXY(&b_snap, 12 , 470 - p_buttons);
                    }

                    Element_Paint(&b_snap_save);
                    Element_Paint(&b_snap_load);

                    snap_state = __checkEmuButton();

                    if(snap_state == STATE_SNAP_SAVE)
                    {
                        doSnapshot (current_dev, globalRom.filename, SNAP_SAVE);
                        p_buttons = 0; 
                        mstate = -1;
                    }

                    if(snap_state == STATE_SNAP_LOAD)
                    {
                        doSnapshot (current_dev, globalRom.filename, SNAP_LOAD);
                        p_buttons = 0; 
                        mstate = -1;
                    }

                    break;

                case STATE_KEYB:

                    if (!screen_y)
                        Element_SelectImg( &cursor, 2 );
    
                    if (screen_y <  55)
                    {
                        screen_y+= CONST_SCREEN_SUM;
                        screen_x+= (1.2 * CONST_SCREEN_SUM);
                        cpc_zoom -= (0.00258 * CONST_SCREEN_SUM);
                    }
    
                    KeyboardShow();

                    break;

                case STATE_GUN:
                    spool = 1;
                    sprintf(spool_cad, " GUSTICK MODE: USE HOME TO EXIT...");
                    mstate = -1;
                    p_buttons = 0;

                    WiiStatus.Gunstick = 1;
                    WiimoteSetupGun(1);

                    break;

                case STATE_FLOPPY:
                    spool = 1;
                    sprintf(spool_cad, " CHANGE-DISK OPT - NOT READY YET...");
                    mstate = -1;

                default:
                    Element_Paint(&b_floppy);
                    Element_Paint(&b_gun);
                    Element_Paint(&b_keyb);
                    Element_Paint(&b_snap);

                    mstate = __checkEmuButton();
                    break;

            }

            Element_Paint(&cursor);

        }
        else
        {
            if(p_buttons)
            {
                p_buttons = 0;
                mstate = -1;
            }

            if (screen_y > 0)
            {
                screen_y-= CONST_SCREEN_SUM;
                screen_x-= (1.2 * CONST_SCREEN_SUM);
                cpc_zoom += (0.00258 * CONST_SCREEN_SUM);
            }

        }
   
        //sprintf(debugt,"DEBUG: z(%f) m(%i) pb(%i) wPon(%i)", cpc_zoom, mstate, p_buttons, WpadOnScreen);
        //PrintW (100, 450, debugt);

    }
}

/* * * ANIM THREADS * * */

int dir;
bool animthread_run = false;

int menuthread_callanimButtons( int btn, sMenuEntry * current )
{

    if(animthread_run == true)
        return -1;

    switch(btn)
    {
        case 1:  dir = 32; break;
        case 0:  dir = -32; break;
    }


    int rc = LWP_CreateThread(&animthreads[0], menuthread_animButtons, (void*) current, NULL, 0, 10);

    if(rc!=0)
    {
        printf("ERROR; return code from LWP_CreateThread is %d\n", rc);
    }

    animthread_run = true;

    return rc;

}

void *menuthread_animButtons(void * arg)
{
    int position = 0;
    int n;

    sMenuEntry * current = (sMenuEntry *) arg;

    while((position > -(ANIM_NEXT_ROWS)) && (position < (ANIM_NEXT_ROWS)))
    {
        position += dir;

        for(n = 0; n < current->nbuttons ; n++)
        {
            current->buttons[n].px += dir;
            Button_SetXY(&renderBtn[n], current->buttons[n].px, current->buttons[n].py);
        }

        GRRLIB_VSync();

    }

    // UPDATE BOTONES
    if(dir > 0) //<<<<
    {
        glistposition -= ROWS_PER_SCREEN;

        if(glistposition <= 0 ) //end izq
        {
            animthread_run = false;
            return 0;
        }

        if(glistposition + (ROWS_PER_SCREEN*3) > WiiStatus.nRoms ) //end der
        {
            animthread_run = false;
            return 0;
        }

        current_gamelist = &gamelist; //reload gamelist
        for (n = 0; (n < (glistposition - ROWS_PER_SCREEN)) && (current_gamelist->fnext != NULL); n++)
            current_gamelist = current_gamelist->fnext;

    }
    else //>>>>
    {
        glistposition += ROWS_PER_SCREEN;

        if(glistposition - (ROWS_PER_SCREEN) <= 0 ) //end izq
        {
            animthread_run = false;
            return 0;
        }

        if(glistposition + (ROWS_PER_SCREEN*2) > WiiStatus.nRoms ) //end der
        {
            animthread_run = false;
            return 0;
        }

        for (n = 0; (n < ROWS_PER_SCREEN) && (current_gamelist->fnext != NULL); n++)
            current_gamelist = current_gamelist->fnext;
    }

    Menu_UpdateButtonsLabels(0);  //actualizo el centro (off screen)


    //recoloco los botones para la siguiente animacion
    position = -ANIM_NEXT_ROWS;
    for(n = 0; n < current->nbuttons ; )
    {
        current->buttons[n].px = (640 / 2) + position;
        Button_SetXY(&renderBtn[n], current->buttons[n].px, current->buttons[n].py);

        n++;

        if((n % ROWS_PER_SCREEN) == 0)
            position += ANIM_NEXT_ROWS;

    }

    Menu_UpdateButtonsLabels(-1);  //actualizo el izq (off screen)
    Menu_UpdateButtonsLabels(1);   //actualizo el der (off screen)
    Menu_UpdateButtonsLabels(2);   //actualizo el der (off screen)


    animthread_run = false;

    return 0;
}


