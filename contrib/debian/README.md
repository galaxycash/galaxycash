
Debian
====================
This directory contains files used to package galaxycashd/galaxycash-qt
for Debian-based Linux systems. If you compile galaxycashd/galaxycash-qt yourself, there are some useful files here.

## galaxycash: URI support ##


galaxycash-qt.desktop  (Gnome / Open Desktop)
To install:

	sudo desktop-file-install galaxycash-qt.desktop
	sudo update-desktop-database

If you build yourself, you will either need to modify the paths in
the .desktop file or copy or symlink your galaxycash-qt binary to `/usr/bin`
and the `../../share/pixmaps/galaxycash128.png` to `/usr/share/pixmaps`

galaxycash-qt.protocol (KDE)

