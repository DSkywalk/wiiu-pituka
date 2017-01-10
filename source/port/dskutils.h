

#ifndef dskutils_h
#define dskutils_h

bool dskBufferedRead ( void * dskBuffer);
bool dskRead( char * dskname );

#define SNAP_SAVE 0
#define SNAP_LOAD 1

bool doSnapshot (char * device, char * romname, int action);


typedef struct filebuf {
   void * buffer;
   int size;
}t_filebuf;

bool fileRead ( t_filebuf * file, char * path );

/* unused */
void updateDol (enum support_type dev);


#endif
