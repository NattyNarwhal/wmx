
LIBS	= -L/usr/X11R6/lib -lXext -lX11 -lXt -lXmu -lSM -lICE -lm -lXcomposite -lXpm -lXft -lfreetype -lz -lfontconfig
INCS	= -I/usr/X11R6/include -I/usr/X11R6/include/freetype2

CCC	= g++
CFLAGS	= -O2 -g -Wall $(INCS)
OBJECTS	= Border.o Buttons.o Channel.o Client.o Config.o Events.o Main.o Manager.o Menu.o Rotated.o Session.o

.cpp.o:
	$(CCC) -c $(CFLAGS) $<

wm2:	$(OBJECTS)
	$(CCC) -o wmx $(OBJECTS) $(LIBS)

depend:
	makedepend -- $(CFLAGS) -- *.cpp

clean:
	rm -f *.o core

distclean: clean
	rm -f wmx wmx.old *~

