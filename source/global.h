/* pituka.h - PituKa Base
 *
 * Copyright (c) 2005 David Skywalker <dantoine@gmail.com>
 *
 * Only for education and learn
 * 
 *
 */ 

#include <sys/types.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#ifndef PITUKA_H
#define PITUKA_H



#define false	0
#define true	1

#ifndef NULL
 #define NULL   0
#endif 

#ifndef _MAX_PATH
 #ifdef _POSIX_PATH_MAX
 #define _MAX_PATH _POSIX_PATH_MAX
 #else
 #define _MAX_PATH 256
 #endif
#endif



typedef unsigned char Bitu8;
typedef unsigned short Bitu16;
typedef unsigned int Bitu32;
typedef signed char Bits8;
typedef signed short Bits16;
typedef signed int Bits32;

typedef struct {
  unsigned char contenido[256];
  unsigned char * cur;
  unsigned char cont;
  unsigned char comienzo;
} Pituka_SpoolKeys;

enum support_type {SU_NONE, SU_DVD, SU_USB, SU_SD, SU_HTTP, SU_FTP};

typedef struct{
  char filename[256];
  enum support_type location;
}t_WiiRom;


typedef struct{
   int xmlversion;
   int wiiversion;
   int xmlgameversion;
   char urlpath[255 + 1];
   int scrtube;
   int scrintensity;
   int cpcspeed;
   int cpcfps;
   int lastrom;
   int disablenet;
}xmlWiiCFG;

typedef struct{
  int nWiimotes;
  int padsConnected;
  int Gunstick;
  int nRoms;
  int Dev_Keyb;
  int Dev_Fat;
  int Dev_Net;
  int CurrentMenu;
  int LoadDISK;
  int LoadSNAP;
  int LoadTAPE;
  int SaveOPT;
  int SaveXML;
  int SaveKEY;
  int UpdateXMLNET;
  int UpdateDIR;
  int VersionXMLNET;
}WiituKa_Status;

typedef struct{
  int x, y;
  int w, h;
} Wii_Rect;

typedef struct{
  int x, y;
  int px, py;
} Wii_cursor;

typedef struct{
  int x, y;
  int state;
  unsigned int timer;
  unsigned int hcolor;
}Wii_gun;

typedef struct {
   unsigned int model;
   unsigned int jumpers;
   unsigned int ram_size;
   double speed;
   unsigned int limit_speed;
   unsigned int paused;
   unsigned int auto_pause;
   unsigned int keyboard_line;
   unsigned int tape_motor;
   unsigned int tape_play_button;
   unsigned int printer;
   unsigned int printer_port;
   unsigned int mf2;
   unsigned int joysticks;
   int cycle_count;

   unsigned int scr_fs_width;
   unsigned int scr_fs_height;
   unsigned int scr_fs_bpp;
   unsigned int scr_style;
   unsigned int scr_vsync;
   unsigned int scr_led;
   unsigned int scr_fps;
   unsigned int scr_tube;
   unsigned int scr_intensity;
   unsigned int scr_window;
   unsigned int scr_bpp;
   unsigned int scr_bps;
   unsigned int scr_line_offs;
   unsigned int *scr_base;
   unsigned char *scr_base2;
   unsigned short *scr_base3;
   unsigned int scr_offs;
   unsigned int scr_line;

   unsigned int snd_enabled;
   unsigned int snd_playback_rate;
   unsigned int snd_bits;
   unsigned int snd_stereo;
   unsigned int snd_volume;
   unsigned int snd_pp_device;
   unsigned int snd_buffersize;
   unsigned char *snd_bufferptr;
   union {
#ifndef GEKKO
      struct {
         unsigned int low;
         unsigned int high;
      };
      int64_t both;
#else
      struct {
         unsigned int high;
         unsigned int low;
      };
      int64_t both;
#endif
   } snd_cycle_count_init;

   unsigned int kbd_layout;

   unsigned int max_tracksize;
   char snap_path[_MAX_PATH + 1];
   char snap_file[_MAX_PATH + 1];
   int snap_zip;
   char drvA_path[_MAX_PATH + 1];
   char drvA_file[_MAX_PATH + 1];
   int drvA_zip;
   unsigned int drvA_format;
   char drvB_path[_MAX_PATH + 1];
   char drvB_file[_MAX_PATH + 1];
   int drvB_zip;
   unsigned int drvB_format;
   char tape_path[_MAX_PATH + 1];
   char tape_file[_MAX_PATH + 1];
   int tape_zip;
   char printer_file[_MAX_PATH + 1];
   char sdump_file[_MAX_PATH + 1];

   char rom_path[_MAX_PATH + 1];
   char rom_file[16][_MAX_PATH + 1];
   char rom_mf2[_MAX_PATH + 1];

   unsigned char control;
   unsigned char frameskip;
   int emu_border;

} t_CPC;


typedef struct {
   char *pchZipFile;
   char *pchExtension;
   char *pchFileNames;
   char *pchSelection;
   int iFiles;
   unsigned int unZipSize;
   unsigned int dwOffset;
} t_zip_info;

//prints a string
void print (unsigned int *pdwAddr, char *pchStr, int bolColour);

//utils
int zipBuffered_dir(const void * zipBuffer, const int bSize, t_zip_info *zi);
int zipBuffered_extract(void * extBuffer, const void * zipBuffer, unsigned int dwOffset);

void process_spoolkey(int spool_ticks);
void lowercase ( char * str, int ucFirst );

//wii input
int poll_input (void);
void ShowLed(int val);

void FillScreen( int Updated );
void UpdateScreen (void);

void AudioPlayer( void );
void SoundInit (void);
void SoundClose (void);
int SoundSetup (void);
void StopSound (int val);

void ShowMenu (int nMenu);
void ShowEmuCommon (void);
void CleanScreen (void);
unsigned int GetTicks (void);

//wii main
void main_process_pause (int val);
int load_rom(t_WiiRom * romfs);

//caprice main
int video_set_palette (void);
void cpc_main (void);
void emu_paused (int val);
void emulator_reset(int bolMF2Reset);
void doCleanUp (void);
int loadBuffered_rom (void * rbuffer, int bSize);

#ifdef GEKKO
  #define port_mktemp mkstemp
#else
  #define port_mktemp mkdtemp
#endif

#define MAX_CPCBUTTONS 24

/* MODOS */

#define HRES 272 * 2
#define WRES 384 * 2

#define CONST_ZOOM_RATIO 0.833 //1.667

//#define CPC_ROOTDIR "/APPS/WIITUKA"
#define CPC_FILEDIR "/WIITUKA"
#define CPC_ROMSDIR CPC_FILEDIR "/DISKS"
#define CPC_SAVEDIR CPC_FILEDIR "/SAVES"
#define CPC_SCFGDIR CPC_FILEDIR "/CFG"
#define CPC_SCRNDIR CPC_FILEDIR "/SCREENS"

#define WIIDEFAULT_URL "http://wiituka.dantoine.org"

#define WIIDEFAULT_NETDATA "/wiidata"
#define WIIDEFAULT_NETROMS "/wiiroms"

#endif
