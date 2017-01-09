

#ifndef utils_h
#define utils_h

void process_spoolkey(int spool_ticks);


/** \brief Estructura de Datos
 *  Esta es una estructura general que contiene datos.
 * \note Vigila el tamaño.
 */
typedef struct{
  unsigned char * buffer; /**< El buffer que contiene los datos */

  long size; /**< El tamaño del buffer creado */
  long _bptr; /**< Puntero interno a los datos */
} t_data;


void FreeData (t_data * dat);
bool InitData (t_data * dat, long alloc_size);

bool SeekData (t_data * dat, long pos, bool from_current_pos);

int ReadData (t_data * dat, void * out, long data_size, bool move_ptr);
int WriteData (t_data * dat, void * source, long data_size, bool move_ptr);

unsigned short GetwordData (t_data * dat, long pos);
long GetptrData (t_data * dat, long pos);
char * GetstrData (t_data * dat, long pos);

#endif
