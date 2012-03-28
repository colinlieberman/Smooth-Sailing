Smooth Sailing plugin for X-Plane 9
===================================

About
-----

This project is an X-plane plugin by [Colin Lieberman](http://www.cactusflower.org) for making some convenient changes to time and weather.

It's inspired by, and similar to, these two excellent projects:

* [Clear Skies](http://forums.x-plane.org/index.php?app=downloads&showfile=1728) by [dad](http://forums.x-plane.org/index.php?&showuser=7519)
* [FlyVFR](http://forums.x-plane.org/index.php?app=downloads&showfile=12661) by [X-Friese](http://forums.x-plane.org/index.php?showuser=82559)

The difference between this effort and those is that this one is focused on convenience only - doing things automatically so you don't have to tell the system how you want to change the wind, for example, and focusing solely on those changes.

The game plan for what I will build is detailed on the [specification page](https://github.com/colinlieberman/Smooth-Sailing/wiki/Spec).

How to Build
------------

I know the community is really into [QT Creator](http://qt.nokia.com/products/developer-tools/). Not me. I don't like some framework making my Makefile for me. I think the way for us to all play nicely together is to check in individual & platform specific Makefiles, eg Makefile-OSX10.5-i686.

You can symlink the checkedin Makefil that works for you to Makefile. That way you could stick a QT .pro in here, and move the Makefile it makes to a reasonable name, check that in, and add the symlink. Then there's a .gitignore so you don't accidentlly check in your own Makefile (unless its name is changed per above).

