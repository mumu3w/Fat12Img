#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "_fat12.h"


void PrintUsage(const char *str1);
int CopyFileFromImage(const PCHAR PathName);



int main(int argc, char *argv[])
{
	if((argc == 1) || ((argc == 2) && (!stricmp(argv[1], "-h"))))
	{
		PrintUsage(argv[0]);
		return 0;
	}
	
    if(argc == 4 && (!stricmp(argv[1], "-a")))
    {
		InitFat12FileSystem(argv[2]);

		AddFile2Image(argv[3]);
		
		FreeFat12FileSystem();
    }
	
	if(argc == 4 && (!stricmp(argv[1], "-c")))
    {
		InitFat12FileSystem(argv[2]);

		CopyFileFromImage(argv[3]);
		
		FreeFat12FileSystem();
    }
	
	if(argc == 4 && (!stricmp(argv[1], "-d")))
    {
		InitFat12FileSystem(argv[2]);

		DeleteFileFromImage(argv[3]);
		
		FreeFat12FileSystem();
    }

	return 0;
}

void PrintUsage(const char *str1)
{
	printf("(C) Mumu3w@outlook.com  %s  (Fat12Image 0.12)\n\n", __DATE__);
	printf("Usage: %s <-a | -c | -d | -h> Image [File]\n", str1);
	printf("  -a Add file to image\n");
	printf("  -c Copy file from image\n");
	printf("  -d Delete file from image\n");
	printf("  -h Show this usage\n");
}

//
// 从镜像复制文件
int CopyFileFromImage(const PCHAR PathName)
{
    FILE *NewFp;
    PCHAR FileName;
	FHANDLE Fp;
	char Buffer[512];
	int Bytes;
    
    if(NULL == (NewFp = fopen(PathName, "wb")))
    {
        fprintf(stderr, "%s Can't open\n", PathName);
        exit(EXIT_FAILURE);     
    }
    
    FileName = strrchr(PathName, SEPARATOR);
    FileName = ((NULL == FileName) ? PathName : (FileName + 1));
    
	Fp = FatOpenFile(FileName, 0);
	if(!(Fp < 0))
	{
		while(1)
		{
			Bytes = FatReadFile(Fp, sizeof(Buffer), Buffer);
			if(Bytes == -1)
			{
				break;
			}
			fwrite(Buffer, sizeof(char), Bytes, NewFp);
			if(Bytes != sizeof(Buffer))
			{
				break;
			}
		}
		
		FatCloseFile(Fp);
		fclose(NewFp);
		return 0;
	}
	
	return -1;
}