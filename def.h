#ifndef __DEF_H__
#define __DEF_H__


typedef unsigned char       UINT8;
typedef char                INT8;
typedef unsigned short      UINT16;
typedef short               INT16;
typedef unsigned int        UINT32;
typedef int                 INT32;
typedef void                VOID;
typedef void				*PVOID;

typedef UINT8               UCHAR;
typedef INT8                CHAR;
typedef CHAR                *PCHAR;
typedef UCHAR               *PUCHAR;
typedef UINT16              *PUINT16;

// 文件句柄
typedef int					FHANDLE;


#ifndef TRUE
typedef int BOOL;
#define TRUE (1 == 1)
#define FALSE (0 == 1)
#endif

#ifndef NUL
#define NUL '\0'
#endif

#ifndef SPACE
#define SPACE ' '
#endif


#endif