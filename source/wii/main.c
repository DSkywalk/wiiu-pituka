/*! \file main.c
 *  \brief Main
 *         Wii Library.
 *
 *
 *  Wii/GC Main Program
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
#include <fat.h>
#include <wiiuse/wpad.h>
#include <ogc/lwp_watchdog.h>
#include <asndlib.h>

#include "grrlib/GRRLIB.h"
#include "../global.h"

#include "keybwii.h"
#include "motewii.h"
#include "audiowii.h"
#include "videowii.h"
#include "menu/menu.h"
#include "menu/explorer.h"
#include "tcp/net.h"
#include "wiixml.h"

#include "sound/snd_defines.h"

#include "../port/dskutils.h"

/*** defines ***/
typedef unsigned char byte;

#define DEFAULT_FIFO_SIZE    (256*1024)

/*** 2D Video Globals ***/
void *pix = NULL;


extern Bitu8 *pbSndBuffer;
extern Bitu8 *pbSndBufferEnd;
extern Bitu8 *pbSndStream;


/* FUNCIONES */
int init_buffer (void);
void close_buffer (void);

/* * GLOBALES * */
xmlWiiCFG WiitukaXML;
extern t_fslist gamelist;
extern char current_dev[8 + 1];

char extension[5];
char file_state[13];

t_WiiRom globalRom = {"", SU_NONE};

char spool_cad[256]={'\0'};
unsigned char spool=0;
unsigned char spool_act=0;
unsigned char reiniciado;
Pituka_SpoolKeys spoolkeys={ {'\0'}, NULL, 0, false };


bool sonido_ant;

/* CPC */
extern Bitu8 keyboard_matrix[16];
extern Bitu8 bit_values[8];
extern Bitu8 keyboard_translation[320];

/* WII EXTERN */
extern bool WpadOnScreen;
extern float screen_x;
extern float screen_y;
extern float cpc_zoom;

/* WII GLOBALES */
int menuLoaded = MENU_NULL;

WiituKa_Status WiiStatus = {0,0,0,0,0,0,0,0,0,0};
Wii_gun gunstick = {0,0,0,0};

extern byte *pbGPBuffer;

/* MAIN FUNCTIONS */
void Wiituka_InitCFG(void);
void Wiituka_LoadCFG(char * device);
void Wiituka_SaveCFG(char * device);
void Wiituka_LoadNetUpdate(void);

int wii_input (void){

    GRRLIB_VSync ();

    WPAD_ScanPads();
    if (WPAD_ButtonsDown(0) & WPAD_BUTTON_A) return 1;
    else return 0;
}


int poll_input (void){

    poll_keyboard();
    poll_wiimote();
    poll_pads();

    return 0;
}

bool BuffersInit(void)
{
    pbGPBuffer = (byte*) malloc(128*1024); //need by unzip lib

    if (pbGPBuffer == NULL)
        return false;

    return true;
}

void DevicesInit(void)
{
    bool mounted = false;
    //printf(" ."); 
    
    //check if USB is available
    if(Explorer_isUSB()) {
        mounted = true;
        strcpy(current_dev, "usb:");
    } else if(Explorer_isSDCARD()) {
        mounted = true;
        strcpy(current_dev, "sd:");
    }
    
    if(!mounted)
    {
        //printf("\n MAIN: Unable to initialise FAT subsystem.  Are there any connected devices?\n  I'll continue without fat support..."); 
        WiiStatus.Dev_Fat = 0;
        net_init_thread();
    } else {
        WiiStatus.Dev_Fat = 1;
        Wiituka_LoadCFG(current_dev);
        //printf("."); 

        //INIT DHCP WII LAN
        if(!WiitukaXML.disablenet)
            net_init_thread();
        else
            WiiStatus.Dev_Net = 0;
    }

    srand(GetTicks());
    //printf(" .\n"); 

}


void doPowerOff()
{

    doCleanUp();

    free(pbGPBuffer);

    VideoClose();
    Explorer_Unmount();
    WPAD_Shutdown();

    if (!!*(u32*)0x80001800)
        exit(1);
    else
        SYS_ResetSystem(SYS_RETURNTOMENU, 0, 0);

  //TODO: AÑADIR UNA OPCION DE APAGADO.

}

void PlayerInit (void)
{
    // Splash Music!!
    ASND_Init(NULL);
    //MP3Player_Init();
    //MP3Player_PlayBuffer(intro_mp3, intro_mp3_size, NULL);
}


/******************************************
PITUKA MAIN
*******************************************/

int main(int argc, char *argv[]) {

    if(!VideoInit())
        return 1;

    if(!BuffersInit())
        return 2;

    Wiituka_InitCFG();

    KeyboardInit();

    WiimoteInit();

    DevicesInit();

    PlayerInit();

    ShowSplash ();

    MenuInit();

    cpc_main();

    //save rom cache/prefs...
    if(WiiStatus.Dev_Fat)
    {
        if(!Explorer_XMLsave (current_dev, &gamelist))
            printf(" DEBUG: Error al Guardar CACHE_ROMS!\n" );

        //save config...
        if(WiiStatus.SaveXML)
            Wiituka_SaveCFG(current_dev);
    }

    MenuClose();
    doPowerOff();
 
    return 0;
}

unsigned int GetTicks(void)
{
    const u64 ticks    = gettime();
    const u64 ms    = ticks / TB_TIMER_CLOCK;
    return ms;
}

void main_process_pause(int val) {

    switch (val)
    {
        case 1: 
            WiimoteSetupGun(0);
            break;
        case 0:
            WiimoteSetupGun(1);
            break;
   }

   WpadOnScreen = false;
   screen_x = screen_y = 0;
   cpc_zoom = CONST_ZOOM_RATIO;

   if(WiiStatus.SaveKEY)
   {
        WiimoteSaveKeys();
        WiiStatus.SaveKEY = 0;
   }
}

extern char debugt [1024];
int strFind (char * cad, char token, bool first);

int load_rom(t_WiiRom * romfs)
{

    int fileSize = 0;
    void * fbuffer = NULL;
    FILE *pfile;
    char path[1024 + 1] = "";

    bool result = true;
    int state = 0;
    int init_state = -1;

    WiimoteLoadKeys(); //load from gamelist struct

    //switch para decidir segun sea fat3:// http:// ftp:// fatx:// 
    switch (romfs->location)
    {
        case SU_SD:
            strcpy(path, current_dev); 

            strcat(path, CPC_ROMSDIR);
            strcat(path, "/");
            strcat(path, romfs->filename);

            //si es un zip primero descomprime en memoria...
            if(!(pfile = fopen(path, "rb")))
            {
                spool = 1;
                sprintf(spool_cad, " LOAD ROM: ERROR (%s)", romfs->filename);
                return false;
            }

            fseek(pfile, 0, SEEK_END);
            fileSize = ftell(pfile);
            rewind(pfile);

            fbuffer = (void *) malloc(fileSize); 

            if(fbuffer == NULL)
            {
                spool = 1;
                sprintf(spool_cad, " LOAD ROM: NO MEMORY (%i)", fileSize);
                fclose(pfile);
                return false;
            }

            if(!fread(fbuffer, 1, fileSize, pfile))
            {
                free(fbuffer);
                fclose(pfile);
                spool = 1;
                sprintf(spool_cad, " LOAD ROM: NO READED? (%s)", romfs->filename);
                return false;
            }

            fclose(pfile);

            break;

        case SU_HTTP:
            strcpy(path, WiitukaXML.urlpath);
            strcat(path, WIIDEFAULT_NETROMS);
            strcat(path, "/");
            strcat(path, romfs->filename);


            init_state = net_start_thread(path, TCP_REQUEST2, 0); //binary download
            result = false;
            while( !result )
            {
                state = net_get_state();
                sprintf(debugt, " DOWNLOAD ROM-NET: Init state(%i), Last state(%i)", init_state, state);
                ShowWait();

                if(state == 6)
                {
                    result = true;
                    fileSize = net_get_buffersize();
                    if(fileSize > 0)
                    {
                        fbuffer = (void *) malloc(fileSize); 
                        if(fbuffer != NULL)
                        {
                            memcpy(fbuffer, net_get_filebuffer(), fileSize);
                        }
                        else
                        {
                            spool = 1;
                            sprintf(spool_cad, " LOAD ROM-NET: NO MEMORY, Size(%i)", fileSize);
                        }
                
                    }
                    else
                    {
                        spool = 1;
                        sprintf(spool_cad, " LOAD ROM-NET: BAD DOWNLOAD? State(%i)", state);
                    }
                }else if(state == 3) //cancel by thread...
                    result = true;
            }

            net_stop_thread();

            break;

        default:
            spool = 1;
            sprintf(spool_cad, " LOAD ROM: ERROR PATH (%s)", path);
            return false;
    }

    if(!(result = loadBuffered_rom (fbuffer, fileSize))){
        spool = 1;
        sprintf(spool_cad, " LOAD BUFFER ROM: BAD? (%s)", romfs->filename);
    }
 
    free(fbuffer);

    return result;
}

int strFind (char * cad, char token, bool first)
{
    int tokenAt = 0;
    int n;

    for ( n = 0; cad[n] != '\0'; n++ )
    {
        if( cad[n] == token )
        {
            if(first)
                return n;

            tokenAt = n;
        }
    }

    return tokenAt;
}

void Wiituka_InitCFG(void)
{
    WiitukaXML.xmlversion = DEFAULT_XMLVER;
    WiitukaXML.xmlgameversion = DEFAULT_XMLGAMELIST;
    strcpy(WiitukaXML.urlpath, WIIDEFAULT_URL);
    WiitukaXML.scrtube = 0;
    WiitukaXML.scrintensity = 100;
    WiitukaXML.cpcspeed = 4;
    WiitukaXML.cpcfps = 0;
    WiitukaXML.lastrom = 0;
    WiitukaXML.disablenet = 0;

    WiiStatus.Dev_Fat = -1;
    WiiStatus.Dev_Keyb = 0; 
    WiiStatus.Dev_Net = -1;
    WiiStatus.CurrentMenu = -1;
    WiiStatus.LoadDISK = 0;
    WiiStatus.LoadSNAP = 0;
    WiiStatus.LoadTAPE = 0;
    WiiStatus.SaveOPT = 0;
    WiiStatus.SaveXML = 0;
    WiiStatus.SaveKEY = 0;

    WiiStatus.nWiimotes = 0;
    WiiStatus.padsConnected = 0;
    WiiStatus.Gunstick = 0;
    WiiStatus.nRoms = -1;
    WiiStatus.UpdateDIR = 1;
    WiiStatus.UpdateXMLNET = 0;

    WiiStatus.VersionXMLNET = 0;

}

void Wiituka_LoadCFG(char * device)
{
    char temp[1024];

    sprintf(temp,"%s%s/wiituka_conf.xml", device, CPC_FILEDIR);

    if(!XML_loadPublic(temp)){
        WiiStatus.SaveXML = 1;
    }


}
void Wiituka_SaveCFG(char * device)
{

    char temp[1024];

    sprintf(temp,"%s%s/wiituka_conf.xml", device, CPC_FILEDIR);
   
    //printf(" Saving Config XML...\n");
    if(!XML_savePublic(temp))
        printf(" MAIN-SaveCFG: ERROR ON XML SAVE !\n");

}


void Wiituka_LoadNetUpdate(void)
{
    char temp[1024];
    char * cad = NULL;
    int fileSize = 0;

    int result = 0;
    int state = 0;
    //int init_state = -3;

    sprintf(temp,"%s%s/wiituka_updates.xml", WiitukaXML.urlpath,  WIIDEFAULT_NETDATA);

    //printf(" Checking for Updates: ");

    //init_state = net_start_thread(temp, TCP_REQUEST1, 1); //char download
    net_start_thread(temp, TCP_REQUEST1, 1); //char download
    result = false;
    while( (!wii_input()) && !result )
    {
        state = net_get_state();

        if(state == 6)
        {
            result = true;
            fileSize = net_get_buffersize();
            if(fileSize > 0)
            {
                cad = (void *) malloc(fileSize + 1); 
                if(cad != NULL)
                {
                    strncpy(cad, (char*) net_get_charbuffer(), fileSize);
                }
                else
                {
                    sprintf(debugt," UPDATE.NET: Downoad No MEM, Size(%i)", fileSize);
                }
            }
            else
                   sprintf(debugt," UPDATE.NET: BAD DOWNLOAD? State(%i)", state);
        }else if(state == 3)
             result = true;

        GRRLIB_VSync (); //need it by thread
    }

    net_stop_thread();

    result = 0; //init
    if(cad != NULL)
        result = XML_checkUpdates(cad);

    switch(result)
    {
        case  1:
            //printf("There is a new Wiituka Build!!\n");
            //TODO: download & unlink stuff
            updateDol(SU_SD);
            //printf(" MAIN: Wiituka Updated.\n  Please restart this application...\n"); 
            break;

        case  2:
            //printf("New Romlist!\n");
            WiiStatus.UpdateXMLNET = 1;
            break;

        case 0: //do nothing ;)
            //printf("Done.\n");
            break;

    }

}


