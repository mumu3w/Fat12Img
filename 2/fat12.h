#ifndef __FAT12_H__
#define __FAT12_H__

//
// FAT������
//
#define FAT1_START_SECTOR  1
#define FAT1_END_SECTOR    9
#define FAT2_START_SECTOR  10
#define FAT2_END_SECTOR    18
//
// ROOTĿ¼������
//
#define ROOT_START_SECTOR  19
#define ROOT_END_SECTOR    32
//
// DATA������
//
#define DATA_START_ESCTOR  33
#define DATA_END_ESCTOR    2879
//
// ������С(һ������512B)
//
#define SECTOR_SIZE        512


#include "def.h"
//
// FAT�ļ�ϵͳ��ʱ������ڽṹ
//
typedef struct _FAT_TIME
{
    UINT16   Seconds : 5;
    UINT16   Minute  : 6;
    UINT16   Hour    : 5;
}FAT_TIME, *PFAT_TIME;

typedef struct _FAT_DATE
{
    UINT16   Day   : 5;
    UINT16   Month : 6;
    UINT16   Year  : 5;
}FAT_DATE, *PFAT_DATE;

#pragma pack(1)
//
// Ŀ¼��ṹ
//
typedef struct _DIRENT
{
    CHAR     Name[11];
    UINT8    Attributes;
    UINT8    Reserved[10];
    FAT_TIME LastWriteTime;
    FAT_DATE LastWriteDate;
    UINT16   FirstCluster;
    UINT32   FileSize;
}DIRENT, *PDIRENT;

#pragma pack()

INT32 FatReadFile(FILE *ImgFp, FILE *NewFp, DIRENT DirEnt);
INT32 FatGetNextCluster(FILE *ImgFp, 
        UINT16 CurrentCluster, 
        UINT16 *NextCluster);
INT32 FatOpenFile(const CHAR *FileName, FILE *ImgFp, PDIRENT pDirEnt);
INT32 ImgWriteBlock(UINT32 Lba, FILE *ImgFp, UCHAR *buffer);
void FileNameCpy(CHAR *FileName1, 
		const CHAR *FileName2, 
		UINT32 FileName2Size);
BOOL FileNameCmp(const CHAR *FileName1, const CHAR *FileName2);
void Error(int status, const char *format, ...);


#endif