# You can remove libraries from this if you disable in Config.h:
# * Composite (remove -lXcomposite)
# * Xft (-lXft -lfreetype -lfontconfig, -I/usr/X11R6/include/freetype2)
# * Pixmaps (-lXpm)

PREFIX  = /usr/local
XPREFIX = /usr/X11R6
LIBDIR	= $(PREFIX)/lib
XLIBDIR	= $(XPREFIX)/lib
INCDIR	= $(PREFIX)/include
XINCDIR	= $(XPREFIX)/include
MANDIR	= $(PREFIX)/man/man1
BINDIR	= $(PREFIX)/bin
APPLDIR	= $(PREFIX)/share/applications

CLIBS	= -L. -L$(LIBDIR) -lportable 
CCLIBS	= -L. -L$(LIBDIR) -L$(XLIBDIR) -lXext -lX11 -lXt -lXmu -lSM -lICE -lm -lXcomposite -lXpm -lXft $(shell freetype-config --libs) -lfontconfig -lportable 
CINCS	= -I. -I$(INCDIR)
CCINCS	= -I. -I$(INCDIR) -I$(XINCDIR) $(shell freetype-config --cflags)

# Clang for wmx doesn't work as of yet
# Also, have to use g++ even for C as gcc/g++ can't link together
AR	= ar
CC	= g++
CCC	= $(CC)
CFLAGS	= -O2 -g -Wall $(CINCS) 
CCFLAGS	= -O2 -g -Wall $(CCINCS) 
# libobj is shared between the two
LIBOBJ	= Portable.o
OBJECTS	= Border.o Buttons.o Channel.o Client.o Config.o Events.o Keybinding.o Main.o Manager.o Menu.o Remote.o Rotated.o Session.o 
WMXCOBJ	= wmxc.o

.cpp.o:
	$(CCC) -c $(CCFLAGS) $<

.c.o:
	$(CC) -c $(CFLAGS) $<

wmx:	libportable.a $(OBJECTS) $(WMXCOBJ)
	$(CCC) -o wmx $(OBJECTS) $(CCLIBS)	# wmx proper
	$(CC) -o wmxc $(WMXCOBJ) $(CLIBS)	# wmxc

libportable.a:	$(LIBOBJ)
	$(AR) rcs libportable.a Portable.o

install: wmx
	install wmx $(BINDIR)
	install wmxc $(BINDIR)
	install wmx.1 $(MANDIR)
	install wmxc.1 $(MANDIR)
	install wmx.desktop $(APPLDIR)

clean:
	rm -f *.o *.core *.a wmx wmx.old wmxc

