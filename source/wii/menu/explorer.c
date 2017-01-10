/*! \file explorer.c
 *  \brief Explorador de ficheros.
 *         Wii Library.
 *
 *  \version 0.6
 *
 *  Wii/GC Filesystem
 *
 *   Pituka - Nintendo Wii/Gamecube Port
 *  (c) Copyright 2008-2009 David Colmenero (aka D_Skywalk)
 */

#include <gccore.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <ctype.h>
#include <fat.h>
#include <sdcard/wiisd_io.h>

#include <sys/iosupport.h>
#include <sys/dir.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
    
#include <gctypes.h>
#include "../videowii.h"
#include "../grrlib/GRRLIB.h"
#include "../wiixml.h"
#include "../tcp/net.h"
#include "elements.h"

#include "../../global.h"
#include "../../port/dskutils.h"

#include "explorer.h"

extern char debugt [1024];
extern Pituka_SpoolKeys spoolkeys;
extern unsigned char spool;
extern unsigned char reiniciado;

extern WiituKa_Status WiiStatus;
extern xmlWiiCFG WiitukaXML;

extern t_fslist gamelist;
extern const unsigned char buttons_def[MAX_CPCBUTTONS * 2];

char current_path [1024 + 1] = "";
char current_dev  [8 + 1] = "";

void _nfoRead(t_infnode * ginfo, char * filename);
void _nfoParse(t_infnode * ginfo, char * buffer, int bsize);

void _btnCreate (t_button * button, t_infnode * ginfo);

/* PRIVATE XMLWII FUNCTIONS */
bool XML_loadGameList(char * xmldata, t_fslist * gamelist);
bool XML_saveGameList(char * filename, t_fslist * gamelist);

void FileList_rClean (t_fslist * filenames)
{
   if(filenames->fnext != NULL){
      FileList_rClean(filenames->fnext);

      free(filenames->fnext);
      filenames->fnext = NULL;
   }

}

void FileList_Init (t_fslist * filenode){

        memset (&filenode->game, 0, sizeof(t_infnode));
    filenode->gfile.filename[0] = 0;
        filenode->gfile.location = SU_NONE;
    filenode->favorite = 0;
        //memset (filenode->binds, 0xff, MAX_CPCBUTTONS * 2);
        memcpy(filenode->binds, buttons_def, MAX_CPCBUTTONS * 2);

    filenode->fnext = NULL;

}

void _FileList_Swap(t_fslist * pre, t_fslist * current)
{

   t_fslist temp;
   FileList_Init(&temp); 

   memcpy(&temp, pre, sizeof(t_fslist));
   memcpy(pre, current, sizeof(t_fslist));

   //pointers
   pre->fnext = temp.fnext;
   temp.fnext = current->fnext;

   memcpy(current, &temp, sizeof(t_fslist));

}

void FileList_OrderbyName(t_fslist * filenames)
{
   t_fslist * pre = filenames;
   t_fslist * current = filenames->fnext;

   int result = 0;
   bool moved = false;    

   while( 1 )
   {

     while ( current->fnext != NULL)
     {
    result = strncmp(pre->game.title, current->game.title, 128);

    if(result == 0)
        { //delete!
        if(memcmp(&pre->gfile, &current->gfile, sizeof(t_WiiRom)) == 0)
        {
            pre->fnext = current->fnext;
            free(current);
            current = pre->fnext;
        }
        
    } 
    else if(result > 0)
    { //move!
        _FileList_Swap(pre, current);
        moved = true;
    }

    current = current->fnext;
    pre = pre->fnext;

     }

     if(moved == true){ //re-init
    moved = false;
        pre = filenames;
        current = filenames->fnext;
     }else //no changes -> end!
    break;
  }

}

void Explorer_rebuildRoms (const t_fslist * filenames)
{
   t_fslist * nnode = (t_fslist *) filenames;
   t_fslist * tmpnode = NULL;

   WiiStatus.nRoms = 0;

   while(nnode->fnext != NULL){ //cuenta numero de roms

    if(nnode->gfile.location == 0)
    {
        tmpnode = nnode->fnext;

        if(tmpnode->fnext == NULL){
            FileList_Init(nnode);
            free(tmpnode);
            return;
        }else{
            memcpy(nnode, tmpnode, sizeof(t_fslist));
            free(tmpnode);
        }
        
    }

    nnode = nnode->fnext;

        //TODO: AÑADIR SOLO ROM+ SI location != 0 :)
          WiiStatus.nRoms++;
   }

}


bool Explorer_XMLread (char * device, t_fslist * filenames)
{
    t_fslist * nnode= filenames;

    char temp[1024]= "";
    t_filebuf file;

    if(!WiiStatus.Dev_Fat)
        return false;

    strcpy(temp, device); 
    strcat(temp, CPC_FILEDIR);
    strcat(temp, "/wiituka_romcache.xml");

    if(!fileRead (&file, temp )){ //carga el XML en memoria
        return false;
    }

    if(nnode->fnext != NULL) //comprueba que la lista esta vacia :-
        FileList_rClean(filenames);


    if(!XML_loadGameList((char *) file.buffer, nnode))
           return false;

    return true;
}


bool Explorer_XMLsave (char * device, t_fslist * filenames)
{
    char temp[1024]= "";

    strcpy(temp, device);
    strcat(temp, CPC_FILEDIR);
    strcat(temp, "/wiituka_romcache.xml");

    if(!XML_saveGameList(temp, filenames))
       return false;

    return true;

}

bool _FileList_UpdateFile(const char * filename, enum support_type nloc )
{
    t_fslist * pnode = &gamelist;

    while ( pnode->fnext != NULL)
    {

        if( (strncmp(filename, pnode->gfile.filename, 256) == 0) )
    {
        pnode->gfile.location = nloc;
            return true;
    }
    
    pnode = pnode->fnext;

    }

    return false;
}

/*
 * Inserta el juego en gamelist ordenado segun nombre.
 */
bool _FileList_InsertGame (const t_fslist * nfile)
{
    t_fslist * pnode = &gamelist;
    int result;

    while ( pnode->fnext != NULL)
    {
        if( (strncmp(nfile->gfile.filename, pnode->gfile.filename, 256) != 0) )
        {
        result = strncmp(nfile->game.title, pnode->game.title, 128);

        if(result == 0){
                if ((nfile->gfile.location == SU_SD) 
                && ((pnode->gfile.location == SU_NONE) || (pnode->gfile.location == SU_HTTP) ))
            {
                memcpy(&pnode->gfile, &nfile->gfile, sizeof(t_WiiRom));
                return true;
            }
            break;
        }
        
        if(result < 0)
            break;

    }else if(nfile->gfile.location == SU_SD)
    {
         if((pnode->gfile.location == SU_NONE) || (pnode->gfile.location == SU_HTTP) )
            pnode->gfile.location = SU_SD;
        return true;
    }

    pnode = pnode->fnext;

    }

    t_fslist * newnode = NULL;

    //creamos
    newnode = malloc(sizeof(t_fslist));
    if(newnode == NULL)
    return false;

    if( pnode->fnext != NULL)
    {      //copiamos los datos
        memcpy(newnode, pnode, sizeof(t_fslist));
    }else
        FileList_Init (newnode);

    //copiamos los datos del nuevo
    memcpy(pnode, nfile, sizeof(t_fslist));

    pnode->fnext = newnode;

    return true;
}


bool Explorer_XMLNETread( void )
{
    t_fslist tmp_list = { 
            {"", SU_NONE}, 
            { "", "", "", "", ""}, 0, 
            { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
              0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff  },
            NULL };

    t_fslist * nnode= &tmp_list;

    bool result = true;
    int state = 0;
    int init_state = -1;
    int fileSize = 0;
    char * cad = NULL;
    char path[1024]="";
    int ngames = 0;


    if(!WiiStatus.Dev_Net)
    return false;

    strcpy(path, WiitukaXML.urlpath);
    strcat(path, WIIDEFAULT_NETDATA);
    strcat(path, "/wiituka_romlist.xml");

    //printf(" Checking for Updates at: %s\n", path);

    init_state = net_start_thread(path, TCP_REQUEST1, 1); //char download
    result = false;

    while( !result )
    {
        state = net_get_state();
        sprintf(debugt, " XMLNET: Init state(%i), Last state(%i)", init_state, state);
        if(state == 6)
        {
            result = true;
            fileSize = net_get_buffersize();
               if(fileSize > 0) {
                cad = (void *) malloc(fileSize + 1); 
                if(cad != NULL)
                    strncpy(cad, (char*) net_get_charbuffer(), fileSize);
                else
                    sprintf(debugt,"XMLNET: Downoad No MEM, Size(%i)", fileSize);
            } else
                   sprintf(debugt,"XMLNET: BAD DOWNLOAD? State(%i)", state);
            

        } else if(state == 3) { //CANCEL
            net_stop_thread();
            return false;
        }
        GRRLIB_VSync (); //need it by thread

    }

    net_stop_thread();

    if(!XML_loadGameList(cad, nnode))
        return false;

    while(nnode->fnext != NULL)
    {
        if(!_FileList_UpdateFile(nnode->gfile.filename, SU_HTTP))
        {
            if(_FileList_InsertGame(nnode))
                ngames++; //to use in debug mode

        }
        nnode = nnode->fnext;
    }

  return true;
}

bool Explorer_dirRead ( void )
{
    int ngames = 0;

    DIR *pdir;
    struct stat statbuf;
    struct dirent *pent;
    t_fslist pnode;

    if(!WiiStatus.Dev_Fat){
        return false;
    }

    pdir=opendir(current_path);

    if (!pdir) {
           printf ("opendir() failure; terminating\n");
           
        return false;
    }

    //while(dirnext(pdir,filename,&statbuf) == 0) {
    while ((pent = readdir(pdir)) != NULL) {
        stat(pent->d_name,&statbuf);
        
        if(S_ISDIR(statbuf.st_mode))
               continue;

        if(strlen(current_path) + strlen(pent->d_name) >= 1024)
            continue; //path demasiado largo, saltamos

        lowercase (pent->d_name, false);

        if ( memcmp((char*) &pent->d_name[(strlen(pent->d_name)-4)], ".zip", 4) != 0) 
            continue;

        if(!_FileList_UpdateFile(pent->d_name, SU_SD)) {
            FileList_Init(&pnode); //cleaning node

            _nfoRead(&pnode.game, pent->d_name);
            strcpy(pnode.gfile.filename, pent->d_name);
            pnode.gfile.location = SU_SD;

            if(_FileList_InsertGame(&pnode)) // TODO: HACER FLUSH PARA EVITAR CUELGUES? 
                ngames++; //to use in debug mode

        }
    }

    closedir(pdir);

    return true;

}

void _nfoRead(t_infnode * ginfo, char * filename) //añadir modo online
{
   char path[1024]= "";
   t_filebuf file;
   t_zip_info zinfo;
   int iErrorCode = 0;

   strncpy(ginfo->title, filename, 127); //minimo

   strcpy(path, current_path); 
   strcat(path, filename);

   if(!fileRead (&file, path ))
     return;

   zinfo.pchFileNames = NULL;
   zinfo.pchZipFile = NULL;
   zinfo.pchExtension = ".diz";



   if ((iErrorCode = zipBuffered_dir(file.buffer, file.size, &zinfo))){
       if(iErrorCode != 15)
             sprintf(debugt,"Zdir: Err: %i (%s-%i)" ,iErrorCode, filename, file.size);

       free(file.buffer);
       return;
   }

   if (zinfo.unZipSize > 0){

       unsigned char * ebuffer = NULL; 
       ebuffer = malloc (zinfo.unZipSize); //Using Zip Info for extract

       if(ebuffer == NULL){
           free(file.buffer);
       return;
       }

       if (!(iErrorCode = zipBuffered_extract( ebuffer, file.buffer, zinfo.dwOffset))) {
           _nfoParse(ginfo, (char *) ebuffer, zinfo.unZipSize);
       }

       free(ebuffer);
   }

   free(file.buffer);

   if (zinfo.pchFileNames) {
      free(zinfo.pchFileNames);
   }

  // sprintf(debugt,"t%sg%sc%sl%sy%s",ginfo->title, ginfo->genre, ginfo->company, ginfo->lang, ginfo->year);

}


void _nfoParse(t_infnode * ginfo, char * buffer, int bsize)
{
  int ps, n, cp;
  int nSize = 0;
  int cMax = 0;
  char * cbuffer = NULL;

  char ltoken[5][10] = {"TITLE:", "TYPE:", "COMPANY:", "LANGUAGE:", "YEAR:" };

  for(n = 0; n < 5; n++){

    nSize = strlen(ltoken[n]);
    cMax = 0;
    cbuffer = NULL;
   

    switch (n)
    {
       case 0: cbuffer = ginfo->title; cMax = 128; break;
       case 1: cbuffer = ginfo->genre; cMax = 128; break;
       case 2: cbuffer = ginfo->company; cMax = 28; break;
       case 3: cbuffer = ginfo->lang; cMax = 28; break;
       case 4: cbuffer = ginfo->year; cMax = 5; break;
    }

    for(ps = 0; ps < (bsize - nSize); ps ++)
    {
       if(memcmp(&buffer[ps], ltoken[n], nSize)!=0)
          continue; //no son iguales +1

       ps+=nSize;

       while((ps < bsize) && (buffer[ps] == ' ')) ps++;

       for(cp = 0; (cp < cMax) && (ps < bsize) && (buffer[ps] != 0x0d); ps++)
       {
          if(buffer[ps]!='"')
          {
            cbuffer[cp] = buffer[ps];
            cp++;
          }
            
       }
       cbuffer[cp] = '\0';
    }
    
    lowercase(cbuffer, true); //Primera mayus.

  }

}

bool Explorer_LoadFilelist(char * path)
{
    int error = 0;
    snprintf(current_path, 1024, "%s%s%s", current_dev, path, "/");

    if(gamelist.fnext != NULL)
        FileList_rClean(&gamelist);

    if(!Explorer_XMLread("sd:", &gamelist)) //lee la cache XML
        error++;
    
    /*
    if(WiiStatus.UpdateXMLNET) {
        if(!Explorer_XMLNETread()) {
            sprintf(debugt,"DEBUG: Exploracion Online erronea!" );
            error++;
        }else{
            WiiStatus.UpdateDIR = 1;
            WiiStatus.SaveXML = 1;
            WiitukaXML.xmlgameversion = WiiStatus.VersionXMLNET;
        }

        WiiStatus.UpdateXMLNET = 0;

    }
    */

    if(WiiStatus.UpdateDIR) {
        if(!Explorer_dirRead()) {
              sprintf(debugt,"DEBUG: Exploracion erronea!" );
            error++;
        }
        WiiStatus.UpdateDIR = 0;
    }

    Explorer_rebuildRoms((const t_fslist *) &gamelist);

    if(error > 2)
        return false;

    return true;

}

/********** FILESYSTEM ************/

void Explorer_Unmount(void) {
    fatUnmount(current_dev);
    WiiStatus.Dev_Fat = 0;
}

const DISC_INTERFACE* sd = &__io_wiisd;
bool _createDirs (char *device);
bool Explorer_isSDCARD (void) {
    DIR *pdir;
    if( (!sd->startup()) || (!fatMountSimple("sd", sd)) )
        return false;
    
    pdir=opendir("sd:" CPC_FILEDIR);

    if (!pdir) {
        if(!_createDirs("sd:")) {
            fatUnmount("sd:");
            printf(" ERROR NO Dirs Created! - Wiituka NETMODE\n  Please, reload Wiituka and check your sd...\n");
            usleep(50000);
            return false;
        }
        
        pdir=opendir("sd:" CPC_FILEDIR);
        if (!pdir) {
            fatUnmount("sd:");
            return false;
        }
    }

    return true;
}

const DISC_INTERFACE* usb = &__io_usbstorage;
bool Explorer_isUSB(void) {
    DIR *pdir;
    if( (!usb->startup()) || (!fatMountSimple("usb", usb)) )
        return false;
    
    pdir=opendir("usb:" CPC_FILEDIR);

    if (!pdir) {
        fatUnmount("usb:");
        return false;
    } else {
        // just try to create if /WIITUKA/ path exists
        if(!_createDirs("usb:")) {
            fatUnmount("usb:");
            return false;
        }
    }

    return true;
}


bool _createDirs (char *device) {
    char temp[FSDIRMAX + 1];
    bool any_created = false;

    snprintf(temp, FSDIRMAX, "%s%s", device, "/");
    DIR* pdir = opendir(temp);

    if (pdir == NULL) {
        printf("CreateDirs: Cannot Open: %s\n", temp);
        return false;
    }

    closedir(pdir);

    //create CPC_FILEDIR?
    snprintf(temp, FSDIRMAX, "%s%s", device, CPC_FILEDIR);
    if (chdir(temp)){ //NOT FOUND?
        printf("CreateDirs: mkdir: %s\n", temp);
        if (mkdir(temp, S_IREAD | S_IWRITE) == -1) //CREATE IT!
            return false;
        else
            any_created = true;
    }

    //cd CPC_FILEDIR?
    if (!chdir(temp)) {
    
        //create CPCR00MS!?
        snprintf(temp, FSDIRMAX, "%s%s", device, CPC_ROMSDIR);
        if (chdir(temp))
        {
            printf("CreateDirs: mkdir: %s\n", temp);
            mkdir(temp, S_IREAD | S_IWRITE);
            any_created = true;
        }

        snprintf(temp, FSDIRMAX, "%s%s", device, CPC_SAVEDIR);

        if (chdir(temp))
        {
            printf("CreateDirs: mkdir: %s\n", temp);
            mkdir(temp, S_IREAD | S_IWRITE);
            any_created = true;
        }
    } else
        return false; //NOT CREATED?

    if(any_created){    
        printf(" Dirs Created OK!\n  Please, reload Wiituka and add some roms on %s...\n", device);
        usleep(50000);
    }
    
    return true;
}

