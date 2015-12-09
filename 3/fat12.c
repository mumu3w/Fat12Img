#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include "def.h"
#include "fat12.h"


int main(int argc, char *argv[])
{   
    DIRENT DirEnt;
    FILE *ImgFp;
    
    if(argc != 3)
    {
        fprintf(stderr, "usage: fat12 FileName ImgName\n");
        exit(EXIT_FAILURE);
    }
    
    if((ImgFp = fopen(argv[2], "rb")) == NULL)
    {
        fprintf(stderr, "%s Can't create\n", argv[2]);
        exit(EXIT_FAILURE);
    }
    
    if(FindEntry(argv[1], ImgFp, &DirEnt))
    {
#if defined(DEBUG)
        printf("File already exists: %s\n", argv[1]);
        printf("FileSize: %d bytes\n", DirEnt.FileSize);
        printf("FirstCluster: 0x%X\n", DirEnt.FirstCluster);
#endif
    }else{
#if defined(DEBUG)
        printf("File does not exist: %s\n", argv[1]);
#endif
    }
    
    fclose(ImgFp);
    return 0;
}

UINT32 ImgCopyFile(const CHAR *FileName, FILE *ImgFp)
{
	DIRENT DirEnt;
	
	if(!FindEntry(FileName, ImgFp, &DirEnt))
	{
		
	}
	
	return 0;
}

BOOL FindEntry(const CHAR *FileName, FILE *ImgFp, PDIRENT pDirEnt)
{
    UCHAR Buffer[SECTOR_SIZE];
    DIRENT DirEntSwap;
    PDIRENT pDirEntSwap;
    CHAR NameTmp[13] = {0};
    int i, j;
    
    for(i = ROOT_START_SECTOR; i <= ROOT_END_SECTOR; i++)
    {
        ImgReadBlock(i, ImgFp, Buffer);
        pDirEntSwap = (PDIRENT)Buffer;
        for(j = 0; j < SECTOR_SIZE / sizeof(DIRENT); j++)
        {
            DirEntSwap = *(pDirEntSwap + j);
            FatNameToFileName(NameTmp, DirEntSwap.Name, 
                                sizeof(DirEntSwap.Name));
            if(FileNameCmp(FileName, NameTmp))
            {
                *pDirEnt = DirEntSwap;
                return TRUE;
            }
        }
    }
    
    return FALSE;
}

void FatNameToFileName(CHAR *FileName, 
        const CHAR *FatName, 
        UINT32 FatNameSize)
{
    CHAR Name[9] = {0};
    CHAR Pos[4] = {0};
    CHAR ch;
    int i;
    
    for(i = 0; i < FatNameSize - 3; i++)
    {
        ch = *FatName++;
        if(isalnum(ch))
        {
            Name[i] = ch;
        }
    }
    
    for(i = 0; i < FatNameSize - 8; i++)
    {
        ch = *FatName++;
        if(isalnum(ch))
        {
            Pos[i] = ch;
        }else{
            break;
        }
    }
    
    strcpy(FileName, Name);
    if(strlen(Pos) != 0)
    {
        strcat(FileName, ".");
        strcat(FileName, Pos);
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

//
// 写逻辑块
//
INT32 ImgWriteBlock(UINT32 Lba, FILE *ImgFp, const UCHAR *Buffer)
{
    UINT32 ImgByteOffset;
    
    // 根据逻辑块号计算出所在Img文件的偏移量
    ImgByteOffset = Lba * SECTOR_SIZE;
    fseek(ImgFp, ImgByteOffset, SEEK_SET);
    
    return fwrite(Buffer, sizeof(UCHAR), SECTOR_SIZE, ImgFp);
}

//
// 读逻辑块
//
INT32 ImgReadBlock(UINT32 Lba, FILE *ImgFp, UCHAR *Buffer)
{
    UINT32 ImgByteOffset;
    
    // 根据逻辑块号计算出所在Img文件的偏移量
    ImgByteOffset = Lba * SECTOR_SIZE;
    fseek(ImgFp, ImgByteOffset, SEEK_SET);
    
    return fread(Buffer, sizeof(UCHAR), SECTOR_SIZE, ImgFp);
}