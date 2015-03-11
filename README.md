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
* move file support (use cut for this. If file is "cut" on the same filesystem, it will be moved.)
* create dir, remove *empty* dir support

As soon as possible there will be a PKGBUILD for archlinux.

It requires ncurses, libconfig and (optionally) fuseiso.

![Alt text](ncursesfm.png?raw=true)