
LIBPATH=
WINDLLMAIN=
COMPILERFLAGS=-Wall
CC=gcc
LIBRARIES=-lusb -lXPLM 

HOSTOS=$(shell uname | tr A-Z a-z)
ifeq ($(HOSTOS),linux)
 LNFLAGS=-shared -rdynamic -nodefaultlibs -L.
 CFLAGS=$(COMPILERFLAGS) -DAPL=0 -DIBM=0 -DLIN=1
 LIBRARIES+=-lopengl -lglu -lpthread
else
  HOSTOS=windows
  LNFLAGS=-m32 -Wl,-O1 -shared -L. -L./SDK/Libraries/Win/
  CFLAGS=$(COMPILERFLAGS) -DAPL=0 -DIBM=1 -DLIN=0
  LIBPATH+=-L".\SDK\Libraries\Win" -L".\lib"
  LIBRARIES+=-lopengl32 -lglu32 -lpthreadGC1
endif

INCLUDE=-I./include
INCLUDE+=-I./SDK/CHeaders/XPLM


all:
	$(CC) -c $(INCLUDE) $(CFLAGS) log.c
	$(CC) -c $(INCLUDE) $(CFLAGS) properties.c
	$(CC) -c $(INCLUDE) $(CFLAGS) settings.c
	$(CC) -c $(INCLUDE) $(CFLAGS) thread.c
	$(CC) -c $(INCLUDE) $(CFLAGS) time.c
	$(CC) -c $(INCLUDE) $(CFLAGS) xplane.c
	$(CC) -c $(INCLUDE) $(CFLAGS) colomboard.c

	$(CC) -o flapindicator.xpl log.o properties.o settings.o thread.o time.o xplane.o colomboard.o  $(WINDLLMAIN) $(LNFLAGS) $(LIBPATH) $(LIBRARIES)

clean:
	$(RM) *.o *.xpl
