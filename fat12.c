/*
	(C) Mumu3w@outlook.com
	
	提供向/从fat12image写入或读取文件
	
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
// 获取文件长度
UINT32 GetFileLen(FILE *Fp)
{
    UINT32 FileLen, FilePos;
    
    FilePos = ftell(Fp); // 保存当前位置
    
    fseek(Fp, 0L, SEEK_END);
    FileLen = ftell(Fp); // 计算文件长度
    
    fseek(Fp, FilePos, SEEK_SET);
    
    return FileLen;
}

// 
// 从镜像中删除文件
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
// 从镜像复制文件
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
// 添加文件到镜像
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
    
    // 存在同名文件则删除
    iRootDirEntry = FindFileInRootDir(FileName);
    if(iRootDirEntry != (UINT16)-1)
    {
        DeleteFile(iRootDirEntry);
    }
    else // 不存在同名文件则要判断是否还有空DirEntry
    {
        if(FindEmptyRootDirEntry() == (UINT16)-1)
        {
            fclose(NewFp);
            return (UINT16)-1;
        }
    }
    
    // 空间是否足够
    FileLen = GetFileLen(NewFp);
    if(FileLen > (GetRestSector() * SECTOR_SIZE)) 
    {
        fclose(NewFp);
        return (UINT16)-1;
    }
    // 普通字符串转dos文件名
    FileName2DosFileName(FileName, DosFileName);
    // 添加文件
    AddFileBase(NewFp, DosFileName);
    
    fclose(NewFp);
    return 0;
}

//
// 从镜像读取文件
UINT16 CopyFileBase(FILE *NewFp, const PCHAR FileName)
{
    UCHAR BlockBuf[SECTOR_SIZE];
    UINT32 FileLen;
    UINT16 iRootDirEntry;
    UINT16 iCurFatEntry, iNextFatEntry;
    DIRENT DirEnt;
    
    // 查找文件
    iRootDirEntry = FindFileInRootDir(FileName);
    if(iRootDirEntry != (UINT16)-1)
    {
        ReadRootDirEntryValue(iRootDirEntry, &DirEnt); // 获取DirEnt项
        FileLen = DirEnt.FileSize; // 文件长度
        iCurFatEntry = DirEnt.FirstCluster; // 第一个iFatEntry
        while(iCurFatEntry != 0 && iCurFatEntry < 0xFF0)
        {
            ReadDataSector(iCurFatEntry, BlockBuf);
            if(FileLen < SECTOR_SIZE) // 对最后一个扇区不足SECTOR_SIZE做特殊处理
            {
                fwrite(BlockBuf, sizeof(UCHAR), FileLen, NewFp);
            }
            else
            {
                fwrite(BlockBuf, sizeof(UCHAR), SECTOR_SIZE, NewFp);
            }
            
            // 下一个iFatEntry
            iNextFatEntry = ReadFatEntryValue(iCurFatEntry);
            iCurFatEntry = iNextFatEntry;
            FileLen -= SECTOR_SIZE; // 还剩下多少字节需要拷贝
        }
        
        return 0;
    }
    else
    {
        return (UINT16)-1;
    }
}

//
// 添加文件到镜像
UINT16 AddFileBase(FILE *NewFp, const PCHAR DosFileName)
{
    UCHAR BlockBuf[SECTOR_SIZE];
    UINT16 SectorLists[DATA_SECTOR_NUM];
    UINT16 SectorNums, ExtBytes, iFinalSector, iRootDirEntry;
    UINT32 FileLen;
    DIRENT DirEnt;
    int i;
    
    // 文件长度
    // fseek(NewFp, 0L, SEEK_END);
    // FileLen = ftell(NewFp);
    FileLen = GetFileLen(NewFp);

    SectorLists[0] = 0; // 空文件FirstCluster为0
    if(FileLen > 0)
    {
        ExtBytes = FileLen % SECTOR_SIZE;
        SectorNums = (FileLen / SECTOR_SIZE) + (ExtBytes != 0 ? 1 : 0);
        iFinalSector = SectorNums - 1;
        
        // iFatEntry链表分配
        SectorLists[0] = FindEmptyFatEntry(FIRSTENTRY);
        for(i = 1; i < SectorNums; i++)
        {
            SectorLists[i] = FindEmptyFatEntry(SectorLists[i-1] + 1);
            WriteFatEntryValue(SectorLists[i-1], SectorLists[i]);
        }
        WriteFatEntryValue(SectorLists[iFinalSector], 0xFFF);
        
        // 记得将位置设置为文件开头
        // fseek(NewFp, 0L, SEEK_SET);
        
        // 拷贝扇区数据, 最后一个扇区要特殊处理
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
    
    // iRootDirEntry分配与设置
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
// 读逻辑块
INT32 ReadBlock(const UINT32 Lba, PUCHAR BlockBuf)
{
    fseek(ImgFp, Lba * SECTOR_SIZE, SEEK_SET);
    return fread(BlockBuf, sizeof(UCHAR), SECTOR_SIZE, ImgFp);
}

//
// 写逻辑块
INT32 WriteBlock(const UINT32 Lba, const PUCHAR BlockBuf)
{
    fseek(ImgFp, Lba * SECTOR_SIZE, SEEK_SET);
    return fwrite(BlockBuf, sizeof(UCHAR), SECTOR_SIZE, ImgFp);
}

/********************************************************************

                    FAT12 FileSystem 初始化
    
********************************************************************/
//
// 初始化文件系统
VOID InitFat12FileSystem(const PUCHAR ImageFileName)
{
    // 打开磁盘镜像文件
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
// 释放文件系统
VOID FreeFat12FileSystem(VOID)
{
    UpdateRootDirEntryBuf();
    UpdateFatEntry();
    fclose(ImgFp);
}

//
// 加载FAT表到缓冲
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
// 将FAT表写回到文件
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
// 加载RootDir表到缓冲
VOID LoadRootDirEntryBuf(VOID)
{
    int i;
    
    for(i = 0; i < ROOT_DIR_SECTOR_NUM; i++)
    {
        ReadBlock(ROOT_DIR_START_SECTOR + i, &ROOTDirBuf[i * SECTOR_SIZE]);
    }
}

//
// 将RootDir表写回到文件
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
// 文件块读
//UINT32 FatReadFile(const FHANDLE Fp, const UINT32 BytesToRead, PVOID Buf)
//{
//	UINT32 ByteRead = 0; // 实际读取的字节数
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
// 读文件块
UINT32 FatReadFile(const FHANDLE Fp, UINT32 BytesToRead, PVOID Buf)
{
	UINT16 OffsetInSector;
	UINT16 iFatEntry; // 其实这是块号
	UINT16 Chars;
	UINT32 Reads = 0;
	PUCHAR pBuf = (PUCHAR)Buf;
	UCHAR BlockBuf[SECTOR_SIZE];
	PUCHAR pBlockBuf;
	int i;
	
	// 偏移大于文件大小直接退出
	if(FcbArray[Fp].Pos >= FcbArray[Fp].FileSize)
	{
		return FILE_EOF;
	}
	//FcbArray[Fp].Pos = Offset;
	
	// 计算字节所在的磁盘块
	iFatEntry = FcbArray[Fp].FirstCluster;
	for(i = FcbArray[Fp].Pos / SECTOR_SIZE; i > 0; i--)
	{
		iFatEntry = ReadFatEntryValue(iFatEntry);
	}
	// 字节所在的磁盘块偏移
	OffsetInSector = FcbArray[Fp].Pos % SECTOR_SIZE;
	
	// 实际可读的字节数受文件长度的限制
	if(BytesToRead > FcbArray[Fp].FileSize - FcbArray[Fp].Pos)
	{
		BytesToRead = FcbArray[Fp].FileSize - FcbArray[Fp].Pos;
	}
	
	while(BytesToRead > 0)
	{
		Chars = SECTOR_SIZE - OffsetInSector; // 本磁盘块需要读取的字节数
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
		
		// 最后一个块只读取有效的部分
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
// 从文件读一个字节
//INT16 FatReadFileBase(const FHANDLE Fp, const UINT32 Offset, PUCHAR Byte)
//{
//	UCHAR Buf[SECTOR_SIZE];
//	UINT16 OffsetInSector;
//	UINT16 iFatEntry;
//	int i;
//	
//	// 偏移大于文件大小直接退出
//	if(Offset >= FcbArray[Fp].FileSize)
//	{
//		return FILE_EOF;
//	}
//	
//	// 计算字节所在的磁盘块
//	iFatEntry = FcbArray[Fp].FirstCluster;
//	for(i = Offset / SECTOR_SIZE; i > 0; i--)
//	{
//		iFatEntry = ReadFatEntryValue(iFatEntry);
//	}
//	// 字节所在的磁盘块偏移
//	OffsetInSector = Offset % SECTOR_SIZE;
//	ReadDataSector(iFatEntry, Buf);
//	// 获取一个字节
//	*Byte = Buf[OffsetInSector];
//	
//	return 0;
//}

//
// 关闭文件
VOID FatCloseFile(const FHANDLE Fp)
{
	if(FcbArray[Fp].FileMode == 0)
	{
		;
	}
	
	memset(&FcbArray[Fp], 0, FCB_SIZE);
}

//
// 打开文件
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
// 查找空闲文件块
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
// 删除文件
VOID DeleteFile(const UINT16 iRootDirEntry)
{
    DIRENT DirEnt;
    UINT16 iCurFatEntry, iNextFatEntry;
    UCHAR BlockBuf[SECTOR_SIZE];
    
    memset(BlockBuf, 0, SECTOR_SIZE);
    
    ReadRootDirEntryValue(iRootDirEntry, &DirEnt);
    iCurFatEntry = DirEnt.FirstCluster;
    if(iCurFatEntry != 0) // 是空文件只用删除DirEnt
    {
        do
        {
            // 块清空
            WriteDataSector(iCurFatEntry, BlockBuf);
            // 获取下一个节点
            iNextFatEntry = ReadFatEntryValue(iCurFatEntry);
            // 释放iFatEntry
            WriteFatEntryValue(iCurFatEntry, 0);
            iCurFatEntry = iNextFatEntry;
        }while(iCurFatEntry < 0xFF8);
    }
    
    // 删除DirEnt
    memset(&DirEnt, 0, ROOT_DIR_ENTRY_BYTE); 
    // 注: DOS只将DirEnt的第一个字节设置为了0xE5
    WriteRootDirEntryValue(iRootDirEntry, &DirEnt);
}

// 
// 获取ROOTDir表中找到的第一条空项
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
            || *((PUCHAR)&DirEnt2) == 0xE5) // 注: DOS删除文件只将DirEnt的
                                            //     第一个字节设置为0xE5
        {
            return iRootDirEntry;
        }
    }
    
    return (UINT16)-1;
}

//
// 获取Fat表中找到的第一条空项
UINT16 FindEmptyFatEntry(const UINT16 iOriginFatEntry)
{
    UINT16 iFatEntry;
    
    for(iFatEntry = iOriginFatEntry; 
                    iFatEntry < DATA_SECTOR_NUM + FIRSTENTRY;
                    iFatEntry++) // iFatEntry从2开始所以加FIRSTENTRY
    {
        if(ReadFatEntryValue(iFatEntry) == 0)
        {
            return iFatEntry;
        }
    }
    
    return (UINT16)-1; 
}

//
// 在根目录表中查找文件
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
// 获取剩余磁盘空间
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
// 将文件名从8+3普通文件名
VOID DosFileName2FileName(const PCHAR DosFileName, PCHAR FileName)
{
    int i;
    int fNameEnd = FILE_NAME_LEN - 1; // 7
    int fExtEnd = FILE_NAME_STR_LEN - 1; // 10
    
    // Name
    while(DosFileName[fNameEnd] == SPACE) // 去处文件名和后缀之间可能的空格
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
    if(fExtEnd >= FILE_NAME_LEN) // 后缀占用8 9 10
    {
        FileName[++fNameEnd] = '.';
        for(i = FILE_NAME_LEN; i <= fExtEnd; i++)
        {
            FileName[++fNameEnd] = DosFileName[i];
        }
    }
    
    FileName[++fNameEnd] = NUL;
    // 字符串转小写
    for(i = 0; i < fNameEnd; i++)
    {
        FileName[i] = tolower(FileName[i]);
    }
}

//
// 将普通文件名转为8+3格式
VOID FileName2DosFileName(const PCHAR FileName, PCHAR DosFileName)
{
    int i, iExt, FileNameLen;
    PCHAR pExt;
    // 取最后一个'.'的位置
    pExt = strrchr(FileName, '.');
    // 找不到'.'表示没有后缀名
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
        // 填充
        for(/*enpty*/; i < FILE_NAME_STR_LEN; i++)
        {
            DosFileName[i] = SPACE;
        }
    }
    else
    {
        // 文件名字符个数
        FileNameLen = pExt - FileName;
        for(i = 0; i < FileNameLen && i < FILE_NAME_LEN; i++)
        {
            DosFileName[i] = FileName[i];
        }
        // 填充
        for(/*enpty*/; i < FILE_NAME_STR_LEN; i++)
        {
            DosFileName[i] = SPACE;
        }
        
        iExt = ++FileNameLen;
        // 后缀
        for(i = FILE_NAME_LEN; i < FILE_NAME_STR_LEN; 
                                i++, iExt++)
        {
            if(FileName[iExt] == NUL)
            {
                break;
            }
            DosFileName[i] = FileName[iExt];
        }
        // 填充
        for(/*enpty*/; i < FILE_NAME_STR_LEN; i++)
        {
            DosFileName[i] = SPACE;
        }
    }
    
    // 字符串转大写
    for(i = 0; i < FILE_NAME_STR_LEN; i++)
    {
        DosFileName[i] = toupper(DosFileName[i]);
    }
    
    DosFileName[FILE_NAME_STR_LEN] = NUL; // ???
}

//
// 获取一条iRootDirEntry的值
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
// 向iRootDirEntry写入值
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
// 获取当前iFatEntry的值
UINT16 ReadFatEntryValue(const UINT16 iFatEntry)
{
    UINT16 ValueTmp, FatEntryByteOff;
    
    // iFatEntry在FATBuf中的偏移(每项iFatEntry占用12位)
    FatEntryByteOff = iFatEntry * 3 / 2;
    
    ValueTmp = ReadValueToFatBuf(FatEntryByteOff);
    
    return (iFatEntry % 2 == 0 
            ? (ValueTmp & 0x0FFF) 
            : (ValueTmp >> 4 & 0x0FFF));
}

//
// 设置当前iFatEntry的值
VOID WriteFatEntryValue(const UINT16 iFatEntry, UINT16 Value)
{
    UINT16 ValueTmp, FatEntryByteOff;
    
    // iFatEntry在FATBuf中的偏移(每项iFatEntry占用12位)
    FatEntryByteOff = iFatEntry * 3 / 2;
    
    ValueTmp = ReadValueToFatBuf(FatEntryByteOff);
    
    // 偶数要保留原值高4位,奇数要保留原值低四位
    if(iFatEntry % 2 == 0)
    {
        Value = Value & 0x0FFF | ValueTmp & 0xF000;
    }
    else
    {
        Value = Value << 4 & 0xFFF0 | ValueTmp & 0x000F;
    }
    
    // 将值写到FAT缓冲中
    WriteValueToFatBuf(FatEntryByteOff, Value);
}

//
// 从FAT缓冲中读取值
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
// 将值写入FAT缓冲
VOID WriteValueToFatBuf(const UINT16 FatEntryByteOff, const UINT16 Value)
{
    *((PUINT16)&FAT1Buf[FatEntryByteOff]) = Value;
    *((PUINT16)&FAT2Buf[FatEntryByteOff]) = Value;
}

//
// 读iFatEntry对应的数据磁盘块内容
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
// 向iFatEntry对应的数据磁盘块写内容
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