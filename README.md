# ncursesFM
Ncurses File Manager for linux

It has following features for now:
* basic file functions support: copy/paste/cut, new/remove. Paste is executed in a different thread.
* 2 tabs support.
* iso mount support (you must have fuseiso installed)
* open files with $editor (settings defined) var
* show hidden files
* stats support (permissions and sizes)
* in-program helper message (press 'l')
* sync between tabs
* rename file/folders support

As soon as possible there will be a PKGBUILD for archlinux.

It needs ncurses, libconfig and (optional) fuseiso.

![Alt text](ncursesfm.png?raw=true)