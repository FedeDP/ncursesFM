# ncursesFM
Ncurses File Manager for linux

It has following features:
* file/dir functions support: copy/paste/cut, new/remove, rename. Copy/cut: to remove a file previously selected for copy just press c/x again on it.
* 2 tabs support.
* iso mount support -> you must have fuseiso installed.
* open files with $editor (settings defined) var.
* show hidden files.
* stats support (permissions and sizes).
* in-program helper message -> press 'l'.
* sync between tabs.
* rename file/folders support.
* shasum, through openssl.
* move file/folders support -> use cut for this. If a file is "cut" on the same filesystem, it will be moved, otherwise it will be copied.
* Search support: it will search your string in current directory tree, and, if anything was found, you'll be able to open it, if it was a file, or to move in your searched location.
* It can search your string inside archives too. Then, if found, you can go to the folder of the archive, to extract it.
* Basic print support: you need "libcups" for this to work.
* Extract (compressed) archive through libarchive.
* Compress files/folders through libarchive -> to select files/folders, use c/x (same as copy/cut)
* View current file's mimetype through libmagic.
* Pasting, compressing, extracting, printing and searching are executed in other threads. You'll get a notification when the job is done.
* If you try to quit while a {pasting, compressing, extracting} thread is still running, you'll be asked if ncursesFM must wait for the thread to finish its work. (Printing thread and search thread are safer, no data corruption is possible)

It reads following variables from /etc/default/ncursesFM.conf (using libconfig)...remember to set them!
* editor -> editor used to open files
* show_hidden -> whether to show hidden files by default or not.
* starting_directory -> default starting directory.
* use_default_starting_dir_second_tab -> whether to use "starting_directory" when opening second tab. Otherwise current tab will be used.

IT DOES NOT SUPPORT TERMINAL RESIZE. It is meant to be used maximized, or from a tty.

Archlinux users can now install ncursesFM from aur: https://aur.archlinux.org/packages/ncursesfm-git/

Build requires ncurses, libconfig, libcups, libarchive, openssl and libmagic.

At runtime it requires ncurses, libconfig, libmagic, libarchive and (optionally) {fuseiso, libcups, openssl}.

In Archlinux libmagic is provided by "file" package, that is part of base-devel group. So it is not listed inside PKGBUILD.

![Alt text](ncursesfm.png?raw=true)