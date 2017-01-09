/*! \file net.c
 *  \brief Net Driver.
 *         Wii Library.
 *
 *  \version 0.2
 *
 *  Interfaz de Red para Wii
 *
 *   Pituka - Nintendo Wii/Gamecube Port
 *  (c) Copyright 2008-2009 David Colmenero (aka D_Skywalk)
 */


#include <stdio.h>
#include <ctype.h>
#include <gccore.h>
#include <ogcsys.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h> 
#include <malloc.h>
#include <network.h>
#include <ogc/lwp_watchdog.h>
#include <sys/types.h>
#include <sys/errno.h>
#include <fcntl.h>
#include <wiiuse/wpad.h>

#include "http.h"
#include "net.h"
#include "../../global.h"

#define NUM_THREADS             2
#define MAX_BUFFER_SIZE  1024*512



lwp_t threads[NUM_THREADS];
mutex_t mutexcheck;
mutex_t mutexdownload;
bool do_tcp_treat;

int  tcp_state;

unsigned char dbuffer[MAX_BUFFER_SIZE];
unsigned int bufferSize = 0;

char globalUrl[1024 + 1];

extern WiituKa_Status WiiStatus;


// -----------------------------------------------------------
// INIT NETWORK
// -----------------------------------------------------------
	

int tcp_init(void) 
{    
	 	
 //   printf(" NET: Waiting for network to initialise...\n");
			
	s32 result;
    while ((result = net_init()) == -EAGAIN);
	
    if ( result>= 0) 
    {
        char myIP[16];
        if (if_config(myIP, NULL, NULL, true, 2) < 0) /* MAX_RETRIES 2 */
        {
            printf(" NET: Error reading IP address, exiting");
			return 1;
        }
//        printf(" NET: Initialised [%s].\n",myIP);
    } 
    else 
    {
//      printf(" NET: Unable to initialise, exiting\n");
	  return 1;
    }
	
	return 0;
}


void tcp_sleep(unsigned int seconds)
{
//   printf(" NET.thread: sleep %d sec.\n",seconds);
   sleep(seconds);
}


// -----------------------------------------------------------
// THREAD STUFF
// -----------------------------------------------------------

int globalNetStatus = -1;
int globalMode = -1;
int globalRetry = -1;
	
void *tcp_thread(void *threadid)
{
   //u32 http_status;
   //u32 outlen;
   int read_size = -1;

   LWP_MutexLock (mutexcheck);
   while(do_tcp_treat)
   {
      LWP_MutexUnlock(mutexcheck);
	  
	  switch (tcp_state)
	  {
	    // Init ethernet DHCP network
	    case TCP_INIT:   
		  globalNetStatus = tcp_init();
		  if (globalNetStatus==0){
		     tcp_state = globalMode; 
		     WiiStatus.Dev_Net = 1;
		  }else{
		     tcp_state = TCP_ERROR;
		     WiiStatus.Dev_Net = 0;
		  }

		  break;

        // Check for CHAR BUFFER
		case TCP_REQUEST1: 
            read_size = http_request(globalUrl, NULL, dbuffer, (1024*128) );
            if(!read_size) {
                printf(" NET: Error making http request\n");
                tcp_state=TCP_RETRY;
            }
            else {     
                //http_get_result(&http_status, &outbuf, &outlen);
                LWP_MutexLock(mutexdownload);
                if (read_size < (1024*128)) {
                    bufferSize = read_size;
                    //strncpy(cbuffer, (char*) outbuf, outlen);
                    //cbuffer = (char*) dbuffer;
                    dbuffer[read_size] = '\0';
                } else
                    printf("<none>\n");
                LWP_MutexUnlock(mutexdownload);
                //free(outbuf);			 
                tcp_state = TCP_END;
            }
            break;

        // Check for BINARY BUFFER
		case TCP_REQUEST2:
            if(strlen(globalUrl) > 5) {
                //printf("stateMachine=TCP_REQUEST1\n");
                read_size = http_request(globalUrl, NULL, dbuffer, MAX_BUFFER_SIZE );
                if (!read_size) {
                    tcp_state=TCP_RETRY;
                    printf(" NET: Error making http request\n");
                } else {      
                    //http_get_result(&http_status, &outbuf, &outlen); 			

                    LWP_MutexLock(mutexdownload);
                    if (read_size < MAX_BUFFER_SIZE) {
                        bufferSize = read_size;
                        //memcpy(dbuffer, outbuf, outlen);
                    } else
                        printf("version: <none>\n");
                    LWP_MutexUnlock(mutexdownload);

                    //free(outbuf);			 
                }
            }
            tcp_state = TCP_END;
            break;

	//TCP Canceled
		case TCP_CANCEL:
		    printf(" NET: state = TCP_CANCEL\n");
           	do_tcp_treat=false;
		    break;

        // Error on network init - retry after 10 seconds			  
		case TCP_ERROR:
		    printf(" NET: state = TCP_ERROR\n");
		    if((globalRetry--) > 0){
	          	tcp_sleep(10);
		    	tcp_state=TCP_INIT;
		    }else
			    tcp_state = TCP_CANCEL;

		    break;
		
		// Error on http download - retry after 30 seconds
		case TCP_RETRY:
		    printf(" NET: state = TCP_RETRY\n");
		    if((globalRetry--) > 0){
	           	tcp_sleep(30);
		    	tcp_state = TCP_INIT;
		    } else
			    tcp_state = TCP_CANCEL;
		    break;

	    // Thread shutdown
		case TCP_END:
            do_tcp_treat=false;
            break;
      }
      LWP_MutexLock(mutexcheck);
   }
   LWP_MutexUnlock(mutexcheck);
   
   return 0;
}


void tcp_clear_memory(void)
{
    memset(dbuffer, 0x00, sizeof(dbuffer));
    //cbuffer = (char*) dbuffer;
}


// -----------------------------------------------------------
// INTERFACE API
// -----------------------------------------------------------

int net_get_state(void)
{
   LWP_MutexLock(mutexcheck);  
   return tcp_state;
   LWP_MutexUnlock(mutexcheck);  
}

int net_get_buffersize(void)
{
   return bufferSize;
}

void * net_get_filebuffer(void)
{
   LWP_MutexLock(mutexdownload);  
   return (void*) dbuffer;
   LWP_MutexUnlock(mutexdownload);
}

char * net_get_charbuffer(void)
{
   LWP_MutexLock(mutexdownload);  
   return (char*) dbuffer;
   LWP_MutexUnlock(mutexdownload);
}

int net_start_thread(char * getUrl, int mode, int retry)
{
    //tcp_state=TCP_INIT;
    do_tcp_treat = true;
    char mproc[5];

    strncpy(mproc, getUrl, 4);
    mproc[4] = '\0';

    if(memcmp(mproc, "http", 4)!=0) //sino es http, fuera...
        return -1;

    if(!WiiStatus.Dev_Net)
	return -2;

    tcp_clear_memory();
    strncpy(globalUrl, getUrl, 1024);
    globalMode = mode;
    tcp_state = globalMode; 
    globalRetry = retry;

    int rc = LWP_CreateThread(&threads[0], tcp_thread, NULL, NULL, 0, 1);

    if(rc!=0) 
    {
        printf("ERROR; return code from LWP_CreateThread is %d\n", rc);
    }
	
    return rc;
}
	
int net_stop_thread(void)
{

   LWP_MutexLock (mutexcheck);
   do_tcp_treat = false;
   LWP_MutexUnlock (mutexcheck);
   
   return 0;
}


void Wiituka_LoadNetUpdate(void);

void *net_init_dev(void * thread_arg)
{

   int retry = 1;
   bool do_init_treat= true;
   int init_state = TCP_INIT;

   while(do_init_treat)
   {

      switch (init_state)
	  {
	    // Init ethernet DHCP network
	    case TCP_INIT:   
	  	globalNetStatus = tcp_init();

	  	if (globalNetStatus==0){
 		     WiiStatus.Dev_Net = 1;
		     init_state = TCP_END;
		}else{
                     WiiStatus.Dev_Net = 0;
		     init_state = TCP_ERROR;
		}

  	        break;

        // Error on network init - retry after 30 seconds		  
		case TCP_ERROR:
		    printf(" NET.INIT: TCP_ERROR... retry...\n");
		    if(retry--){
	            	tcp_sleep(10);
		    	tcp_state=TCP_INIT;
		    }else
       	    	   	do_init_treat=false;
		    break;

		case TCP_END:
           	   do_init_treat=false;
                   break;		

      }	  
   }

   printf(" NET.INIT: FINISH...\n");

   if(WiiStatus.Dev_Net)
      Wiituka_LoadNetUpdate();

   return 0;

}

int net_init_thread(void)
{

    int rc = LWP_CreateThread(&threads[1], net_init_dev, NULL, NULL, 0, 1);

    if(rc!=0) 
    {
        printf("ERROR; return code from LWP_CreateThread is %d\n", rc);
    }
	
    return rc;
}


