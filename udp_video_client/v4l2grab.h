/****************************************************/
/*                                                  */
/*                      v4lgrab.h                   */
/*                                                  */
/****************************************************/


#ifndef __V4LGRAB_H__
#define __V4LGRAB_H__


#include <stdio.h>

 

//typedef enum { FALSE = 0, TRUE = 1, OK = 2} BOOL;

 

//#define SWAP_HL_WORD(x) {(x) = ((x)<<8) | ((x)>>8);}

//#define SWAP_HL_DWORD(x) {(x) = ((x)<<24) | ((x)>>24) | (((x)&0xff0000)>>8) | (((x)&0xff00)<<8);}

 

#define  FREE(x)       if((x)){free((x));(x)=NULL;}

 

typedef unsigned char  BYTE;
typedef unsigned short	WORD;
typedef unsigned long  DWORD;

/**/
#pragma pack(1)

typedef struct tagBITMAPFILEHEADER{
     WORD	bfType;                // the flag of bmp, value is "BM"
     DWORD    bfSize;                // size BMP file ,unit is bytes
     DWORD    bfReserved;            // 0
     DWORD    bfOffBits;             // must be 54

}BITMAPFILEHEADER;

 
typedef struct tagBITMAPINFOHEADER{
     DWORD    biSize;                // must be 0x28
     DWORD    biWidth;           //
     DWORD    biHeight;          //
     WORD		biPlanes;          // must be 1
     WORD		biBitCount;            //
     DWORD    biCompression;         //
     DWORD    biSizeImage;       //
     DWORD    biXPelsPerMeter;   //
     DWORD    biYPelsPerMeter;   //
     DWORD    biClrUsed;             //
     DWORD    biClrImportant;        //
}BITMAPINFOHEADER;

typedef struct tagRGBQUAD{
     BYTE	rgbBlue;
     BYTE	rgbGreen;
     BYTE	rgbRed;
     BYTE	rgbReserved;
}RGBQUAD;

 

#if defined(__cplusplus)
extern "C" {     /* Make sure we have C-declarations in C++ programs */
#endif

 

//if successful return 1,or else return 0
int openVideo();
int closeVideo();

//data 用来存储数据的空间, size空间大小
void getVideoData(unsigned char *data, int size);
 
#if defined(__cplusplus)
}

#endif


#endif //__V4LGRAB_H___
