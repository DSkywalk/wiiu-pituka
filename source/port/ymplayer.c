/*-----------------------------------------------------------------------------

    ST-Sound ( YM files player library )

    Copyright (C) 1995-1999 Arnaud Carre ( http://leonard.oxg.free.fr )
    Copyright (C) 2009-2011 David Colmenero aka D_Skywalk ( http://david.dantoine.org )

    This is a sample program: it's a real-time YM player using windows WaveOut API.

-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------

    This file is part of ST-Sound

    ST-Sound is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    ST-Sound is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with ST-Sound; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

-----------------------------------------------------------------------------*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define    REPLAY_RATE         48000
#define    REPLAY_DEPTH        16
#define    REPLAY_SAMPLELEN        (REPLAY_DEPTH/8)
#define    REPLAY_NBSOUNDBUFFER    2


#include "../ymlib/StSoundLibrary.h"


void StopSound ( int val );

int ym_playing = 0;
volatile YMMUSIC * s_pMusic = NULL;


int ymload_file( char* filename )
{

    if(ym_playing != 0) //playing or closing
        return 0;

    //--------------------------------------------------------------------------
    // Load YM music and creates WAV file
    //--------------------------------------------------------------------------
    printf("Loading music \"%s\"...\n", filename);
    YMMUSIC * pMusic = ymMusicCreate();

    if (ymMusicLoad(pMusic,filename))
    {
        ymMusicInfo_t info;
        ymMusicGetInfo(pMusic,&info);
        printf("Name.....: %s\n",info.pSongName);
        //printf("Author...: %s\n",info.pSongAuthor);
        //printf("Comment..: %s\n",info.pSongComment);
        //printf("Duration.: %d:%02d\n",info.musicTimeInSec/60,info.musicTimeInSec%60);

        //printf("\nPlaying music...(press a key to abort)\n");

        ymMusicSetLoopMode(pMusic,YMTRUE);
        ymMusicPlay(pMusic);
        s_pMusic = pMusic;        // global instance for soundserver callback
        ym_playing = 1;

        StopSound (0); // SDL callback started
    }
    else
    {
        printf("Error in loading file %s:\n%s\n", filename,ymMusicGetLastError(pMusic));
    }

    return 1;

}


int ymload_buffer( char* buffer, int size )
{

    if(ym_playing != 0) //playing or closing
        return 0;

    //--------------------------------------------------------------------------
    // Load YM music and creates WAV file
    //--------------------------------------------------------------------------
    printf("Loading music from buffer...\n");
    YMMUSIC * pMusic = ymMusicCreate();

    if (ymMusicLoadMemory(pMusic,buffer,size))
    {
        ymMusicInfo_t info;
        ymMusicGetInfo(pMusic,&info);
        printf("Name.....: %s\n",info.pSongName);

        ymMusicSetLoopMode(pMusic,YMTRUE);
        ymMusicPlay(pMusic);
        s_pMusic = pMusic;        // global instance for soundserver callback
        ym_playing = 1;

        StopSound (0); // SDL callback started
    }
    else
    {
        printf("Error in loading ym buffer:\n%s\n", ymMusicGetLastError(pMusic));
    }

    return 1;

}




void ymclose(void)
{
    printf("ym Stop.\n");
    ym_playing = -1;

    // Switch off replayer
    StopSound (1);
    ymMusicStop((YMMUSIC *)s_pMusic);
    ymMusicDestroy((YMMUSIC *)s_pMusic);
    s_pMusic = NULL;
    ym_playing = 0;

}

int ymtime(void)
{
    if(s_pMusic != NULL)
        return ymMusicGetPos((YMMUSIC *)s_pMusic) / 1000;
    else
        return 0;
}


