# You can remove libraries from this if you disable in Config.h:
# * Composite (remove -lXcomposite)
# * Xft (-lXft -lfreetype -lfontconfig, -I/usr/X11R6/include/freetype2)
# * Pixmaps (-lXpm)
LIBS	= -L/usr/X11R6/lib -lXext -lX11 -lXt -lXmu -lSM -lICE -lm -lXcomposite -lXpm -lXft -lfreetype -lz -lfontconfig
INCS	= -I/usr/X11R6/include -I/usr/X11R6/include/freetype2

PREFIX	= /usr/local
MANDIR	= $(PREFIX)/share/man/1
BINDIR	= $(PREFIX)/bin
APPLDIR	= $(PREFIX)/share/applications

CCC	= g++
CFLAGS	= -O2 -g -Wall $(INCS)
OBJECTS	= Border.o Buttons.o Channel.o Client.o Config.o Events.o Main.o Manager.o Menu.o Rotated.o Session.o

.cpp.o:
	$(CCC) -c $(CFLAGS) $<

wmx:	$(OBJECTS)
	$(CCC) -o wmx $(OBJECTS) $(LIBS)

install: wmx
	install wmx $(BINDIR)
	install wmx.1 $(MANDIR)
	install wmx.desktop $(APPLDIR)

clean:
	rm -f *.o core

distclean: clean
	rm -f wmx wmx.old *~

