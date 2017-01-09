/****************************************************************************
 * Pituka - Nintendo Wii/Gamecube Port
 *
 * D_Skywalk Sep 2008
 *
 * utils.c
 *
 * libreria general
 ***************************************************************************/

#include "../caprice/cap32.h"
#include "../global.h"
#include "utils.h"


void FreeData (t_data * dat)
{
    free(dat->buffer);

    dat->buffer = NULL;
    dat->_bptr = 0;
    dat->size = 0;
}

void ClearData (t_data * dat)
{
    if(dat->buffer != NULL)
        free(dat->buffer);
        
    dat->buffer = NULL;
    dat->_bptr = 0;
    dat->size = 0;
}

bool InitData (t_data * dat, long alloc_size)
{
     dat->buffer = malloc (alloc_size);
     memset(dat->buffer, 0, alloc_size);

     if (dat->buffer == NULL)
        return false;

    dat->_bptr = 0;
    dat->size = alloc_size;

    return true;
}


int ReadData (t_data * dat, void * out, long data_size, bool move_ptr)
{
    if ( data_size  <= 0 )
        return 0;

    if ( dat->_bptr + data_size > dat->size ) //error don't read - out of limits!
    {
        memcpy(out, &dat->buffer[dat->_bptr], data_size);
        if (move_ptr)
            dat->_bptr += data_size;
    }
    else
    {
        data_size = (dat->_bptr + data_size) - dat->size; //get last info
        memcpy(out, &dat->buffer[dat->_bptr], data_size);
        if (move_ptr)
            dat->_bptr = -1; //end of data
    }

    return data_size;
}

int WriteData (t_data * dat, void * source, long data_size, bool move_ptr)
{
    if ( data_size  <= 0 )
        return 0;

    if ( dat->_bptr + data_size <= dat->size ) //error don't read - out of limits!
    {
        memcpy(&dat->buffer[dat->_bptr], source, data_size);
        if (move_ptr)
            dat->_bptr += data_size;
    }
    else
    {
        data_size = (dat->_bptr + data_size) - dat->size; //get last info
        memcpy(&dat->buffer[dat->_bptr], source, data_size);
        if (move_ptr)
            dat->_bptr = -1; //end of data
    }

    return data_size;
}

bool SeekData (t_data * dat, long pos, bool from_ptr)
{
    if ( pos >=  dat->size )
        return false;

    if ( from_ptr )
    {
        if ( dat->_bptr + pos <= dat->size ) //error don't read - out of limits!
            return false;

        dat->_bptr += pos;
    }
    else
    {
        dat->_bptr = pos;
    }

    return true;
}

unsigned short GetwordData (t_data * dat, long pos)
{

    if ((pos < 0) && (dat != NULL))
        return 0;
    
    if (pos > dat->size - 2)
        return 0;

    const unsigned short val = (dat->buffer[pos] << 8) + dat->buffer[pos+1];

    return (val);
}
/*
	const UInt16	val = (data_arr [pos] << 8) + data_arr [pos + 1];

	return (val);
*/

long GetptrData (t_data * dat, long pos)
{
    if ((pos < 0) && (dat != NULL))
        return 0;
    
    if (pos > dat->size - 2)
        return 0;

    signed short val = (signed short) GetwordData( dat, pos );
    val += pos;
    
    return (val);
}
/*
	Int16				val = static_cast <Int16> (read_word (data_arr, pos));
	val += pos;

	return (val);
*/

char * GetstrData (t_data * dat, long pos)
{
    if ((pos < 0) && (dat != NULL))
        return 0;
    
    if (pos > dat->size - 1)
        return 0;

    int tmp_pos = 0;
	char * str;
	const long file_len = dat->size;
	
	while ((pos + tmp_pos) < file_len && dat->buffer[(pos + tmp_pos)] != '\0')
	    tmp_pos++;

    str = malloc (tmp_pos+1);
    if(str == NULL)
        return 0;

    strncpy(str, ((char *) &dat->buffer[pos]), tmp_pos);

    str[(tmp_pos+1)] = '\0';

	return (str);

}

/*

 OLD PITUKA UTILS

*/


extern byte keyboard_matrix[16];
extern byte keyboard_translation_SDL[320];
extern byte bit_values[8];

extern char spool_cad[256];
extern unsigned char spool;
extern unsigned char spool_act;
extern Pituka_SpoolKeys spoolkeys;

void process_spoolkey(int spool_ticks){
    static byte tecla_pulsada='\0';
    static int tiempo_spool = 0;
    static bool pulsada = false;

    if ( spoolkeys.comienzo == true )
    {
        spoolkeys.cur = &spoolkeys.contenido[0];
        spoolkeys.cont = strlen((char *) spoolkeys.contenido) + 1; //contenido + intro final
        tecla_pulsada = '\0';
        tiempo_spool = 0;
        pulsada = false;

        //no mas iniciaciones
        spoolkeys.comienzo = false;
    }
            
    if ( (tiempo_spool+spool_ticks)>50 ) //si han pasado 50ms
    {
        tiempo_spool -= 50; //asi siempre empezamos de 0 :)
        tecla_pulsada=keyboard_translation_SDL[*spoolkeys.cur];

        if ( (tecla_pulsada != 0xff) ) 
        {
            if ( pulsada == false ) 
            {
                keyboard_matrix[tecla_pulsada >> 4] &= ~bit_values[tecla_pulsada & 7]; // tecla pulsada
                pulsada = true;
            }
            else
            {
                keyboard_matrix[tecla_pulsada >> 4] |= bit_values[tecla_pulsada & 7];  // tecla soltada
                pulsada = false;
            }
        }
        else
        {
            if ( *spoolkeys.cur=='|' ) 
            {
                if ( strcmp((char *) spoolkeys.contenido,"|cpm") == 0 ) //son iguales :?
                {
                    if ( pulsada == false )
                    {
                        keyboard_matrix[ 0x32 >> 4] &= ~bit_values[ 0x32 & 7]; // tecla pulsada -> "|"
                        keyboard_matrix[ 0x25 >> 4] &= ~bit_values[ 0x25 & 7]; // tecla pulsada -> 0x25 (SHIFT)
                        pulsada = true;
                    }
                    else
                    {
                        keyboard_matrix[ 0x32 >> 4] |= bit_values[ 0x32 & 7];  // tecla soltada -> "|"
                        keyboard_matrix[ 0x25 >> 4] |= bit_values[ 0x25 & 7];  // tecla soltada -> 0x25 (SHIFT)
                        pulsada = false;
                    }
                }
                else
                {
                    if ( pulsada == false )
                    {
                        keyboard_matrix[ 0x81 >> 4] &= ~bit_values[ 0x81 & 7]; // tecla pulsada -> "
                        keyboard_matrix[ 0x25 >> 4] &= ~bit_values[ 0x25 & 7]; // tecla pulsada -> 0x25 (SHIFT)
                        pulsada = true;
                    }
                    else
                    {
                        keyboard_matrix[ 0x81 >> 4] |= bit_values[ 0x81 & 7];  // tecla soltada -> "
                        keyboard_matrix[ 0x25 >> 4] |= bit_values[ 0x25 & 7];  // tecla soltada -> 0x25 (SHIFT)
                        pulsada = false;
                    }
                }
            }else if( *spoolkeys.cur == '\0' ) 
            {
                if ( pulsada == false )
                {
                    keyboard_matrix[ 0x06 >> 4] &= ~bit_values[ 0x06 & 7]; // tecla pulsada -> INTRO
                    pulsada = true;
                }
                else
                {
                    keyboard_matrix[ 0x06 >> 4] |= bit_values[ 0x06 & 7];  // tecla soltada -> INTRO
                    pulsada = false;
                }
            }else if( *spoolkeys.cur == '!' )
            {
                if ( pulsada == false )
                {
                    keyboard_matrix[ 0x80 >> 4] &= ~bit_values[ 0x80 & 7]; // tecla pulsada -> "1"
                    keyboard_matrix[ 0x25 >> 4] &= ~bit_values[ 0x25 & 7]; // tecla pulsada -> 0x25 (SHIFT)
                    pulsada = true;
                }
                else
                {
                    keyboard_matrix[ 0x80 >> 4] |= bit_values[ 0x80 & 7];  // tecla soltada -> "1"
                    keyboard_matrix[ 0x25 >> 4] |= bit_values[ 0x25 & 7];  // tecla soltada -> 0x25 (SHIFT)
                    pulsada = false;
                }
            }else if(*spoolkeys.cur == '#')
            {
                if( pulsada == false )
                {
                    keyboard_matrix[ 0x71 >> 4] &= ~bit_values[ 0x71 & 7]; // tecla pulsada -> "3"
                    keyboard_matrix[ 0x25 >> 4] &= ~bit_values[ 0x25 & 7]; // tecla pulsada -> 0x25 (SHIFT)
                    pulsada = true;
                }
                else
                {
                    keyboard_matrix[ 0x71 >> 4] |= bit_values[ 0x71 & 7];  // tecla soltada -> "3"
                    keyboard_matrix[ 0x25 >> 4] |= bit_values[ 0x25 & 7];  // tecla soltada -> 0x25 (SHIFT)
                    pulsada = false;
                }
            }else if(*spoolkeys.cur == '+')
            {
                if(pulsada == false)
                {
                    keyboard_matrix[ 0x34 >> 4] &= ~bit_values[ 0x34 & 7]; // tecla pulsada -> "; o ?"
                    keyboard_matrix[ 0x25 >> 4] &= ~bit_values[ 0x25 & 7]; // tecla pulsada -> 0x25 (SHIFT)
                    pulsada = true;
                }
                else
                {
                    keyboard_matrix[ 0x34 >> 4] |= bit_values[ 0x34 & 7];  // tecla soltada -> "; o ?"
                    keyboard_matrix[ 0x25 >> 4] |= bit_values[ 0x25 & 7];  // tecla soltada -> 0x25 (SHIFT)
                    pulsada = false;
                }
            }else if(*spoolkeys.cur == '{')
            {
                if(pulsada == false)
                {
                    keyboard_matrix[ 0x21 >> 4] &= ~bit_values[ 0x21 & 7]; // tecla pulsada -> "{ o [" 
                    keyboard_matrix[ 0x25 >> 4] &= ~bit_values[ 0x25 & 7]; // tecla pulsada -> 0x25 (SHIFT)
                    pulsada = true;
                }
                else
                {
                    keyboard_matrix[ 0x21 >> 4] |= bit_values[ 0x21 & 7];  // tecla soltada -> "{ o [" 
                    keyboard_matrix[ 0x25 >> 4] |= bit_values[ 0x25 & 7];  // tecla soltada -> 0x25 (SHIFT)
                    pulsada = false;
                }
            }else if(*spoolkeys.cur == '}')
            {
                if(pulsada == false)
                {
                    keyboard_matrix[ 0x23 >> 4] &= ~bit_values[ 0x23 & 7]; // tecla pulsada -> "} o ]"
                    keyboard_matrix[ 0x25 >> 4] &= ~bit_values[ 0x25 & 7]; // tecla pulsada -> 0x25 (SHIFT)
                    pulsada = true;
                }
                else
                {
                    keyboard_matrix[ 0x23 >> 4] |= bit_values[ 0x23 & 7];  // tecla soltada -> "} o ]"
                    keyboard_matrix[ 0x25 >> 4] |= bit_values[ 0x25 & 7];  // tecla soltada -> 0x25 (SHIFT)
                    pulsada = false;
                }
            }else if(*spoolkeys.cur == '&')
            {
                if(pulsada==false)
                {
                    keyboard_matrix[ 0x60 >> 4] &= ~bit_values[ 0x60 & 7]; // tecla pulsada -> "6"
                    keyboard_matrix[ 0x25 >> 4] &= ~bit_values[ 0x25 & 7]; // tecla pulsada -> 0x25 (SHIFT)
                    pulsada = true;
                }
                else
                {
                    keyboard_matrix[ 0x60 >> 4] |= bit_values[ 0x60 & 7];  // tecla soltada -> "6"
                    keyboard_matrix[ 0x25 >> 4] |= bit_values[ 0x25 & 7];  // tecla soltada -> 0x25 (SHIFT)
                    pulsada = false;
                }
            }else if(*spoolkeys.cur == '\'')
            {
                if(pulsada == false)
                {
                    keyboard_matrix[ 0x51 >> 4] &= ~bit_values[ 0x51 & 7]; // tecla pulsada -> "7"
                    keyboard_matrix[ 0x25 >> 4] &= ~bit_values[ 0x25 & 7]; // tecla pulsada -> 0x25 (SHIFT)
                    pulsada = true;
                }
                else
                {
                    keyboard_matrix[ 0x51 >> 4] |= bit_values[ 0x51 & 7];  // tecla soltada -> "7"
                    keyboard_matrix[ 0x25 >> 4] |= bit_values[ 0x25 & 7];  // tecla soltada -> 0x25 (SHIFT)
                    pulsada = false;
                }
            }else if(*spoolkeys.cur == '^') //pone libra
            {
                if(pulsada == false){
                    keyboard_matrix[ 0x30 >> 4] &= ~bit_values[ 0x30 & 7]; // tecla pulsada -> "^"
                    keyboard_matrix[ 0x25 >> 4] &= ~bit_values[ 0x25 & 7]; // tecla pulsada -> 0x25 (SHIFT)
                    pulsada = true;
                }
                else
                {
                    keyboard_matrix[ 0x30 >> 4] |= bit_values[ 0x30 & 7];  // tecla soltada -> "^"
                    keyboard_matrix[ 0x25 >> 4] |= bit_values[ 0x25 & 7];  // tecla soltada -> 0x25 (SHIFT)
                    pulsada = false;
                }
            }// falta @
            
            else if (*spoolkeys.cur == '$') //para borrar TEMP!
            {
                if(pulsada == false)
                {
                    keyboard_matrix[ 0x97 >> 4] &= ~bit_values[ 0x97 & 7]; // tecla pulsada -> 0x25 (SHIFT)
                    pulsada = true;
                }
                else
                {
                    keyboard_matrix[ 0x97 >> 4] |= bit_values[ 0x97 & 7];  // tecla soltada -> 0x25 (SHIFT)
                    pulsada = false;
                }
            }
        }
              
        if(pulsada == false)
        {
            if(spoolkeys.cont>1)
            {
                if(*spoolkeys.cur!=*(spoolkeys.cur+1))
                {
                    spoolkeys.cur++;
                    spoolkeys.cont--;
                }
                else
                    *spoolkeys.cur=0x01;
            }
            else
            {
                spoolkeys.cur = &spoolkeys.contenido[0];
                spoolkeys.cont = 0;
                spoolkeys.contenido[0] = '\0';
                tiempo_spool = 0;
            }
        }
    }
}


