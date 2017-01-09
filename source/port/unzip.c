/****************************************************************************
 * Pituka - Nintendo Wii/Gamecube Port
 *
 * D_Skywalk Sep 2008
 *
 * unzip.c
 *
 * D_Skywalk, miniunzip lib for pituka
 ***************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>

#include "../caprice/z80.h"
#include "../global.h"
#include "unzip.h"

extern byte *pbGPBuffer;
extern char debugt [1024];

int zipBuffered_dir (const void * zipBuffer, const int bSize, t_zip_info *zi)
{
   int n, iFileCount;
   long lFilePosition;
   dword dwCentralDirPosition, dwNextEntry;
   word wCentralDirEntries, wCentralDirSize, wFilenameLength;
   byte *pbPtr;
   char *pchStrPtr;
   dword dwOffset, dwUnzipSize;
   char *pchThisExtension;

   iFileCount = 0;

   if(zipBuffer == NULL || bSize == 0){
      return ERR_FILE_NOT_FOUND;
   }

   #ifdef GEKKO
    #define ZIP_HEAD  0x504b0506
   #else
    #define ZIP_HEAD  0x06054b50
   #endif

   

   wCentralDirEntries = 0;
   wCentralDirSize = 0;
   dwCentralDirPosition = 0;
   lFilePosition = bSize - 256; //total zip bytes - 256
   do {
      if(lFilePosition <= 0)
         return ERR_FILE_BAD_ZIP; // exit if loading of data ends
 
      memcpy(pbGPBuffer, zipBuffer + lFilePosition, 256);


      pbPtr = pbGPBuffer + (256 - 22); // pointer to end of central directory (under ideal conditions)
      while (pbPtr != (byte *)pbGPBuffer) {
         if (*(dword *)pbPtr == ZIP_HEAD) { // check for end of central directory signature
            wCentralDirEntries = *(byte *)(pbPtr + 10) + (*(byte *)(pbPtr + 11) << 8);
            wCentralDirSize = *(byte *)(pbPtr + 12) + (*(byte *)(pbPtr + 13) << 8);
            dwCentralDirPosition = *(byte *)(pbPtr + 16) + (*(byte *)(pbPtr + 17) << 8) + (*(byte *)(pbPtr + 18)<< 16) + (*(byte *)(pbPtr + 19)<<24); //*(dword *)(pbPtr + 16);
            break;
         }
         pbPtr--; // move backwards through buffer
      }
      lFilePosition -= 256; // move backwards through ZIP file

   } while (wCentralDirEntries == 0);

   if (wCentralDirSize == 0) {
      return ERR_FILE_BAD_ZIP; // exit if no central directory was found
   }
      
   memcpy(pbGPBuffer, zipBuffer + dwCentralDirPosition, wCentralDirSize);


   pbPtr = pbGPBuffer;
   if (zi->pchFileNames) {
      free(zi->pchFileNames); // dealloc old string table
   }

   zi->pchFileNames = (char *)malloc(wCentralDirSize); // approximate space needed by using the central directory size
   pchStrPtr = zi->pchFileNames;

   for (n = wCentralDirEntries; n; n--) {
      wFilenameLength = *(byte *)(pbPtr + 28) + (*(byte *)(pbPtr + 29) << 8);
      dwOffset = *(byte *)(pbPtr + 42) + (*(byte *)(pbPtr + 43) << 8) + (*(byte *)(pbPtr + 44)<< 16) + (*(byte *)(pbPtr + 45)<<24); 
      dwUnzipSize = *(byte *)(pbPtr + 24) + (*(byte *)(pbPtr + 25) << 8) + (*(byte *)(pbPtr + 26)<< 16) + (*(byte *)(pbPtr + 27)<<24); 
      dwNextEntry = wFilenameLength + ( *(byte *)(pbPtr + 30) + (*(byte *)(pbPtr + 31) << 8) ) + ( *(byte *)(pbPtr + 32) + (*(byte *)(pbPtr + 33) << 8) );
      pbPtr += 46;

      pchThisExtension = (char *) zi->pchExtension;
      while (*pchThisExtension != '\0') { // loop for all extensions to be checked
         if (strncasecmp((char *)pbPtr + (wFilenameLength - 4), pchThisExtension, 4) == 0) {
            strncpy(pchStrPtr, (char *)pbPtr, wFilenameLength); // copy filename from zip directory
            pchStrPtr[wFilenameLength] = 0; // zero terminate string
            pchStrPtr += wFilenameLength+1;
            zi->unZipSize = dwUnzipSize;
            zi->dwOffset = dwOffset;
            iFileCount++;
            break;
         }
         pchThisExtension += 4; // advance to next extension
      }
      pbPtr += dwNextEntry;
   }

   if (iFileCount == 0) { // no files found?
      return ERR_FILE_EMPTY_ZIP;
   }

   zi->iFiles = iFileCount;

   return 0; // operation completed successfully
}



int zipBuffered_extract (void * extBuffer, const void * zipBuffer, unsigned int dwOffset)
{
   int iStatus, iCount;
   dword dwSize;
   byte *pbInputBuffer, *pbOutputBuffer;
   z_stream z;


   if ((extBuffer == NULL) || (zipBuffer == NULL))
      return ERR_FILE_UNZIP_FAILED;
   

   memcpy(pbGPBuffer, zipBuffer + dwOffset, 30); 

   // length of compressed data
   dwSize = *(byte *)(pbGPBuffer + 18) + (*(byte *)(pbGPBuffer + 19) << 8) + (*(byte *)(pbGPBuffer + 20)<< 16) + (*(byte *)(pbGPBuffer + 21)<<24); 
   dwOffset += 30 + (*(byte *)(pbGPBuffer + 26) + (*(byte *)(pbGPBuffer + 27) << 8)) + (*(byte *)(pbGPBuffer + 28) + (*(byte *)(pbGPBuffer + 29) << 8)); 

   zipBuffer += dwOffset;

   pbInputBuffer = pbGPBuffer; // space for compressed data chunck
   pbOutputBuffer = pbInputBuffer + 16384; // space for uncompressed data chunck
   z.zalloc = (alloc_func)0;
   z.zfree = (free_func)0;
   z.opaque = (voidpf)0;
   iStatus = inflateInit2(&z, -MAX_WBITS); // init zlib stream (no header)
   do {
      z.next_in = pbInputBuffer;
      if (dwSize > 16384) { // limit input size to max 16K or remaining bytes
         z.avail_in = 16384;
      } else {
         z.avail_in = dwSize;
      }

      memcpy(pbInputBuffer, zipBuffer, z.avail_in);
      zipBuffer += z.avail_in;

      while ((z.avail_in) && (iStatus == Z_OK)) { // loop until all data has been processed
         z.next_out = pbOutputBuffer;
         z.avail_out = 16384;
         iStatus = inflate(&z, Z_NO_FLUSH); // decompress data
         iCount = 16384 - z.avail_out;
         if (iCount) { // save data to file if output buffer is full
            memcpy(extBuffer, pbOutputBuffer, iCount); 
            extBuffer += iCount;
         }
      }
      dwSize -= 16384; // advance to next chunck
   } while ((dwSize > 0) && (iStatus == Z_OK)) ; // loop until done
   if (iStatus != Z_STREAM_END) {
      return ERR_FILE_UNZIP_FAILED; // abort on error
   }
   iStatus = inflateEnd(&z); // clean up

   return 0; // data was successfully decompressed


}



