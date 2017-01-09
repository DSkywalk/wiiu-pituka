

#ifndef dskutils_h
#define dskutils_h

bool Explorer_dskBufferedRead ( void * dskBuffer);
bool Explorer_dskRead( char * dskname );

#define SNAP_SAVE 0
#define SNAP_LOAD 1

bool doSnapshot (char * path, char * romname, int action);


typedef struct filebuf {
   void * buffer;
   int size;
}t_filebuf;

bool Explorer_fileRead ( t_filebuf * file, char * path );
bool CreateDirs (char *path);
bool mountDev ( enum support_type dev, bool remount);
void updateDol (enum support_type dev);

#endif
