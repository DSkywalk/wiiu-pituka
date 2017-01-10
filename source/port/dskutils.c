/****************************************************************************
 * Pituka - Nintendo Wii/Gamecube Port
 *
 * D_Skywalk Sep 2008
 *
 * dskutils.c
 *
 * libreria para manejar DSK
 ***************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <dirent.h>
#include "sys/dir.h"

#ifdef GEKKO
  typedef unsigned int bool;
#endif

#include "../global.h"
#include "dskutils.h"

#define DSK_TRACKMAX    102   // max amount that fits in a DSK header
#define DSK_SIDEMAX     2
#define DSK_SECTORMAX   29    // max amount that fits in a track header


extern char debugt [1024];
extern Pituka_SpoolKeys spoolkeys;
extern unsigned char spool;
extern unsigned char reiniciado;


extern char current_path [1024 + 1];
//extern char current_dev  [8 + 1];

bool Explorer_dskBufferedRead ( void * dskBuffer)
{
    unsigned char * Buffer = NULL;
    char chStr[40];

    char contenido[50][20];

    unsigned char tracks = 0, sides = 0, disco[256];

    //char dtipo='O';
    bool derror= false ;

    int n,x,i,j,temp;

    strcpy(contenido[0],"|cpm"); //ponemos |CPM en la posicion -> 0

    Buffer = dskBuffer;

    if (memcmp(Buffer, "EXTENDED", 8) == 0)  // extended DSK image?
    {
        //dtipo='X';
        tracks = *(Buffer + 0x30); sides=*(Buffer + 0x31);
            if (tracks > DSK_TRACKMAX) { // compare against upper limit
                tracks = DSK_TRACKMAX; // limit to maximum
            }
            if (sides > DSK_SIDEMAX) { // abort if more than maximum
                return false;
            }

        for(n = 0; n < (tracks * sides); n++) 
            disco[n] = *(Buffer + 0x34 + n); //asi completamos cada cara y sabemos el tamaño de cada una
    }
    else
    {
        //dtipo='A';
        tracks = *(Buffer + 0x30); sides=*(Buffer + 0x31);
            if (tracks > DSK_TRACKMAX) { // compare against upper limit
                tracks = DSK_TRACKMAX; // limit to maximum
            }
            if (sides > DSK_SIDEMAX) { // abort if more than maximum
                return false;
            }

        for(n = 0; n < (tracks * sides); n++) 
            disco[n] = *(Buffer + 0x33); //asi completamos cada cara y sabemos el tamaño de cada una
    }

    if((sides*tracks)>= 256)
        return false; //too long

    //la primera vez
    Buffer = dskBuffer + 0x100;
              
    for(n = 1, x = 0; x < (tracks * sides); x++){
        if(disco[x])
            Buffer += 0x100;
        for(temp = 0; temp < (disco[x] * 0x100) ; temp += 0x20){

            derror = false;

            //fread(Buffer, 0x20, 1, pfile);

            if( ((Buffer[0]==0x00)&&( (Buffer[9]==0xA0)||(Buffer[9]==0x20) )&&(Buffer[10]==0x20)&&(Buffer[11]==0x20)&&(Buffer[15]!=0x00))
                ||( (Buffer[0]==0x00)&&( ( ((Buffer[9]==0x42)||(Buffer[9]==0xC2)) && (Buffer[10]==0x41) )
                                       ||( ((Buffer[9]==0x42)||(Buffer[9]==0xC2)) && (Buffer[10]==0x49) ))
                                                 &&(Buffer[15]!=0x00) ) ) 
            { //Si tiene B o blanco y empieza por 00

                for(i=1,j=0;((Buffer[i]!=0x20)&&(i<9));i++,j++){
                    //sprintf(debugt,"DEBUG: DENTRO (j %i - i %i) X(%i)", j, i, x);
                    if( ((Buffer[i]>='A')&&(Buffer[i]<='Z')) || ((Buffer[i]>='0')&&(Buffer[i]<='9'))
                       ||(Buffer[i]=='-')||(Buffer[i]=='!')||(Buffer[i]=='{')||(Buffer[i]=='}')||(Buffer[i]=='\'')||(Buffer[i]=='&')
                       ||(Buffer[i]=='^')||(Buffer[i]=='@')||(Buffer[i]=='+')||(Buffer[i]=='#') )
                    {
                        chStr[j]=Buffer[i];
                    }
                    else
                    {
                        derror = true;
                        //sprintf(debugt,"DEBUG: Error (j %i - i %i) X(%i)", j, i, x);
                    }
                }

                if( derror )
                    continue;

                chStr[j]='.'; // "." de la extension

                while(Buffer[i]==0x20) i++; //saltamos los espacios

                if(Buffer[i]==0x42)
                { //ficheros BAS, BIN, B**
                    for(j++;((Buffer[i]!=0x00)&&(i<12));i++,j++) //mientras no sea fin de fila
                        chStr[j]=Buffer[i];

                    chStr[j]='\0';
                    strcpy(contenido[n],chStr);
                    if(strcmp(contenido[n],contenido[n-1])!=0) //no son iguales
                        n++;

                }
                else if((Buffer[i]==0xa0)||(Buffer[9]==0x20)) //ficheros que solo tienen un punto
                {
                    //fin de cadena
                    j++;
                    chStr[j]='\0';
                    strcpy(contenido[n],chStr);
                    if(strcmp(contenido[n],contenido[n-1])!=0) //no son iguales
                        n++;

                }else if(Buffer[i]==0xC2)
                {
                    Buffer[i]=0x42; //lo cambiamos a B 
                    for(j++;((Buffer[i]!=0x00)&&(i<12));i++,j++) //mientras no sea fin de fila
                        chStr[j]=Buffer[i];

                    chStr[j]='\0';
                    strcpy(contenido[n],chStr);
                    if(strcmp(contenido[n],contenido[n-1])!=0) //no son iguales
                        n++;
                }
            }
            Buffer += 0x20;
        }
    }

    spoolkeys.contenido[0] = 0;
    derror = true;
    switch(n) //si solo hay dos seleccionados
    {
        case 3:
        case 2:
            lowercase(contenido[n-1], false);
            strcpy( (char *) spoolkeys.contenido, (char *) "run|");
        case 1:
            derror = false;
            break;
        default:
            for(i = 1; i < n; i++){
                if(memcmp(".BAS", (char*)&contenido[i][(strlen(contenido[i])-4)], 4) == 0)
                {
                    strcpy( (char *) spoolkeys.contenido, (char *) "run|");
                    n = i + 1;
                    derror = false;
                }
            }

            sprintf(debugt,"DEBUG: L(%i) C(%s)", n, (char *) spoolkeys.contenido);
            break;
    }

    if(!derror)
    {
        lowercase(contenido[n-1], false);
        strcat( (char *) spoolkeys.contenido, (char *) contenido[n-1]);

        sprintf(debugt,"DEBUG: L(%i-%s) C(%s)", n, contenido[n-1], (char *) spoolkeys.contenido);

        spoolkeys.comienzo = 1;
        spool = 1;
        reiniciado = 1;
    }

    return true;
}


bool Explorer_dskRead ( char * dskname )
{
    char tmppath[50] = "\0";
    unsigned char Buffer[256] = "\0";
    char chStr[40];

    char contenido[50][20];

    unsigned char tracks = 0, sides = 0, disco[50];
    FILE *pfile;

    //char dtipo='O';
    bool derror= false ;

    int n,x,i,j,temp;

    strcpy(tmppath, current_path); //usar default path (se cambia desde dirRead)
    strcat(tmppath, /* game->fname*/ dskname );
    strcpy(contenido[0],"|cpm"); //ponemos |CPM en la posicion -> 0

    if ((pfile = fopen(tmppath, "rb")) == NULL) 
        return false;

    fread(Buffer, 0x100, 1, pfile); // read DSK header

    if (memcmp(Buffer, "EXTENDED", 8) == 0)  // extended DSK image?
    {
        //dtipo='X';
        tracks = *(Buffer + 0x30); sides=*(Buffer + 0x31);

        for(n = 0; n < (tracks * sides); n++) 
            disco[n] = *(Buffer + 0x34 + n); //asi completamos cada cara y sabemos el tamaño de cada una
    }
    else
    {
        //dtipo='A';
        tracks = *(Buffer + 0x30); sides=*(Buffer + 0x31);

        for(n = 0; n < (tracks * sides); n++) 
            disco[n] = *(Buffer + 0x33); //asi completamos cada cara y sabemos el tamaño de cada una
    }

    //la primera vez
    fseek ( pfile, 0x100, SEEK_SET ); 
              
    for(n = 1, x = 0; x < (tracks * sides); x++){
        fseek ( pfile, 0x100, SEEK_CUR ); //saltamos la cabecera: file-track
        for(temp = 0; temp < (disco[x] * 0x100) ; temp += 0x20){
            derror = false;

            fread(Buffer, 0x20, 1, pfile);
            if( ((Buffer[0]==0x00)&&( (Buffer[9]==0xA0)||(Buffer[9]==0x20) )&&(Buffer[10]==0x20)&&(Buffer[11]==0x20)&&(Buffer[15]!=0x00))
                ||( (Buffer[0]==0x00)&&( ( ((Buffer[9]==0x42)||(Buffer[9]==0xC2)) && (Buffer[10]==0x41) )
                                       ||( ((Buffer[9]==0x42)||(Buffer[9]==0xC2)) && (Buffer[10]==0x49) ))
                                                 &&(Buffer[15]!=0x00) ) ) 
            { //Si tiene B o blanco y empieza por 00

                for(i=1,j=0;((Buffer[i]!=0x20)&&(i<9));i++,j++){
                     //sprintf(debugt,"DEBUG: DENTRO (j %i - i %i) X(%i)", j, i, x);
                    if( ((Buffer[i]>='A')&&(Buffer[i]<='Z')) || ((Buffer[i]>='0')&&(Buffer[i]<='9'))
                      ||(Buffer[i]=='-')||(Buffer[i]=='!')||(Buffer[i]=='{')||(Buffer[i]=='}')||(Buffer[i]=='\'')||(Buffer[i]=='&')
                      ||(Buffer[i]=='^')||(Buffer[i]=='@')||(Buffer[i]=='+')||(Buffer[i]=='#') )
                    {
                        chStr[j]=Buffer[i];
                    }
                    else
                    {
                        //sprintf(debugt,"DEBUG: Error (j %i - i %i) X(%i)", j, i, x);
                        derror = true;
                    }
                }

                if( derror )
                    continue;

                chStr[j]='.'; // "." de la extension

                while(Buffer[i]==0x20) i++; //saltamos los espacios

                if(Buffer[i]==0x42)
                { //ficheros BAS, BIN, B**
                    for(j++;((Buffer[i]!=0x00)&&(i<12));i++,j++) //mientras no sea fin de fila
                        chStr[j]=Buffer[i];

                    chStr[j]='\0';
                    strcpy(contenido[n],chStr);
                    if(strcmp(contenido[n],contenido[n-1])!=0) //no son iguales
                        n++;

                }else if((Buffer[i]==0xa0)||(Buffer[9]==0x20)) //ficheros que solo tienen un punto
                {
                    //fin de cadena
                    j++;
                    chStr[j]='\0';
                    strcpy(contenido[n],chStr);
                    if(strcmp(contenido[n],contenido[n-1])!=0) //no son iguales
                        n++;

                }else if(Buffer[i]==0xC2)
                {
                    Buffer[i]=0x42; //lo cambiamos a B 
                    for(j++;((Buffer[i]!=0x00)&&(i<12));i++,j++) //mientras no sea fin de fila
                        chStr[j]=Buffer[i];

                    chStr[j]='\0';
                    strcpy(contenido[n],chStr);
                    if(strcmp(contenido[n],contenido[n-1])!=0) //no son iguales
                        n++;
                }
            }
        }
    }

    fclose(pfile);
    sprintf(debugt,"DEBUG: L(%i)", n);

    #if 1
    spoolkeys.contenido[0] = 0;
    switch(n) //si solo hay dos seleccionados
    {
        case 2:
            lowercase(contenido[n-1], false);
            strcpy( (char *) spoolkeys.contenido, (char *) "run|");

        case 1:
            strcat( (char *) spoolkeys.contenido, (char *) contenido[n-1]);
            spoolkeys.comienzo = 1;
            spool = 1;
            reiniciado = 1;
            break;
        default:
            break;
    }

    #endif

    return true;

}

/*
  FILE UTILS
*/

bool fileRead ( t_filebuf * file, char * path )
{
    FILE * pfile;

    //inicializa
    file->size = 0; 
    file->buffer = NULL;

    if(!(pfile = fopen(path, "rb"))){
        //printf ("fileRead(%s) open failure; terminating\n", path);
        return false;
    }

    fseek(pfile, 0, SEEK_END);
    file->size = ftell(pfile);
    rewind(pfile);

    if(file->size < 1){
        printf ("fileRead(%s) memory failure (%i); terminating\n", path, file->size);
        fclose(pfile);
        return false;
    }

    file->buffer = (void *) malloc(file->size); 

    if(file->buffer == NULL){
        printf ("fileRead(%s) memory failure (%i); terminating\n", path, file->size);
        fclose(pfile);
        return false;
    }

    int result = 0;

    if(!(result = fread(file->buffer, 1, file->size, pfile))){
        printf ("fileRead(%s) read failure (%i/%i); terminating\n", path, result, file->size);
        free(file->buffer);
        fclose(pfile);
        return false;
    }
 
    fclose(pfile);
    return true;
}


int snapshot_save (char *pchFileName);
int snapshot_load (char *pchFileName);
extern unsigned char spool;
extern char spool_cad[256];

bool doSnapshot (char * device, char * romname, int action) {
    char pathSnap[1024] = "";
    char tmp[256]= "";
    int result= -1;

    if(strlen(romname) < 5) {
        spool = 1;
        sprintf(spool_cad, " SNAPSHOT-Err: I NEED A ROM LOADED...");

        return false;
    }

    StopSound(1);

    strncpy(tmp, romname, (strlen(romname)-4)); // grab the extension
    tmp[strlen(romname)-4] = '\0'; // zero terminate string
    strcat(tmp, ".sna");

    strncpy(pathSnap, device, 1024);
    strcat(pathSnap, CPC_SAVEDIR);
    strcat(pathSnap, "/");
    strcat(pathSnap, tmp);

    switch(action)
    {
        case SNAP_SAVE:
            result = snapshot_save (pathSnap);
            if(!result)
            {
                spool = 1;
                sprintf(spool_cad, " SNAPSHOT: CPC STATE IS SAVED - OK! (%s)", tmp);
            }
            else
            {
                spool = 1;
                sprintf(spool_cad, " SNAPSHOT-Err: SAVE Err(%s - %i)", tmp, result);
            }
            break;

        case SNAP_LOAD:
            result = snapshot_load (pathSnap);
            if(!result)
            {
                spool = 1;
                sprintf(spool_cad, " SNAPSHOT: CPC STATE LOADED - OK! (%s)", tmp);
            }
            else
            {
                spool = 1;
                sprintf(spool_cad, " SNAPSHOT-Err: LOAD Err(%s - %i)", tmp, result);
            }
            break;
    }
    
    StopSound(0);

    return true;

}

void updateDol (enum support_type dev)
{
#if 0
    char fsys[128]="";
    char path[1024]="";
    t_filebuf file;
    t_zip_info zinfo;
    int iErrorCode = 0;
 
    bool result = true;
    int state = 0;
    int init_state = -1;

    printf( " DOWNLOAD-DOL NET(%i): No update forced - Exit...", init_state);


    const DISC_INTERFACE* disc = NULL;

    switch(dev)
    {
        case SU_USB:
            sprintf(fsys, "usb");
            disc = usb;
        break;
        case SU_SD:
            sprintf(fsys, "sd");
            disc = sd;
        break;

    case SU_DVD:
        default:
        return;
    }

    sprintf(fsys, "%s:/apps/wiituka", device);

    strcpy(path, WiitukaXML.urlpath);
    strcat(path, WIIDEFAULT_NETDATA);
    strcat(path, "/wiituka_update.zip");

    init_state = net_start_thread(path, TCP_REQUEST2, 0); //binary download
    result = false;
    printf( " DOWNLOAD-DOL NET(%i): Init...", init_state);
    while( !result )
    {
        state = net_get_state();

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
        }else if(state == 3)
            result = true;

            GRRLIB_VSync (); //need it by thread

    }
    net_stop_thread();

    zinfo.pchFileNames = NULL;
    zinfo.pchZipFile = NULL;
    zinfo.pchExtension = ".dol";

    if ((iErrorCode = zipBuffered_dir(fbuffer, fileSize, &zinfo)))
    {
        if(iErrorCode != 15)
            sprintf(debugt,"Zdir: Err: %i (%i)" ,iErrorCode, fileSize);

        free(fbuffer);
        return;
    }

    if (zinfo.unZipSize > 0)
    {
        unsigned char * ebuffer = NULL; 
        ebuffer = malloc (zinfo.unZipSize); //Using Zip Info for extract

        if(ebuffer == NULL)
        {
            free(fbuffer);
            return;
        }

        if (!(iErrorCode = zipBuffered_extract( ebuffer, fbuffer, zinfo.dwOffset))) 
        {
            sprintf(path, "%s/boot.dol.tmp", fsys);
            fopen(pfile, path);
            if(fwrite(pfile, ebuffer, zinfo.unZipSize))
            {
                sprintf(path, "%s/boot.dol", fsys);
                unlink(path);
                rename(path, topath);
            }
       }

       free(ebuffer);
    }

    free(fbuffer);
#endif
}


/*
  OTHER UTILS
*/

void lowercase ( char * str,  int ucFirst)
{
    int i = 0;

    if(ucFirst)
    {
        str[0] = toupper(str[i]);
        i++;
    }

    for (; str[i] ; i++)
        str[i] = tolower(str[i]);

}


