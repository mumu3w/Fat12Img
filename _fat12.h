#ifndef ___FAT12_H__
#define ___FAT12_H__


// FAT1起始与结束扇区号
#define FAT1_START_SECTOR           1
#define FAT1_END_SECTOR             9
// FAT2起始与结束扇区号
#define FAT2_START_SECTOR           10
#define FAT2_END_SECTOR             18
// FAT1占用扇区的数量
#define FAT1_SECROT_NUM             (FAT1_END_SECTOR \
                                    - FAT1_START_SECTOR \
                                    + 1)
// FAT2占用扇区的数量
#define FAT2_SECROT_NUM             (FAT2_END_SECTOR \
                                    - FAT2_START_SECTOR \
                                    + 1)
// 扇区字节
#define SECTOR_SIZE                 512
// 根目录起始与结束扇区号
#define ROOT_DIR_START_SECTOR       19
#define ROOT_DIR_END_SECTOR         32
// 根目录占用扇区的数量
#define ROOT_DIR_SECTOR_NUM         (ROOT_DIR_END_SECTOR \
                                    - ROOT_DIR_START_SECTOR \
                                    + 1)
// 每项根目录项占用32B
#define ROOT_DIR_ENTRY_BYTE         (sizeof(DIRENT))
// 对于1440KB软盘最多有224项
#define ROOT_DIR_ENTRY_NUM          (ROOT_DIR_SECTOR_NUM \
                                    * SECTOR_SIZE \
                                    / ROOT_DIR_ENTRY_BYTE)
                                    
// 数据区起始与结束扇区号
#define DATA_START_SECTOR           33
#define DATA_END_SECTOR             2879
// 数据区占用扇区的数量
#define DATA_SECTOR_NUM             (DATA_END_SECTOR \
                                    - DATA_START_SECTOR \
                                    + 1)

// DOS文件名与后缀字符数                                 
#define FILE_NAME_LEN               8
#define FILE_NAME_EXT_LEN           3
#define FILE_NAME_STR_LEN           (FILE_NAME_LEN + FILE_NAME_EXT_LEN)

// 第一个有效簇号
#define FIRSTENTRY                  2

// DOS文件属性
#define DIRATTR_ARCHIVE             0x20 // 存档

// 路径分隔符
#define SEPARATOR					'/'

// FCB结构大小
#define FCB_SIZE					(sizeof(FCB))
// 最多可以打开的文件数
#define FCB_MAX						64
// 文件结束标志
#define FILE_EOF					-1




#include "def.h"
//
// FAT文件系统的时间和日期结构
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
// 目录项结构
//
typedef struct _DIRENT
{
    CHAR     Name[11];
    UINT8    Attributes;
    UINT8    Reserved[10];
//    FAT_TIME LastWriteTime;
//    FAT_DATE LastWriteDate;
    UINT16   LastWriteTime;
    UINT16   LastWriteDate;
    UINT16   FirstCluster;
    UINT32   FileSize;
}DIRENT, *PDIRENT;

#pragma pack()

// 
// 文件控制块
//
typedef struct _FCB
{
    CHAR     Name[13];
    UINT8    Attributes;
//    FAT_TIME LastWriteTime;
//    FAT_DATE LastWriteDate;
    UINT16   LastWriteTime;
    UINT16   LastWriteDate;
    UINT16   FirstCluster;
    UINT32   FileSize;
	
	UINT8	 FileMode; // 文件的打开模式
	UINT16	 iRootDirEntry; // 目录项编号
	UINT32	 Pos; // 位置指针
}FCB, *PFCB;

/********************************************************************

                    FAT12 FileSystem Image
    
********************************************************************/
// 
// 从镜像中删除文件
UINT16 DeleteFileFromImage(const PCHAR FileName);
//
// 从镜像复制文件
//UINT16 CopyFileFromImage(const PCHAR PathName);
//
// 添加文件到镜像
UINT16 AddFile2Image(const PCHAR PathName);
//
// 从镜像读取文件
UINT16 CopyFileBase(FILE *NewFp, const PCHAR FileName);
//
// 添加文件到镜像
UINT16 AddFileBase(FILE *NewFp, const PCHAR DosFileName);
//
// 读逻辑块
INT32 ReadBlock(const UINT32 Lba, PUCHAR BlockBuf);
//
// 写逻辑块
INT32 WriteBlock(const UINT32 Lba, const PUCHAR BlockBuf);

/********************************************************************

                    FAT12 FileSystem Initialization
    
********************************************************************/
void InitFat12FileSystem(const PUCHAR ImageFileName);
void FreeFat12FileSystem(void);
//
// 加载FAT表到缓冲
void LoadFatEntryBuf(void);
//
// 将FAT表写回到文件
void UpdateFatEntry(void);
//
// 加载RootDir表到缓冲
void LoadRootDirEntryBuf(void);
//
// 将RootDir表写回到文件
void UpdateRootDirEntryBuf(void);

/********************************************************************

                       FAT12 FileSystem API
    
********************************************************************/
//
// 文件块读
//UINT32 FatReadFile(const FHANDLE Fp, const UINT32 BytesToRead, PVOID Buf);
UINT32 FatReadFile(const FHANDLE Fp, UINT32 BytesToRead, PVOID Buf);
//
// 从文件读一个字节
//INT16 FatReadFileBase(const FHANDLE Fp, const UINT32 Offset, PUCHAR Byte);
//
// 关闭文件
VOID FatCloseFile(const FHANDLE Fp);
//
// 打开文件
FHANDLE FatOpenFile(const PCHAR FileName, const UINT16 FileMode);
//
// 查找空闲文件块
FHANDLE FindEmptyFCB(VOID);
//
// 删除文件
VOID DeleteFile(const UINT16 iRootDirEntry);
// 
// 获取ROOTDir表中找到的第一条空项
UINT16 FindEmptyRootDirEntry(VOID);
//
// 获取Fat表中找到的第一条空项
UINT16 FindEmptyFatEntry(const UINT16 iOriginFatEntry);
//
// 在根目录表中查找文件
UINT16 FindFileInRootDir(const PCHAR FileName);
//
// 获取剩余磁盘空间
UINT16 GetRestSector(VOID);
//
// 将文件名从8+3普通文件名
VOID DosFileName2FileName(const PCHAR DosFileName, PCHAR FileName);
//
// 将普通文件名转为8+3格式
VOID FileName2DosFileName(const PCHAR FileName, PCHAR DosFileName);
//
// 获取一条iRootDirEntry的值
UINT16 ReadRootDirEntryValue(const UINT16 iRootDirEntry, PDIRENT pDirEnt);
//
// 向iRootDirEntry写入值
UINT16 WriteRootDirEntryValue(const UINT16 iRootDirEntry, 
                              const PDIRENT pDirEnt);
//
// 获取当前iFatEntry的值
UINT16 ReadFatEntryValue(const UINT16 iFatEntry);
//
// 设置当前iFatEntry的值
void WriteFatEntryValue(const UINT16 iFatEntry, UINT16 Value);
//
// 从FAT缓冲中读取值
UINT16 ReadValueToFatBuf(const UINT16 FatEntryByteOff);
//
// 将值写入FAT缓冲
void WriteValueToFatBuf(const UINT16 FatEntryByteOff, const UINT16 Value);
//
// 读iFatEntry对应的数据磁盘块内容
UINT16 ReadDataSector(const UINT16 iFatEntry, PUCHAR SectorBuf);
//
// 向iFatEntry对应的数据磁盘块写内容
UINT16 WriteDataSector(const UINT16 iFatEntry, const PUCHAR SectorBuf);



#endif