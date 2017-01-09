/*! \file audiowii.c
 *  \brief Audio Driver.
 *         Wii Library.
 *
 *  \version 0.4
 *
 *  Audio para 48Khz/16bit/Stereo
 *
 *   Pituka - Nintendo Wii/Gamecube Port
 *  (c) Copyright 2008-2009 David Colmenero (aka D_Skywalk)
 */

#include <gccore.h>
#include <ogcsys.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <asndlib.h>
#include <mp3player.h>

#include "../global.h"

#define SFX_STACK 16384
#define SFX_THREAD_PRIO 80
#define SFX_THREAD_FRAG_SIZE 2048 //1024

static lwpq_t sfx_queue;
static lwp_t sfx_thread;
static u8 astack[SFX_STACK];
static mutex_t sfx_mutex = LWP_MUTEX_NULL;

static bool sfx_thread_paused = true;

static int sb = 0;

static u8 sound_buffer[2][SFX_THREAD_FRAG_SIZE]
  __attribute__ ((__aligned__ (32)));


//punteros al sistema de sonido del pituka
extern unsigned char *pbSndBuffer;
extern unsigned char *pbSndBufferEnd;
extern unsigned char *pbSndStream;



/*! \fn static void audio_switch_buffers()
    \brief Envia el buffer de Wii al DMA.
    \todo hacer esta funcion solo interna.
*/
static void audio_switch_buffers() 
{

    if (!sfx_thread_paused)
    {
        sb = sb ^ 1; //cambia el puntero
        AUDIO_InitDMA((u32) sound_buffer[sb], SFX_THREAD_FRAG_SIZE);
        LWP_ThreadSignal(sfx_queue);
    }
}

/*! \fn static void * sfx_thread_func(void *arg)
    \brief El hilo que copia el buffer del CPC al buffer que sera enviado a la Wii.

    \param arg No usado.
    \return No usado, siempre NULL.
    \todo hacer esta funcion solo interna.
*/
static void * sfx_thread_func(void *arg) {

    LWP_InitQueue(&sfx_queue);

    while (true)
    {

        if (sfx_thread_paused)
            memset (sound_buffer[sb], 0, SFX_THREAD_FRAG_SIZE);
        else
        {
            LWP_MutexLock(sfx_mutex);

            if((pbSndStream + SFX_THREAD_FRAG_SIZE) >= pbSndBufferEnd)
            {
                memcpy(sound_buffer[sb], pbSndStream, (pbSndBufferEnd-pbSndStream)); //copia del cpc al buffer
                pbSndStream = pbSndBuffer;           // vuelve al comienzo
            }
            else
            {
                memcpy(sound_buffer[sb], pbSndStream, SFX_THREAD_FRAG_SIZE); //copia del cpc al buffer
                pbSndStream += (SFX_THREAD_FRAG_SIZE); // se prepara para el próximo copiado
            }

            LWP_MutexUnlock(sfx_mutex);

        }

        DCFlushRange(sound_buffer[sb], SFX_THREAD_FRAG_SIZE); //vacia la cache del envio realizado 
                                                              //y espera al ok de la CPU
        LWP_ThreadSleep(sfx_queue);
    }

    return NULL;
}

/*! \fn void Init_SoundSystem( void )
    \brief Inicia el sistema de sonido crea los hilos y configura el callback.
    \todo hacer esta funcion solo interna.
*/
void Init_SoundSystem( void ) {
    memset(sound_buffer[0], 0, SFX_THREAD_FRAG_SIZE);
    memset(sound_buffer[1], 0, SFX_THREAD_FRAG_SIZE);

    DCFlushRange(sound_buffer[0], SFX_THREAD_FRAG_SIZE);
    DCFlushRange(sound_buffer[1], SFX_THREAD_FRAG_SIZE);

    LWP_MutexInit(&sfx_mutex, false);
    LWP_CreateThread (&sfx_thread, sfx_thread_func, NULL, astack, SFX_STACK, SFX_THREAD_PRIO);
}

/*! \fn void Close_SoundSystem( void )
    \brief Para el dma, cierra los hilos y libera memoria.
    \todo hacer esta funcion solo interna.
*/
void Close_SoundSystem( void )
{
    AUDIO_StopDMA();
}

/*! \fn void StopSound ( int val )
    \param val inicia el sistema de sonido cuando es 0
    \brief Pausa y reanuda el sonido.
    \note Habria que buscar una forma mejor de hacer esto...
*/
void StopSound ( int val )
{

    sfx_thread_paused = val;

    //clean
    memset (sound_buffer[0], 0, SFX_THREAD_FRAG_SIZE);
    memset (sound_buffer[1], 0, SFX_THREAD_FRAG_SIZE);
    DCFlushRange(sound_buffer[0], SFX_THREAD_FRAG_SIZE);
    DCFlushRange(sound_buffer[1], SFX_THREAD_FRAG_SIZE);

    switch(val)
    {
        case 1:
            AUDIO_RegisterDMACallback(NULL);
            ASND_Init();
            //needed by mp3player
            SND_Pause(0);
            break;    

        case 0:
            //if (MP3Player_IsPlaying()) { MP3Player_Stop(); }
            ASND_Pause(1);
            AUDIO_StopDMA();
            AUDIO_InitDMA((u32) sound_buffer[sb], SFX_THREAD_FRAG_SIZE);
            AUDIO_StartDMA();

            AUDIO_RegisterDMACallback(audio_switch_buffers);
            //SoundInit();
            break;
    }

}

/*! \fn int SoundInit (void)
    \brief Funcion global de inicializacion de sonido.

*/
void SoundInit (void)
{
  audio_switch_buffers();
}
/*! \fn int SoundSetup (void)
    \brief Funcion global de configuracion del sonido.
    \note Indica a la emulacion el tamaño del buffer (x4)
*/
int SoundSetup (void)
{
    Init_SoundSystem();
    return SFX_THREAD_FRAG_SIZE << 2;
}

/*! \fn int SoundInit (void)
    \brief Funcion global de finalizacion del sistema de sonido.
*/
void SoundClose(void) 
{
  Close_SoundSystem();
}

