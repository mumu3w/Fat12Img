RM		= -del
CP		= copy
CC		= gcc
CFLAGS	= -g -DDEBUG -Werror -I.
LDFLAGS	= 


APPS	= Fat12Img.exe
OBJS	= fat12img.o fat12.o

.PHONY: all clean
all : $(APPS)

$(APPS) : $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)
	$(APPS) -a fat12.img ./Fat12Img.exe

fat12img.o : fat12img.c _fat12.h

fat12.o : fat12.c _fat12.h def.h

%o : %c

clean:
	@$(RM) $(APPS) $(OBJS)