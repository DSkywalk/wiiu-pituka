
#ifndef libunzip_h
#define libunzip_h

#define ERR_FILE_NOT_FOUND       13
#define ERR_FILE_BAD_ZIP         14
#define ERR_FILE_EMPTY_ZIP       15
#define ERR_FILE_UNZIP_FAILED    16

int zipBuffered_dir(const void * zipBuffer, const int bSize, t_zip_info *zi);
int zipBuffered_extract(void * extBuffer, const void * zipBuffer, unsigned int dwOffset);

#endif
