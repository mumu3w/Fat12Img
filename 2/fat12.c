#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include "def.h"
#include "fat12.h"


int main(int argc, char *argv[])
{
    FILE *ImgFp, *NewFp;
    DIRENT DirEnt;
    int i;
    
    if(argc != 3)
    {
        fprintf(stderr, "usage: fat12 FileName ImgName\n");
        exit(EXIT_FAILURE);
    }
    
    ImgFp = fopen(argv[2], "rb");
    if(ImgFp == NULL)
    {
        fprintf(stderr, "%s Can't create\n", argv[2]);
        exit(EXIT_FAILURE);
    }
    
    if(!FatOpenFile(argv[1], ImgFp, &DirEnt))
    {
        NewFp = fopen(argv[1], "wb");
        if(NewFp == NULL)
        {
            fprintf(stderr, "%s Can't create\n", argv[1]);
            exit(EXIT_FAILURE);
        }
        
        FatReadFile(ImgFp, NewFp, DirEnt);
        
        fclose(NewFp);
    }
    
    fclose(ImgFp);
    return 0;
}

INT32 FatReadFile(FILE *ImgFp, FILE *NewFp, DIRENT DirEnt)
{
    UCHAR Buffer[SECTOR_SIZE];
    UINT16 NextEntry = 0, CurrentEntry;
    UINT32 FileSize = DirEnt.FileSize;
    UINT16 ClusterNumber;
    
    CurrentEntry = DirEnt.FirstCluster;
    while(CurrentEntry != 0 && NextEntry < 0xFF0)
    {
#if defined(DEBUG)
        printf("ClusterNumber: 0x%X\n", CurrentEntry);
#endif
        ClusterNumber = DATA_START_ESCTOR + CurrentEntry - 2;
        ImgWriteBlock(ClusterNumber, ImgFp, Buffer);
        if(FileSize < SECTOR_SIZE)
        {
            fwrite(Buffer, sizeof(UCHAR), FileSize, NewFp);
        }else{
            fwrite(Buffer, sizeof(UCHAR), SECTOR_SIZE, NewFp);
        }
        FatGetNextCluster(ImgFp, CurrentEntry, &NextEntry);
        CurrentEntry = NextEntry;
        FileSize -= SECTOR_SIZE;
    }
#if defined(DEBUG)
    printf("ClusterNumber: 0x%X\n", CurrentEntry);
#endif
   
   return 0;
}

INT32 FatGetNextCluster(FILE *ImgFp, 
        UINT16 CurrentCluster, 
        UINT16 *NextCluster)
{
    UCHAR Buffer[SECTOR_SIZE];
    UINT16 SizeOffset;
    UINT16 Sectors, ExtraBytes;
    
    if(CurrentCluster % 2 == 0)
    {
        SizeOffset = CurrentCluster * 2 - CurrentCluster / 2;
        Sectors = SizeOffset / SECTOR_SIZE;
        ExtraBytes = SizeOffset % SECTOR_SIZE;
        ImgWriteBlock(Sectors + FAT1_START_SECTOR, ImgFp, Buffer);
        *NextCluster = *((UINT16 *)&Buffer[ExtraBytes]) & 0x0FFF;
    }else{
        SizeOffset = CurrentCluster * 2 - CurrentCluster / 2 - 1;
        Sectors = SizeOffset / SECTOR_SIZE;
        ExtraBytes = SizeOffset % SECTOR_SIZE;
        ImgWriteBlock(Sectors + FAT1_START_SECTOR, ImgFp, Buffer);
        *NextCluster = (*((UINT16 *)&Buffer[ExtraBytes]) >> 4) 
                        & 0x0FFF;
    }
    
    return 0;
}

INT32 FatOpenFile(const CHAR *FileName, FILE *ImgFp, PDIRENT pDirEnt)
{
    UCHAR Buffer[SECTOR_SIZE];
    DIRENT DirEntSwap;
    PDIRENT pDirEntSwap;
    UCHAR NameTmp[13] = {0};
    int i, j;

#if defined(DEBUG)  
    // 遍历根目录条目
    for(i = ROOT_START_SECTOR; i <= ROOT_END_SECTOR; i++)
    {
        ImgWriteBlock(i, ImgFp, Buffer);
        pDirEntSwap = (PDIRENT)Buffer;
        for(j = 0; j < SECTOR_SIZE / sizeof(DIRENT); j++)
        {
            DirEntSwap = *(pDirEntSwap + j);
            FileNameCpy(NameTmp, DirEntSwap.Name, sizeof(DirEntSwap.Name));
            if(strlen(NameTmp))
            {
                *pDirEnt = DirEntSwap;
                printf("SectorNumber: %d\n", i);
                printf("SectorOffset: %d\n", j);
                printf("FileName: %-12s\n", NameTmp);
                printf("FileSize: %d bytes\n", pDirEnt->FileSize);
                printf("FirstCluster: 0x%X\n", pDirEnt->FirstCluster);
            }
        }
    }
    printf("\n\n");
#endif
 
    // 搜索根目录条目
    for(i = ROOT_START_SECTOR; i <= ROOT_END_SECTOR; i++)
    {
        ImgWriteBlock(i, ImgFp, Buffer);
        pDirEntSwap = (PDIRENT)Buffer;
        for(j = 0; j < SECTOR_SIZE / sizeof(DIRENT); j++)
        {
            DirEntSwap = *(pDirEntSwap + j);
            FileNameCpy(NameTmp, DirEntSwap.Name, sizeof(DirEntSwap.Name));
            if(FileNameCmp(FileName, NameTmp))
            {
                *pDirEnt = DirEntSwap;
#if defined(DEBUG)
                printf("Find the file: %s\n", FileName);
                printf("SectorNumber: %d\n", i);
                printf("SectorOffset: %d\n", j);
                printf("FileName: %-12s\n", NameTmp);
                printf("FileSize: %d bytes\n", pDirEnt->FileSize);
                printf("FirstCluster: 0x%X\n", pDirEnt->FirstCluster);
#endif
                return 0;
            }
        }
    }
    
    return -1;
}

INT32 ImgWriteBlock(UINT32 Lba, FILE *ImgFp, UCHAR *Buffer)
{
    UINT32 ImgByteOffset;
    
    // 根据逻辑块号计算出所在Img文件的偏移量
    ImgByteOffset = Lba * SECTOR_SIZE;
    fseek(ImgFp, ImgByteOffset, SEEK_SET);
    
    return fread(Buffer, sizeof(UCHAR), SECTOR_SIZE, ImgFp);
}

void FileNameCpy(CHAR *FileName1, 
        const CHAR *FileName2, 
        UINT32 FileName2Size)
{
    CHAR Name[9] = {0};
    CHAR Pos[4] = {0};
    CHAR ch;
    int i;
    
    for(i = 0; i < FileName2Size - 3; i++)
    {
        ch = *FileName2++;
        if(isalnum(ch))
        {
            Name[i] = ch;
        }
    }
    
    for(i = 0; i < FileName2Size - 8; i++)
    {
        ch = *FileName2++;
        if(isalnum(ch))
        {
            Pos[i] = ch;
        }else{
            break;
        }
    }
    
    strcpy(FileName1, Name);
    if(strlen(Pos) != 0)
    {
        strcat(FileName1, ".");
        strcat(FileName1, Pos);
    }
}

BOOL FileNameCmp(const CHAR *FileName1, const CHAR *FileName2)
{
    if(!strcmp(FileName1, FileName2))
    {
        return TRUE;
    }
    
    return FALSE;
}

void Error(int status, const char *format, ...)
{
    va_list ap;

    va_start(ap, format);
    vfprintf(stderr, format, ap);
    va_end(ap);
    
    exit(status);
}