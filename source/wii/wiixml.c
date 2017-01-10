/*! \file wiixml.c
 *  \brief XML Driver
 *         Wii Library.
 *
 *
 *  Wii/GC manejo XML
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
#include <mxml.h>

#include "../global.h"
#include "menu/explorer.h"

#include "wiixml.h"

extern WiituKa_Status WiiStatus;
extern xmlWiiCFG WiitukaXML;
extern const unsigned char buttons_def[MAX_CPCBUTTONS * 2];


bool XML_savePublic(char * filename)
{

    mxml_node_t *xml;
    mxml_node_t *group;
    mxml_node_t *data = NULL;   
    char tmp[100];
      
    xml = mxmlNewXML("1.0");   
    group = mxmlNewElement(xml, "WiituKaPublicXML");
    data = mxmlNewElement(group, "option");

    //Create Some config value
    sprintf(tmp, "%i", (int) WiitukaXML.xmlversion);
    mxmlElementSetAttr(data, "XMLversion", tmp);

    sprintf(tmp, "%i", (int) WiitukaXML.xmlgameversion);
    mxmlElementSetAttr(data, "XMLNETversion", tmp);


    sprintf(tmp, "%s", WiitukaXML.urlpath);
    mxmlElementSetAttr(data, "globalUrl", tmp);

    sprintf(tmp, "%i", WiitukaXML.scrtube);
    mxmlElementSetAttr(data, "screenTube", tmp);

    sprintf(tmp, "%i", WiitukaXML.scrintensity);
    mxmlElementSetAttr(data, "screenIntensity", tmp);

    sprintf(tmp, "%i", WiitukaXML.cpcspeed);
    mxmlElementSetAttr(data, "cpcSpeed", tmp);

    sprintf(tmp, "%i", WiitukaXML.cpcfps);
    mxmlElementSetAttr(data, "cpcFps", tmp);

    sprintf(tmp, "%i", WiitukaXML.lastrom);
    mxmlElementSetAttr(data, "lastRom", tmp);

    sprintf(tmp, "%i", WiitukaXML.disablenet);
    mxmlElementSetAttr(data, "disableNet", tmp);

    FILE *fp;
    fp = fopen(filename, "w");

    if(fp == NULL)
        return false;

    if(mxmlSaveFile(xml, fp, MXML_NO_CALLBACK))
        return false;
   
    fclose(fp);
    mxmlDelete(data);
    mxmlDelete(group);
    mxmlDelete(xml);

    return true;

}

void _XML_AddNode( t_fslist * gamenode, mxml_node_t * tree_xml)
{
    mxml_node_t * data = NULL;   
    mxml_node_t * newdata = NULL;

    mxml_node_t * tree = NULL;    

    char str_tmp[1024 + 1];
    const char *tmp;

    char xmltitle[256];

    int to_insert = MXML_ADD_AFTER; //default after last element

    tree = tree_xml;

    for (data = mxmlFindElement(tree, tree, "game", NULL, NULL, MXML_DESCEND);
       data != NULL;
       data = mxmlWalkNext ( data, tree, MXML_NO_DESCEND )
       )
    {

        tmp=mxmlElementGetAttr(data,"title"); 
        if (tmp!=NULL) strncpy(xmltitle, tmp, 255); else continue; //next game


        if( strncmp(gamenode->game.title, xmltitle, 128) < 0){
            to_insert = MXML_ADD_BEFORE; 
            break;
        }
    
    }

    newdata = mxmlNewElement(NULL, "game");

    //Create values
    sprintf(str_tmp, "%s", gamenode->game.title);
    mxmlElementSetAttr(newdata, "title", str_tmp);
      
    sprintf(str_tmp, "%s", gamenode->gfile.filename);
    mxmlElementSetAttr(newdata, "file", str_tmp);

    sprintf(str_tmp, "%s", gamenode->game.genre);
    mxmlElementSetAttr(newdata, "genre", str_tmp);
      
    sprintf(str_tmp, "%s", gamenode->game.company);
    mxmlElementSetAttr(newdata, "company", str_tmp);

    sprintf(str_tmp, "%s", gamenode->game.lang);
    mxmlElementSetAttr(newdata, "language", str_tmp);

    sprintf(str_tmp, "%s", gamenode->game.year);
    mxmlElementSetAttr(newdata, "year", str_tmp);

    sprintf(str_tmp, "%i", (int) gamenode->favorite);
    mxmlElementSetAttr(newdata, "favorite", str_tmp);

    mxmlAdd ( tree, to_insert, data, newdata );
}

bool XML_saveGameList(char * filename, t_fslist * gamelist)
{
    t_fslist * nnode= gamelist;

    FILE *fp;

    mxml_node_t * xml = NULL;
    mxml_node_t * tree_cache = NULL;
    mxml_node_t * data = NULL;   
    mxml_node_t * bind = NULL;

    char str_tmp[1024 + 1];
    const char *tmp;

    int location = -1;
    unsigned char keys[MAX_CPCBUTTONS * 2];

    int UpdateXMLCACHE = 0;

    int exit = 32000;

    if(nnode->fnext == NULL){ //lista vacia
        printf("nothing to save, empty filelist...\n");
        return false;
    }

    //Load xml
    fp = fopen(filename, "r");
    if (fp != NULL) {
        xml = mxmlLoadFile(NULL, fp, MXML_NO_CALLBACK);
           fclose(fp);

        tree_cache = mxmlFindElement(xml, xml, "WiituKaPublicXML", NULL, NULL, MXML_DESCEND);
        if(tree_cache == NULL)
            return false; //malformed xml? exit!

    }else {
        xml = mxmlNewXML("1.0");
        tree_cache = mxmlNewElement(xml, "WiituKaPublicXML");
    }

    while((nnode->fnext != NULL) && (exit)) {
        data = mxmlFindElement(tree_cache, tree_cache, "game",
                           "file", nnode->gfile.filename,
                           MXML_DESCEND);

        if (data == NULL) { 
            _XML_AddNode( nnode, tree_cache);
            UpdateXMLCACHE = 1;
            exit --; //prevents err xml hangs :(
            continue; //retry now :)
        }

        location = SU_NONE;
        bind = NULL;

        tmp = mxmlElementGetAttr(data,"location");
        if (tmp!=NULL)
            location = atoi(tmp); 

        if(location != nnode->gfile.location) {
            sprintf(str_tmp, "%i", (int) nnode->gfile.location);
            mxmlElementSetAttr(data, "location", str_tmp);
            UpdateXMLCACHE = 1;
        }

        int n = 0;
        unsigned char key = 0xff;

        memcpy(keys, buttons_def, MAX_CPCBUTTONS * 2);

        for (bind = mxmlFindElement(data, data, "bind", NULL, NULL, MXML_DESCEND);
                   bind != NULL;
                   bind = mxmlFindElement(bind, data, "bind", NULL, NULL, MXML_DESCEND))
        {
                tmp = mxmlElementGetAttr(bind,"position"); 
                if (tmp!=NULL) n = atoi(tmp); else continue;

                tmp = mxmlElementGetAttr(bind,"key"); 
                if (tmp!=NULL) key = atoi(tmp); else continue;

                keys[n] = key;
        }

        //compara antiguo con actual.
        if(memcmp(keys, nnode->binds, MAX_CPCBUTTONS * 2) != 0) {
            for(n = 0; n < MAX_CPCBUTTONS * 2; n++) {
                if(nnode->binds[n] != keys[n]) {
                    bind = mxmlNewElement(data, "bind");

                    sprintf(str_tmp, "%i", (int) n);
                    mxmlElementSetAttr(bind, "position", str_tmp);
                    sprintf(str_tmp, "%i", (int) nnode->binds[n]);
                    mxmlElementSetAttr(bind, "key", str_tmp);
                }

                UpdateXMLCACHE = 1;
            }
        }

        nnode = nnode->fnext;
   }


   if(UpdateXMLCACHE) {
        //saving xml!
        fp = fopen(filename, "w");

        if(fp != NULL) {
           UpdateXMLCACHE = mxmlSaveFile(xml, fp, MXML_NO_CALLBACK);
           fclose(fp);
        } else
            UpdateXMLCACHE = -1; //error on save

        mxmlDelete(bind);
        mxmlDelete(data);
        mxmlDelete(xml);

   }

   return UpdateXMLCACHE ? false : true;
}


bool XML_loadPublic(char * filename)
{
    FILE *fp;
    mxml_node_t *tree = NULL;
    mxml_node_t *data = NULL;
    const char *tmp;
   
    //Load xml
    fp = fopen(filename, "r");
    if (fp == NULL)
        return false;

    tree = mxmlLoadFile(NULL, fp, MXML_NO_CALLBACK);
    fclose(fp);

    data = mxmlFindElement(tree, tree, "option", NULL, NULL, MXML_DESCEND);
    if(data == NULL)
        return false;

    tmp=mxmlElementGetAttr(data,"XMLversion");
    if (tmp == NULL)
        return false;
    else
        WiitukaXML.xmlversion=atoi(tmp);

    if(WiitukaXML.xmlversion != DEFAULT_XMLVER)
        return false;

    tmp = mxmlElementGetAttr(data,"XMLNETversion");
    if (tmp == NULL)
        return false;
    else
        WiitukaXML.xmlgameversion=atoi(tmp);

    tmp = mxmlElementGetAttr(data,"globalUrl"); 
    if (tmp == NULL) 
        return false;
    else
        strncpy(WiitukaXML.urlpath, tmp, 255);

    tmp = mxmlElementGetAttr(data,"screenTube");   
    if (tmp == NULL)
        return false;
    else {
        WiitukaXML.scrtube = atoi(tmp);
        if(WiitukaXML.scrtube != 1)
            WiitukaXML.scrtube = 0;
    }

    tmp = mxmlElementGetAttr(data,"screenIntensity");   
    if (tmp == NULL)
        return false;
    else {
        WiitukaXML.scrintensity = atoi(tmp);
        if((WiitukaXML.scrintensity > 140 ) || (WiitukaXML.scrintensity < 60) )
            WiitukaXML.scrtube = 100;
    }

    tmp = mxmlElementGetAttr(data,"cpcSpeed");   
    if (tmp == NULL)
        return false;
    else {
        WiitukaXML.cpcspeed = atoi(tmp);
        if((WiitukaXML.cpcspeed > 5 ) || (WiitukaXML.cpcspeed < 3) )
            WiitukaXML.cpcspeed = 4;
    }

    tmp = mxmlElementGetAttr(data,"cpcFps");   
    if (tmp == NULL)
        return false;
    else {
        WiitukaXML.cpcfps = atoi(tmp);
        if(WiitukaXML.cpcfps != 1 )
            WiitukaXML.cpcfps = 0;
    }
   
    tmp = mxmlElementGetAttr(data,"lastRom");   
    if (tmp == NULL)
        return false;
    else {
        WiitukaXML.lastrom = atoi(tmp);
        if(WiitukaXML.lastrom < 0 )
            WiitukaXML.lastrom = 0;
    }

    tmp = mxmlElementGetAttr(data,"disableNet");   
    if (tmp == NULL)
        return false;
    else{
        WiitukaXML.disablenet = atoi(tmp);
        if(WiitukaXML.disablenet != 1)
            WiitukaXML.disablenet = 0;
    }

 
    mxmlDelete(data);
    mxmlDelete(tree);

    return true;
}

bool XML_loadGameList(char * xmldata, t_fslist * gamelist)
{
    mxml_node_t *tree = NULL;
    mxml_node_t *data = NULL;
    mxml_node_t *bind = NULL;
    const char *tmp;

    t_fslist * nnode= gamelist;

    tree = mxmlLoadString(NULL, xmldata, MXML_TEXT_CALLBACK);

    for (data = mxmlFindElement(tree, tree, "game", NULL, NULL, MXML_DESCEND);
       data != NULL;
       data = mxmlFindElement(data, tree, "game", NULL, NULL, MXML_DESCEND))
    {

        tmp=mxmlElementGetAttr(data,"file"); 
        if (tmp!=NULL) 
            strncpy(nnode->gfile.filename, tmp, 255); 
        else 
            continue; //next game
        tmp = mxmlElementGetAttr(data,"location"); 
        if (tmp!=NULL) {
            nnode->gfile.location = atoi(tmp);
            if(nnode->gfile.location == SU_SD) //fichero de cache a ser comprobado
                nnode->gfile.location = SU_NONE;
            else if(((nnode->gfile.location == SU_HTTP) || 
                     (nnode->gfile.location == SU_FTP) ) && 
                     (WiiStatus.Dev_Net < 1)) //INET ACCESIBLE
             
                nnode->gfile.location = SU_NONE;

        } else 
            continue; //next game

        tmp = mxmlElementGetAttr(data,"title");
        if (tmp!=NULL) strncpy(nnode->game.title, tmp, 127); else strncpy(nnode->game.title, nnode->gfile.filename, 127);

        tmp = mxmlElementGetAttr(data,"genre");
        if (tmp!=NULL) strncpy(nnode->game.genre, tmp, 127); else strcpy(nnode->game.genre, "");

        tmp = mxmlElementGetAttr(data,"company");
        if (tmp!=NULL) strncpy(nnode->game.company, tmp, 27); else strcpy(nnode->game.company, "");

        tmp = mxmlElementGetAttr(data,"language");
        if (tmp!=NULL) strncpy(nnode->game.lang, tmp, 27); else strcpy(nnode->game.lang, "");

        tmp = mxmlElementGetAttr(data,"year");
        if (tmp!=NULL) strncpy(nnode->game.year, tmp, 4); else strcpy(nnode->game.year, "");

        tmp = mxmlElementGetAttr(data,"favorite"); 
        if (tmp!=NULL) nnode->favorite = atoi(tmp); else nnode->favorite = 0;

        int n = 0;
        unsigned char key = 0xff;
        memcpy(nnode->binds, buttons_def, MAX_CPCBUTTONS * 2); //TODO: Despues de que la inicializacion se hace con def
                                //     quizas esto no haga falta...

        for (bind = mxmlFindElement(data, data, "bind", NULL, NULL, MXML_DESCEND);
               bind != NULL;
               bind = mxmlFindElement(bind, data, "bind", NULL, NULL, MXML_DESCEND))
        {

            tmp = mxmlElementGetAttr(bind,"position"); 
            if (tmp!=NULL) n = atoi(tmp); else continue;

            tmp = mxmlElementGetAttr(bind,"key"); 
            if (tmp!=NULL) key = atoi(tmp); else continue;

            nnode->binds[n] = key;
        }

        nnode->fnext = malloc(sizeof(t_fslist));

        if(nnode->fnext == NULL)
            return false;

        //limpiamos el siguiente
        FileList_Init (nnode->fnext);

        nnode = nnode->fnext;
 
    }

    mxmlDelete(data);
    mxmlDelete(tree);

    return true;
}


int XML_checkUpdates(char * xmldata)
{
    mxml_node_t *tree = NULL;
    mxml_node_t *data = NULL;
    const char * tmp;
   
    tree = mxmlLoadString(NULL, xmldata, MXML_TEXT_CALLBACK);

    data = mxmlFindElement(tree, tree, "wiitukaOnline", NULL, NULL, MXML_DESCEND);
    if(data == NULL)
        return -1;

    tmp = mxmlElementGetAttr(data,"WiitukaBuild");   
    if (tmp == NULL)
        return -2;
    else
        WiitukaXML.wiiversion = (int) atoi(tmp);

    if(WiitukaXML.wiiversion > DEFAULT_WIIBUILD){
        printf(" New Wiituka Version: %i (%i)", WiitukaXML.wiiversion, DEFAULT_WIIBUILD);
        return 1;
    }
        
    tmp = mxmlElementGetAttr(data,"GamelistVersion"); 
    if (tmp == NULL)
        return -3;
    else
        WiiStatus.VersionXMLNET = atoi(tmp);

    if(WiiStatus.VersionXMLNET > WiitukaXML.xmlgameversion){
        printf(" New GameList: %i (%i)", WiiStatus.VersionXMLNET, WiitukaXML.xmlgameversion);
        return 2;
    }
        
    mxmlDelete(data);
    mxmlDelete(tree);

    return 0;
}


