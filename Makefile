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

CLIBS	= -L$(LIBDIR)
CCLIBS	= -L$(LIBDIR) -L$(XLIBDIR) -lXext -lX11 -lXt -lXmu -lSM -lICE -lm -lXcomposite -lXpm -lXft $(shell freetype-config --libs) -lfontconfig
CINCS	= -I$(INCDIR)
CCINCS	= -I$(INCDIR) -I$(XINCDIR) $(shell freetype-config --cflags)


# Clang for wmx doesn't work as of yet
CC	= gcc
CCC	= g++
# Add -DHAVE_STRLCAT and -DHAVE_STRLCPY if you don't running Linux - BSD has it!
PLTFLGS	= 
CFLAGS	= -O2 -g -Wall $(CINCS) $(PLTFLGS)
CCFLAGS	= -O2 -g -Wall $(CCINCS) $(PLTFLGS)
OBJECTS	= Border.o Buttons.o Channel.o Client.o Config.o Events.o Keybinding.o Main.o Manager.o Menu.o Portable.o Remote.o Rotated.o Session.o
WMXCOBJ	= wmxc.o Portable.o

.cpp.o:
	$(CCC) -c $(CCFLAGS) $<

.c.o:
	$(CC) -c $(CFLAGS) $<

wmx:	$(OBJECTS) $(WMXCOBJ)
	$(CCC) -o wmx $(OBJECTS) $(CCLIBS)	# wmx proper
	$(CC) -o wmxc $(WMXCOBJ) $(CLIBS)	# wmxc

install: wmx
	install wmx $(BINDIR)
	install wmxc $(BINDIR)
	install wmx.1 $(MANDIR)
	install wmxc.1 $(MANDIR)
	install wmx.desktop $(APPLDIR)

clean:
	rm -f *.o *.core wmx wmx.old wmxc

