
# wmx: another window manager

[![Build Status](https://travis-ci.org/NattyNarwhal/wmx.svg?branch=master)](https://travis-ci.org/NattyNarwhal/wmx)

wmx is another window manager for X.  It is based on wm2 and provides
a similarly unusual style of window decoration; but in place of wm2's
minimal functionality it offers many of the features of more
conventional managers, often in the most simplistic implementations
imaginable.

Documentation can be found in the man page - you can read it with man:

    # when wmx is installed:
    $ man wmx
    # GNU
    $ man ./wmx.1
    # BSD
    $ mandoc wmx.1 | less

## Building wmx

wmx should build on any Unix machine with X11. In particular, it uses
Xpm, Xkb, Freetype, and strl\* functions.

You might want to change the settings for wmx to suit your desires, and
edit the Makefile as needed for your platform. By default, it should work
out of the box on most OSes, but should be tweaked if you're not running a
typical Linux system. After that, you should be able to simply run make.

## xterm

Some versions of xterm and rxvt run badly with wmx.  If you use xterm
and find that it refreshes the window excessively slowly, you might
like to try experimenting with a different terminal emulation program.
I think it might help to ensure that the scrollbar is on the
right-hand side of the rxvt window and is thick enough that wmx's
resize handle doesn't obscure any of the text area.

## NETWM and desktop environments

As of this release, wmx now includes a significant amount of support
for the NETWM extended window manager hints, enough to make it
usable as the primary window manager with a GNOME or KDE desktop.
This support is incomplete; fixes and improvements will be welcomed
(more than bug reports will).

## Credits

wmx was written by Chris Cannam, recycling a lot of code and structure
from "9wm" by David Hogan.

The sideways tabs on the window frames were Andy Green's idea.

Alan Richardson's "xvertext" font-rotation routines are used for
the window tabs.

Kazushi (Jam) Marukawa provided internationalisation code, which I
think is currently only tested for Japanese; see README.contrib for
his notes and copyright notice.

Jeremy Fitzhardinge provided the original application-menu code.

The dynamic configuration code is mostly due to Stefan `Sec' Zehl.

Multiheaded X support is due to Sven Oliver `SvOlli' Moll.

NETWM support for use with desktop environments was based on a
substantial patch from James Montgomery sent to the wmx mailing list
in November 2000.  A mere seven years later, I got around to
integrating and updating the patch, and only eight years and two
months after the original patch, here it is in a release.  Sorry about
that.

Earlier Gnome support was mostly due to Henri Naccache, as is the
support for shaped clients.

This release contains code and bug fixes provided by Eric Marsden,
Lasse Rasinen, Bill Spitzak, Jacques Garrigue, Stefan `Sec' Zehl, Sven
Oliver Moll, Richard Sharman, Martin Andrews, Glyn Faulkner, Zvezdan
Petkovic, Damion Yates, Teemu Voipio, Ben Stern and, well, probably
several other people.

This wmx fork is currently maintained by Calvin Buckley.

If you want to hack the code into something else for your own
amusement, please go ahead.  Feel free to modify and redistribute, as
long as you retain the original copyrights as appropriate.


## Mailing list

There is a mailing list for discussion of wm2 and wmx, hosted by
majordomo at 42.org.  To subscribe, send email to majordomo@42.org
with "subscribe wmx" in the body of the mail.  The list is archived on
the web at http://ml.42.org/wmx/.

## Footer

The original wmx website can be found at
[Chris Cannam's website](http://www.all-day-breakfast.com/wmx/).

Calvin Buckley <calvin@openmailbox.org>, May 2014
