RM		= -del
CC		= gcc
CFLAGS	= -DDEBUG -Werror -I.
LDFLAGS	= 

APP		= fat12.exe
OBJS	= fat12.o
FINDFILE= HELLO.EXE
IMAGE	= ../fat12.img


.PHONY: all clean
all : $(APP)
	@$(APP) $(FINDFILE) $(IMAGE)

$(APP) : $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)

fat12.o : fat12.c fat12.h def.h

%o : %c

clean :
	@$(RM) $(OBJS) $(APP) $(FINDFILE)