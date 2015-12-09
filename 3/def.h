#ifndef __DEF_H__
#define __DEF_H__


typedef unsigned char  UINT8;
typedef unsigned char  BYTE;
typedef char           INT8;
typedef unsigned char  UCHAR;
typedef char           CHAR;

typedef unsigned short UINT16;
typedef unsigned short WORD;
typedef short          INT16;

typedef unsigned int   UINT32;
typedef unsigned int   DWORD;
typedef int            INT32;

#ifndef TRUE
typedef int BOOL;
#define TRUE (1 == 1)
#define FALSE (0 == 1)
#endif


#endif