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
* create dir, remove dir support

It reads following variables from ncursesFM.conf (using libconfig)...remember to set them!
* editor -> editor used to open files
* show_hidden -> whether to show hidden files by default or not.
* iso_mount_point -> mount point of any iso (for example /home/$your_username). Mount point will be "$iso_mount_point/iso_name".

PKGBUILD for Archlinux is here! I'll upload this to AUR as soon as I finish some basic functions (i.e. folder functions).

It requires ncurses, libconfig and (optionally) fuseiso.

![Alt text](ncursesfm.png?raw=true)