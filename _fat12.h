#ifndef ___FAT12_H__
#define ___FAT12_H__


// FAT1��ʼ�����������
#define FAT1_START_SECTOR           1
#define FAT1_END_SECTOR             9
// FAT2��ʼ�����������
#define FAT2_START_SECTOR           10
#define FAT2_END_SECTOR             18
// FAT1ռ������������
#define FAT1_SECROT_NUM             (FAT1_END_SECTOR \
                                    - FAT1_START_SECTOR \
                                    + 1)
// FAT2ռ������������
#define FAT2_SECROT_NUM             (FAT2_END_SECTOR \
                                    - FAT2_START_SECTOR \
                                    + 1)
// �����ֽ�
#define SECTOR_SIZE                 512
// ��Ŀ¼��ʼ�����������
#define ROOT_DIR_START_SECTOR       19
#define ROOT_DIR_END_SECTOR         32
// ��Ŀ¼ռ������������
#define ROOT_DIR_SECTOR_NUM         (ROOT_DIR_END_SECTOR \
                                    - ROOT_DIR_START_SECTOR \
                                    + 1)
// ÿ���Ŀ¼��ռ��32B
#define ROOT_DIR_ENTRY_BYTE         (sizeof(DIRENT))
// ����1440KB���������224��
#define ROOT_DIR_ENTRY_NUM          (ROOT_DIR_SECTOR_NUM \
                                    * SECTOR_SIZE \
                                    / ROOT_DIR_ENTRY_BYTE)
                                    
// ��������ʼ�����������
#define DATA_START_SECTOR           33
#define DATA_END_SECTOR             2879
// ������ռ������������
#define DATA_SECTOR_NUM             (DATA_END_SECTOR \
                                    - DATA_START_SECTOR \
                                    + 1)

// DOS�ļ������׺�ַ���                                 
#define FILE_NAME_LEN               8
#define FILE_NAME_EXT_LEN           3
#define FILE_NAME_STR_LEN           (FILE_NAME_LEN + FILE_NAME_EXT_LEN)

// ��һ����Ч�غ�
#define FIRSTENTRY                  2

// DOS�ļ�����
#define DIRATTR_ARCHIVE             0x20 // �浵

// ·���ָ���
#define SEPARATOR					'/'

// FCB�ṹ��С
#define FCB_SIZE					(sizeof(FCB))
// �����Դ򿪵��ļ���
#define FCB_MAX						64
// �ļ�������־
#define FILE_EOF					-1




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
//    FAT_TIME LastWriteTime;
//    FAT_DATE LastWriteDate;
    UINT16   LastWriteTime;
    UINT16   LastWriteDate;
    UINT16   FirstCluster;
    UINT32   FileSize;
}DIRENT, *PDIRENT;

#pragma pack()

// 
// �ļ����ƿ�
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
	
	UINT8	 FileMode; // �ļ��Ĵ�ģʽ
	UINT16	 iRootDirEntry; // Ŀ¼����
	UINT32	 Pos; // λ��ָ��
}FCB, *PFCB;

/********************************************************************

                    FAT12 FileSystem Image
    
********************************************************************/
// 
// �Ӿ�����ɾ���ļ�
UINT16 DeleteFileFromImage(const PCHAR FileName);
//
// �Ӿ������ļ�
//UINT16 CopyFileFromImage(const PCHAR PathName);
//
// ����ļ�������
UINT16 AddFile2Image(const PCHAR PathName);
//
// �Ӿ����ȡ�ļ�
UINT16 CopyFileBase(FILE *NewFp, const PCHAR FileName);
//
// ����ļ�������
UINT16 AddFileBase(FILE *NewFp, const PCHAR DosFileName);
//
// ���߼���
INT32 ReadBlock(const UINT32 Lba, PUCHAR BlockBuf);
//
// д�߼���
INT32 WriteBlock(const UINT32 Lba, const PUCHAR BlockBuf);

/********************************************************************

                    FAT12 FileSystem Initialization
    
********************************************************************/
void InitFat12FileSystem(const PUCHAR ImageFileName);
void FreeFat12FileSystem(void);
//
// ����FAT������
void LoadFatEntryBuf(void);
//
// ��FAT��д�ص��ļ�
void UpdateFatEntry(void);
//
// ����RootDir������
void LoadRootDirEntryBuf(void);
//
// ��RootDir��д�ص��ļ�
void UpdateRootDirEntryBuf(void);

/********************************************************************

                       FAT12 FileSystem API
    
********************************************************************/
//
// �ļ����
//UINT32 FatReadFile(const FHANDLE Fp, const UINT32 BytesToRead, PVOID Buf);
UINT32 FatReadFile(const FHANDLE Fp, UINT32 BytesToRead, PVOID Buf);
//
// ���ļ���һ���ֽ�
//INT16 FatReadFileBase(const FHANDLE Fp, const UINT32 Offset, PUCHAR Byte);
//
// �ر��ļ�
VOID FatCloseFile(const FHANDLE Fp);
//
// ���ļ�
FHANDLE FatOpenFile(const PCHAR FileName, const UINT16 FileMode);
//
// ���ҿ����ļ���
FHANDLE FindEmptyFCB(VOID);
//
// ɾ���ļ�
VOID DeleteFile(const UINT16 iRootDirEntry);
// 
// ��ȡROOTDir�����ҵ��ĵ�һ������
UINT16 FindEmptyRootDirEntry(VOID);
//
// ��ȡFat�����ҵ��ĵ�һ������
UINT16 FindEmptyFatEntry(const UINT16 iOriginFatEntry);
//
// �ڸ�Ŀ¼���в����ļ�
UINT16 FindFileInRootDir(const PCHAR FileName);
//
// ��ȡʣ����̿ռ�
UINT16 GetRestSector(VOID);
//
// ���ļ�����8+3��ͨ�ļ���
VOID DosFileName2FileName(const PCHAR DosFileName, PCHAR FileName);
//
// ����ͨ�ļ���תΪ8+3��ʽ
VOID FileName2DosFileName(const PCHAR FileName, PCHAR DosFileName);
//
// ��ȡһ��iRootDirEntry��ֵ
UINT16 ReadRootDirEntryValue(const UINT16 iRootDirEntry, PDIRENT pDirEnt);
//
// ��iRootDirEntryд��ֵ
UINT16 WriteRootDirEntryValue(const UINT16 iRootDirEntry, 
                              const PDIRENT pDirEnt);
//
// ��ȡ��ǰiFatEntry��ֵ
UINT16 ReadFatEntryValue(const UINT16 iFatEntry);
//
// ���õ�ǰiFatEntry��ֵ
void WriteFatEntryValue(const UINT16 iFatEntry, UINT16 Value);
//
// ��FAT�����ж�ȡֵ
UINT16 ReadValueToFatBuf(const UINT16 FatEntryByteOff);
//
// ��ֵд��FAT����
void WriteValueToFatBuf(const UINT16 FatEntryByteOff, const UINT16 Value);
//
// ��iFatEntry��Ӧ�����ݴ��̿�����
UINT16 ReadDataSector(const UINT16 iFatEntry, PUCHAR SectorBuf);
//
// ��iFatEntry��Ӧ�����ݴ��̿�д����
UINT16 WriteDataSector(const UINT16 iFatEntry, const PUCHAR SectorBuf);



#endif