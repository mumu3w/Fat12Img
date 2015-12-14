/*
	(C) Mumu3w@outlook.com
	
	�ṩ��/��fat12imageд����ȡ�ļ�
	
    fat12.c
	
		Mumu3w@outlook.com  2015.12.12
	
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "def.h"
#include "_fat12.h"


static FILE *ImgFp;
static UCHAR FAT1Buf[FAT1_SECROT_NUM * SECTOR_SIZE];
static UCHAR FAT2Buf[FAT2_SECROT_NUM * SECTOR_SIZE];
static UCHAR ROOTDirBuf[ROOT_DIR_SECTOR_NUM * SECTOR_SIZE];
static FCB FcbArray[FCB_MAX];

/********************************************************************

                    FAT12 FileSystem Image
    
********************************************************************/
// ��ȡ�ļ�����
UINT32 GetFileLen(FILE *Fp)
{
    UINT32 FileLen, FilePos;
    
    FilePos = ftell(Fp); // ���浱ǰλ��
    
    fseek(Fp, 0L, SEEK_END);
    FileLen = ftell(Fp); // �����ļ�����
    
    fseek(Fp, FilePos, SEEK_SET);
    
    return FileLen;
}

// 
// �Ӿ�����ɾ���ļ�
UINT16 DeleteFileFromImage(const PCHAR FileName)
{
    UINT16 iRootDirEntry;
    
    iRootDirEntry = FindFileInRootDir(FileName);
    if(iRootDirEntry != (UINT16)-1)
    {
        DeleteFile(iRootDirEntry);
    }
    else
    {
        return (UINT16)-1;
    }
}

//
// �Ӿ������ļ�
//UINT16 CopyFileFromImage(const PCHAR PathName)
//{
//    FILE *NewFp;
//    PCHAR FileName;
//    UINT16 iRootDirEntry;
//    
//    if(NULL == (NewFp = fopen(PathName, "wb")))
//    {
//        fprintf(stderr, "%s Can't open\n", PathName);
//        exit(EXIT_FAILURE);     
//    }
//    
//    FileName = strrchr(PathName, SEPARATOR);
//    FileName = ((NULL == FileName) ? PathName : (FileName + 1));
//    
//    iRootDirEntry = FindFileInRootDir(FileName);
//    if(iRootDirEntry != (UINT16)-1)
//    {
//        CopyFileBase(NewFp, FileName);
//        fclose(NewFp);
//        return 0;
//    }
//    else
//    {
//        fclose(NewFp);
//        return (UINT16)-1;
//    }
//}

//
// ����ļ�������
UINT16 AddFile2Image(const PCHAR PathName)
{
    FILE *NewFp;
    PCHAR FileName;
    UCHAR DosFileName[FILE_NAME_STR_LEN+1];
    UINT32 FileLen;
    UINT16 iRootDirEntry;
    
    if(NULL == (NewFp = fopen(PathName, "rb")))
    {
        fprintf(stderr, "%s Can't open\n", PathName);
        exit(EXIT_FAILURE);     
    }
    
    FileName = strrchr(PathName, SEPARATOR);
    FileName = ((NULL == FileName) ? PathName : (FileName + 1));
    
    // ����ͬ���ļ���ɾ��
    iRootDirEntry = FindFileInRootDir(FileName);
    if(iRootDirEntry != (UINT16)-1)
    {
        DeleteFile(iRootDirEntry);
    }
    else // ������ͬ���ļ���Ҫ�ж��Ƿ��п�DirEntry
    {
        if(FindEmptyRootDirEntry() == (UINT16)-1)
        {
            fclose(NewFp);
            return (UINT16)-1;
        }
    }
    
    // �ռ��Ƿ��㹻
    FileLen = GetFileLen(NewFp);
    if(FileLen > (GetRestSector() * SECTOR_SIZE)) 
    {
        fclose(NewFp);
        return (UINT16)-1;
    }
    // ��ͨ�ַ���תdos�ļ���
    FileName2DosFileName(FileName, DosFileName);
    // ����ļ�
    AddFileBase(NewFp, DosFileName);
    
    fclose(NewFp);
    return 0;
}

//
// �Ӿ����ȡ�ļ�
UINT16 CopyFileBase(FILE *NewFp, const PCHAR FileName)
{
    UCHAR BlockBuf[SECTOR_SIZE];
    UINT32 FileLen;
    UINT16 iRootDirEntry;
    UINT16 iCurFatEntry, iNextFatEntry;
    DIRENT DirEnt;
    
    // �����ļ�
    iRootDirEntry = FindFileInRootDir(FileName);
    if(iRootDirEntry != (UINT16)-1)
    {
        ReadRootDirEntryValue(iRootDirEntry, &DirEnt); // ��ȡDirEnt��
        FileLen = DirEnt.FileSize; // �ļ�����
        iCurFatEntry = DirEnt.FirstCluster; // ��һ��iFatEntry
        while(iCurFatEntry != 0 && iCurFatEntry < 0xFF0)
        {
            ReadDataSector(iCurFatEntry, BlockBuf);
            if(FileLen < SECTOR_SIZE) // �����һ����������SECTOR_SIZE�����⴦��
            {
                fwrite(BlockBuf, sizeof(UCHAR), FileLen, NewFp);
            }
            else
            {
                fwrite(BlockBuf, sizeof(UCHAR), SECTOR_SIZE, NewFp);
            }
            
            // ��һ��iFatEntry
            iNextFatEntry = ReadFatEntryValue(iCurFatEntry);
            iCurFatEntry = iNextFatEntry;
            FileLen -= SECTOR_SIZE; // ��ʣ�¶����ֽ���Ҫ����
        }
        
        return 0;
    }
    else
    {
        return (UINT16)-1;
    }
}

//
// ����ļ�������
UINT16 AddFileBase(FILE *NewFp, const PCHAR DosFileName)
{
    UCHAR BlockBuf[SECTOR_SIZE];
    UINT16 SectorLists[DATA_SECTOR_NUM];
    UINT16 SectorNums, ExtBytes, iFinalSector, iRootDirEntry;
    UINT32 FileLen;
    DIRENT DirEnt;
    int i;
    
    // �ļ�����
    // fseek(NewFp, 0L, SEEK_END);
    // FileLen = ftell(NewFp);
    FileLen = GetFileLen(NewFp);

    SectorLists[0] = 0; // ���ļ�FirstClusterΪ0
    if(FileLen > 0)
    {
        ExtBytes = FileLen % SECTOR_SIZE;
        SectorNums = (FileLen / SECTOR_SIZE) + (ExtBytes != 0 ? 1 : 0);
        iFinalSector = SectorNums - 1;
        
        // iFatEntry�������
        SectorLists[0] = FindEmptyFatEntry(FIRSTENTRY);
        for(i = 1; i < SectorNums; i++)
        {
            SectorLists[i] = FindEmptyFatEntry(SectorLists[i-1] + 1);
            WriteFatEntryValue(SectorLists[i-1], SectorLists[i]);
        }
        WriteFatEntryValue(SectorLists[iFinalSector], 0xFFF);
        
        // �ǵý�λ������Ϊ�ļ���ͷ
        // fseek(NewFp, 0L, SEEK_SET);
        
        // ������������, ���һ������Ҫ���⴦��
        for(i = 0; i < iFinalSector; i++)
        {
            fread(BlockBuf, sizeof(UCHAR), SECTOR_SIZE, NewFp);
            WriteDataSector(SectorLists[i], BlockBuf);
        }
        if(ExtBytes != 0)
        {
            memset(BlockBuf, 0, SECTOR_SIZE);
            fread(BlockBuf, sizeof(UCHAR), ExtBytes, NewFp);
        }
        else
        {
            fread(BlockBuf, sizeof(UCHAR), SECTOR_SIZE, NewFp);
        }
        WriteDataSector(SectorLists[iFinalSector], BlockBuf);
    }
    
    // iRootDirEntry����������
    iRootDirEntry = FindEmptyRootDirEntry();
    memset(&DirEnt, 0, ROOT_DIR_ENTRY_BYTE);
    memcpy(DirEnt.Name, DosFileName, FILE_NAME_STR_LEN);
    DirEnt.Attributes = DIRATTR_ARCHIVE;
    DirEnt.LastWriteTime = 0;
    DirEnt.LastWriteDate = 0;
    DirEnt.FirstCluster = SectorLists[0];
    DirEnt.FileSize = FileLen;
    WriteRootDirEntryValue(iRootDirEntry, &DirEnt);
    
    return 0;
}

//
// ���߼���
INT32 ReadBlock(const UINT32 Lba, PUCHAR BlockBuf)
{
    fseek(ImgFp, Lba * SECTOR_SIZE, SEEK_SET);
    return fread(BlockBuf, sizeof(UCHAR), SECTOR_SIZE, ImgFp);
}

//
// д�߼���
INT32 WriteBlock(const UINT32 Lba, const PUCHAR BlockBuf)
{
    fseek(ImgFp, Lba * SECTOR_SIZE, SEEK_SET);
    return fwrite(BlockBuf, sizeof(UCHAR), SECTOR_SIZE, ImgFp);
}

/********************************************************************

                    FAT12 FileSystem ��ʼ��
    
********************************************************************/
//
// ��ʼ���ļ�ϵͳ
VOID InitFat12FileSystem(const PUCHAR ImageFileName)
{
    // �򿪴��̾����ļ�
    if(NULL == (ImgFp = fopen(ImageFileName, "rb+")))
    {
        fprintf(stderr, "%s Can't open\n", ImageFileName);
        exit(EXIT_FAILURE);
    }
    
    LoadFatEntryBuf();
    LoadRootDirEntryBuf();
	memset(FcbArray, 0 ,FCB_MAX * FCB_SIZE);
}
//
// �ͷ��ļ�ϵͳ
VOID FreeFat12FileSystem(VOID)
{
    UpdateRootDirEntryBuf();
    UpdateFatEntry();
    fclose(ImgFp);
}

//
// ����FAT������
VOID LoadFatEntryBuf(VOID)
{
    int i;
    
    for(i = 0; i < FAT1_SECROT_NUM; i++)
    {
        ReadBlock(FAT1_START_SECTOR + i, &FAT1Buf[i * SECTOR_SIZE]);
    }
    for(i = 0; i < FAT2_SECROT_NUM; i++)
    {
        ReadBlock(FAT2_START_SECTOR + i, &FAT2Buf[i * SECTOR_SIZE]);
    }
}

//
// ��FAT��д�ص��ļ�
VOID UpdateFatEntry(VOID)
{
    int i;
    
    for(i = 0; i < FAT1_SECROT_NUM; i++)
    {
        WriteBlock(FAT1_START_SECTOR + i, &FAT1Buf[i * SECTOR_SIZE]);
    }
    for(i = 0; i < FAT2_SECROT_NUM; i++)
    {
        WriteBlock(FAT2_START_SECTOR + i, &FAT2Buf[i * SECTOR_SIZE]);
    }
}

//
// ����RootDir������
VOID LoadRootDirEntryBuf(VOID)
{
    int i;
    
    for(i = 0; i < ROOT_DIR_SECTOR_NUM; i++)
    {
        ReadBlock(ROOT_DIR_START_SECTOR + i, &ROOTDirBuf[i * SECTOR_SIZE]);
    }
}

//
// ��RootDir��д�ص��ļ�
VOID UpdateRootDirEntryBuf(VOID)
{
    int i;
    
    for(i = 0; i < ROOT_DIR_SECTOR_NUM; i++)
    {
        WriteBlock(ROOT_DIR_START_SECTOR + i, &ROOTDirBuf[i * SECTOR_SIZE]);
    }
}

/********************************************************************

                       FAT12 FileSystem API
    
********************************************************************/
//
// �ļ����
//UINT32 FatReadFile(const FHANDLE Fp, const UINT32 BytesToRead, PVOID Buf)
//{
//	UINT32 ByteRead = 0; // ʵ�ʶ�ȡ���ֽ���
//	UCHAR Byte;
//	PUCHAR pBuf = (PUCHAR)Buf;
//	int i;
//	
//	for(i = 0; i < BytesToRead; i++)
//	{
//		if(FatReadFileBase(Fp, FcbArray[Fp].Pos++, &Byte) 
//							!= FILE_EOF)
//		{
//			pBuf[i] = Byte;
//			ByteRead++;
//		}
//	}
//	
//	return ByteRead;
//}

//
// ���ļ���
UINT32 FatReadFile(const FHANDLE Fp, UINT32 BytesToRead, PVOID Buf)
{
	UINT16 OffsetInSector;
	UINT16 iFatEntry; // ��ʵ���ǿ��
	UINT16 Chars;
	UINT32 Reads = 0;
	PUCHAR pBuf = (PUCHAR)Buf;
	UCHAR BlockBuf[SECTOR_SIZE];
	PUCHAR pBlockBuf;
	int i;
	
	// ƫ�ƴ����ļ���Сֱ���˳�
	if(FcbArray[Fp].Pos >= FcbArray[Fp].FileSize)
	{
		return FILE_EOF;
	}
	//FcbArray[Fp].Pos = Offset;
	
	// �����ֽ����ڵĴ��̿�
	iFatEntry = FcbArray[Fp].FirstCluster;
	for(i = FcbArray[Fp].Pos / SECTOR_SIZE; i > 0; i--)
	{
		iFatEntry = ReadFatEntryValue(iFatEntry);
	}
	// �ֽ����ڵĴ��̿�ƫ��
	OffsetInSector = FcbArray[Fp].Pos % SECTOR_SIZE;
	
	// ʵ�ʿɶ����ֽ������ļ����ȵ�����
	if(BytesToRead > FcbArray[Fp].FileSize - FcbArray[Fp].Pos)
	{
		BytesToRead = FcbArray[Fp].FileSize - FcbArray[Fp].Pos;
	}
	
	while(BytesToRead > 0)
	{
		Chars = SECTOR_SIZE - OffsetInSector; // �����̿���Ҫ��ȡ���ֽ���
		if(Chars > BytesToRead)
		{
			Chars = BytesToRead;
		}
		if(ReadDataSector(iFatEntry, BlockBuf) == (UINT16)-1)
		{
			return Reads ? Reads : FILE_EOF;
		}
		iFatEntry = ReadFatEntryValue(iFatEntry);
		pBlockBuf = &BlockBuf[OffsetInSector];
		
		// ���һ����ֻ��ȡ��Ч�Ĳ���
		if((FcbArray[Fp].Pos + Chars) >= FcbArray[Fp].FileSize)
		{
			Chars = FcbArray[Fp].FileSize - FcbArray[Fp].Pos;
		}
		
		memcpy(pBuf, pBlockBuf, Chars);
		//pBuf += Chars;
		OffsetInSector = 0;
		FcbArray[Fp].Pos += Chars;
		Reads += Chars;
		BytesToRead -= Chars;	
	}
	
	return Reads;
}

//
// ���ļ���һ���ֽ�
//INT16 FatReadFileBase(const FHANDLE Fp, const UINT32 Offset, PUCHAR Byte)
//{
//	UCHAR Buf[SECTOR_SIZE];
//	UINT16 OffsetInSector;
//	UINT16 iFatEntry;
//	int i;
//	
//	// ƫ�ƴ����ļ���Сֱ���˳�
//	if(Offset >= FcbArray[Fp].FileSize)
//	{
//		return FILE_EOF;
//	}
//	
//	// �����ֽ����ڵĴ��̿�
//	iFatEntry = FcbArray[Fp].FirstCluster;
//	for(i = Offset / SECTOR_SIZE; i > 0; i--)
//	{
//		iFatEntry = ReadFatEntryValue(iFatEntry);
//	}
//	// �ֽ����ڵĴ��̿�ƫ��
//	OffsetInSector = Offset % SECTOR_SIZE;
//	ReadDataSector(iFatEntry, Buf);
//	// ��ȡһ���ֽ�
//	*Byte = Buf[OffsetInSector];
//	
//	return 0;
//}

//
// �ر��ļ�
VOID FatCloseFile(const FHANDLE Fp)
{
	if(FcbArray[Fp].FileMode == 0)
	{
		;
	}
	
	memset(&FcbArray[Fp], 0, FCB_SIZE);
}

//
// ���ļ�
FHANDLE FatOpenFile(const PCHAR FileName, const UINT16 FileMode)
{
	UINT16 iRootDirEntry;
	DIRENT DirEnt;
	FHANDLE Fp;
	
	if((iRootDirEntry = FindFileInRootDir(FileName)) == (UINT16)-1)
	{
		return -1;
	}
	else
	{
		Fp = FindEmptyFCB();
		if(Fp == -1)
		{
			return -1;
		}
		
		ReadRootDirEntryValue(iRootDirEntry, &DirEnt);
		DosFileName2FileName(DirEnt.Name, FcbArray[Fp].Name);
		FcbArray[Fp].Attributes 	= DirEnt.Attributes;
		FcbArray[Fp].LastWriteTime 	= DirEnt.LastWriteTime;
		FcbArray[Fp].LastWriteDate 	= DirEnt.LastWriteDate;
		FcbArray[Fp].FirstCluster 	= DirEnt.FirstCluster;
		FcbArray[Fp].FileSize 		= DirEnt.FileSize;
		FcbArray[Fp].FileMode 		= FileMode;
		FcbArray[Fp].iRootDirEntry 	= iRootDirEntry;
		
		return Fp;
	}
}

//
// ���ҿ����ļ���
FHANDLE FindEmptyFCB(VOID)
{
	FHANDLE Fp;
	
	for(Fp = 0; Fp < FCB_MAX; Fp++)
	{
		if(*(PUCHAR)&FcbArray[Fp] == 0)
		{
			return Fp;
		}
	}
	
	return -1;
}

//
// ɾ���ļ�
VOID DeleteFile(const UINT16 iRootDirEntry)
{
    DIRENT DirEnt;
    UINT16 iCurFatEntry, iNextFatEntry;
    UCHAR BlockBuf[SECTOR_SIZE];
    
    memset(BlockBuf, 0, SECTOR_SIZE);
    
    ReadRootDirEntryValue(iRootDirEntry, &DirEnt);
    iCurFatEntry = DirEnt.FirstCluster;
    if(iCurFatEntry != 0) // �ǿ��ļ�ֻ��ɾ��DirEnt
    {
        do
        {
            // �����
            WriteDataSector(iCurFatEntry, BlockBuf);
            // ��ȡ��һ���ڵ�
            iNextFatEntry = ReadFatEntryValue(iCurFatEntry);
            // �ͷ�iFatEntry
            WriteFatEntryValue(iCurFatEntry, 0);
            iCurFatEntry = iNextFatEntry;
        }while(iCurFatEntry < 0xFF8);
    }
    
    // ɾ��DirEnt
    memset(&DirEnt, 0, ROOT_DIR_ENTRY_BYTE); 
    // ע: DOSֻ��DirEnt�ĵ�һ���ֽ�����Ϊ��0xE5
    WriteRootDirEntryValue(iRootDirEntry, &DirEnt);
}

// 
// ��ȡROOTDir�����ҵ��ĵ�һ������
UINT16 FindEmptyRootDirEntry(VOID)
{
    DIRENT DirEnt1, DirEnt2;
    UINT16 iRootDirEntry;
    
    memset(&DirEnt1, 0, ROOT_DIR_ENTRY_BYTE);
    
    for(iRootDirEntry = 0; iRootDirEntry < ROOT_DIR_ENTRY_NUM; 
                            iRootDirEntry++)
    {
        ReadRootDirEntryValue(iRootDirEntry, &DirEnt2);
        if(!memcmp(&DirEnt1, &DirEnt2, ROOT_DIR_ENTRY_BYTE) 
            || *((PUCHAR)&DirEnt2) == 0xE5) // ע: DOSɾ���ļ�ֻ��DirEnt��
                                            //     ��һ���ֽ�����Ϊ0xE5
        {
            return iRootDirEntry;
        }
    }
    
    return (UINT16)-1;
}

//
// ��ȡFat�����ҵ��ĵ�һ������
UINT16 FindEmptyFatEntry(const UINT16 iOriginFatEntry)
{
    UINT16 iFatEntry;
    
    for(iFatEntry = iOriginFatEntry; 
                    iFatEntry < DATA_SECTOR_NUM + FIRSTENTRY;
                    iFatEntry++) // iFatEntry��2��ʼ���Լ�FIRSTENTRY
    {
        if(ReadFatEntryValue(iFatEntry) == 0)
        {
            return iFatEntry;
        }
    }
    
    return (UINT16)-1; 
}

//
// �ڸ�Ŀ¼���в����ļ�
UINT16 FindFileInRootDir(const PCHAR FileName)
{
    PDIRENT pDirEnt;
    CHAR DosFileName[FILE_NAME_STR_LEN + 1];
    UINT16 iRootDirEntry;
    
    FileName2DosFileName(FileName, DosFileName);
    pDirEnt = (PDIRENT)&ROOTDirBuf[0];
    for(iRootDirEntry = 0; iRootDirEntry < ROOT_DIR_ENTRY_NUM; 
                            iRootDirEntry++, pDirEnt++)
    {
        if(!memcmp(pDirEnt->Name, DosFileName, FILE_NAME_STR_LEN))
        {
            return iRootDirEntry;
        }
    }
    
    return (UINT16)-1;
}

//
// ��ȡʣ����̿ռ�
UINT16 GetRestSector(VOID)
{
    UINT16 iCurFatEntry = FIRSTENTRY, RestSectors = 0;
    
    for(/*empty*/; iCurFatEntry < DATA_SECTOR_NUM + FIRSTENTRY; 
                    iCurFatEntry++)
    {
        if(ReadFatEntryValue(iCurFatEntry) == 0)
        {
            RestSectors++;
        }
    }
    
    return RestSectors;
}

//
// ���ļ�����8+3��ͨ�ļ���
VOID DosFileName2FileName(const PCHAR DosFileName, PCHAR FileName)
{
    int i;
    int fNameEnd = FILE_NAME_LEN - 1; // 7
    int fExtEnd = FILE_NAME_STR_LEN - 1; // 10
    
    // Name
    while(DosFileName[fNameEnd] == SPACE) // ȥ���ļ����ͺ�׺֮����ܵĿո�
    {
        fNameEnd--;
    }
    for(i = 0; i <= fNameEnd; i++)
    {
        FileName[i] = DosFileName[i];
    }
    
    // Ext
    while(DosFileName[fExtEnd] == SPACE)
    {
        fExtEnd--;
    }
    if(fExtEnd >= FILE_NAME_LEN) // ��׺ռ��8 9 10
    {
        FileName[++fNameEnd] = '.';
        for(i = FILE_NAME_LEN; i <= fExtEnd; i++)
        {
            FileName[++fNameEnd] = DosFileName[i];
        }
    }
    
    FileName[++fNameEnd] = NUL;
    // �ַ���תСд
    for(i = 0; i < fNameEnd; i++)
    {
        FileName[i] = tolower(FileName[i]);
    }
}

//
// ����ͨ�ļ���תΪ8+3��ʽ
VOID FileName2DosFileName(const PCHAR FileName, PCHAR DosFileName)
{
    int i, iExt, FileNameLen;
    PCHAR pExt;
    // ȡ���һ��'.'��λ��
    pExt = strrchr(FileName, '.');
    // �Ҳ���'.'��ʾû�к�׺��
    if(NULL == pExt)
    {
        for(i = 0; i < FILE_NAME_LEN; i++)
        {
            if(FileName[i] == NUL)
            {
                break;
            }
            DosFileName[i] = FileName[i];
        }
        // ���
        for(/*enpty*/; i < FILE_NAME_STR_LEN; i++)
        {
            DosFileName[i] = SPACE;
        }
    }
    else
    {
        // �ļ����ַ�����
        FileNameLen = pExt - FileName;
        for(i = 0; i < FileNameLen && i < FILE_NAME_LEN; i++)
        {
            DosFileName[i] = FileName[i];
        }
        // ���
        for(/*enpty*/; i < FILE_NAME_STR_LEN; i++)
        {
            DosFileName[i] = SPACE;
        }
        
        iExt = ++FileNameLen;
        // ��׺
        for(i = FILE_NAME_LEN; i < FILE_NAME_STR_LEN; 
                                i++, iExt++)
        {
            if(FileName[iExt] == NUL)
            {
                break;
            }
            DosFileName[i] = FileName[iExt];
        }
        // ���
        for(/*enpty*/; i < FILE_NAME_STR_LEN; i++)
        {
            DosFileName[i] = SPACE;
        }
    }
    
    // �ַ���ת��д
    for(i = 0; i < FILE_NAME_STR_LEN; i++)
    {
        DosFileName[i] = toupper(DosFileName[i]);
    }
    
    DosFileName[FILE_NAME_STR_LEN] = NUL; // ???
}

//
// ��ȡһ��iRootDirEntry��ֵ
UINT16 ReadRootDirEntryValue(const UINT16 iRootDirEntry, PDIRENT pDirEnt)
{
    if(iRootDirEntry < ROOT_DIR_ENTRY_NUM)
    {
        *pDirEnt = *((PDIRENT)
                    &ROOTDirBuf[iRootDirEntry * ROOT_DIR_ENTRY_BYTE]);
        return 0;
    }
    else
    {
        return (UINT16)-1;
    }
}

//
// ��iRootDirEntryд��ֵ
UINT16 WriteRootDirEntryValue(const UINT16 iRootDirEntry, 
                              const PDIRENT pDirEnt)
{
    if(iRootDirEntry < ROOT_DIR_ENTRY_NUM)
    {
        *((PDIRENT)&ROOTDirBuf[iRootDirEntry * ROOT_DIR_ENTRY_BYTE])
        = *pDirEnt;
        
        return 0;
    }
    else
    {
        return (UINT16)-1;
    }
}

//
// ��ȡ��ǰiFatEntry��ֵ
UINT16 ReadFatEntryValue(const UINT16 iFatEntry)
{
    UINT16 ValueTmp, FatEntryByteOff;
    
    // iFatEntry��FATBuf�е�ƫ��(ÿ��iFatEntryռ��12λ)
    FatEntryByteOff = iFatEntry * 3 / 2;
    
    ValueTmp = ReadValueToFatBuf(FatEntryByteOff);
    
    return (iFatEntry % 2 == 0 
            ? (ValueTmp & 0x0FFF) 
            : (ValueTmp >> 4 & 0x0FFF));
}

//
// ���õ�ǰiFatEntry��ֵ
VOID WriteFatEntryValue(const UINT16 iFatEntry, UINT16 Value)
{
    UINT16 ValueTmp, FatEntryByteOff;
    
    // iFatEntry��FATBuf�е�ƫ��(ÿ��iFatEntryռ��12λ)
    FatEntryByteOff = iFatEntry * 3 / 2;
    
    ValueTmp = ReadValueToFatBuf(FatEntryByteOff);
    
    // ż��Ҫ����ԭֵ��4λ,����Ҫ����ԭֵ����λ
    if(iFatEntry % 2 == 0)
    {
        Value = Value & 0x0FFF | ValueTmp & 0xF000;
    }
    else
    {
        Value = Value << 4 & 0xFFF0 | ValueTmp & 0x000F;
    }
    
    // ��ֵд��FAT������
    WriteValueToFatBuf(FatEntryByteOff, Value);
}

//
// ��FAT�����ж�ȡֵ
UINT16 ReadValueToFatBuf(const UINT16 FatEntryByteOff)
{
    UINT16 Value = *((PUINT16)&FAT2Buf[FatEntryByteOff]);
    
    if(Value == *((PUINT16)&FAT1Buf[FatEntryByteOff]))
    {
        return Value;
    }
    else
    {
        return (UINT16)-1;
    }
}

//
// ��ֵд��FAT����
VOID WriteValueToFatBuf(const UINT16 FatEntryByteOff, const UINT16 Value)
{
    *((PUINT16)&FAT1Buf[FatEntryByteOff]) = Value;
    *((PUINT16)&FAT2Buf[FatEntryByteOff]) = Value;
}

//
// ��iFatEntry��Ӧ�����ݴ��̿�����
UINT16 ReadDataSector(const UINT16 iFatEntry, PUCHAR SectorBuf)
{
    if(iFatEntry >= FIRSTENTRY 
                    && iFatEntry < DATA_SECTOR_NUM + FIRSTENTRY)
    {
        ReadBlock(DATA_START_SECTOR + iFatEntry - FIRSTENTRY, 
                    SectorBuf);
        return 0;
    }
    else
    {
        return (UINT16)-1;
    }
}

//
// ��iFatEntry��Ӧ�����ݴ��̿�д����
UINT16 WriteDataSector(const UINT16 iFatEntry, const PUCHAR SectorBuf)
{
    if(iFatEntry >= FIRSTENTRY 
                    && iFatEntry < DATA_SECTOR_NUM + FIRSTENTRY)
    {
        WriteBlock(DATA_START_SECTOR + iFatEntry - FIRSTENTRY, 
                    SectorBuf);
        return 0;
    }
    else
    {
        return (UINT16)-1;
    }
}