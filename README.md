# ncursesFM
Ncurses File Manager for linux

It has following features for now:
* basic file/dir functions support: copy/paste/cut, new/remove, rename. Copy/cut: to remove a file previously selected for copy just press c/x again on it.
* 2 tabs support.
* iso mount support -> you must have fuseiso installed.
* open files with $editor (settings defined) var.
* show hidden files.
* stats support (permissions and sizes).
* in-program helper message -> press 'l'.
* sync between tabs.
* rename file/folders support.
* move file/folders support -> use cut for this. If a file is "cut" on the same filesystem, it will be moved, otherwise it will be copied.
* Paste is executed in a different thread. If you try to quit while paste thread is still running, you'll be asked if ncursesFM must wait for the thread to finish its work.
* Search support: it will search your string in current directory tree, and, if anything was found, you'll be able to open it, if it was a file, or to move in your searched location.

It reads following variables from ncursesFM.conf (using libconfig)...remember to set them!
* editor -> editor used to open files
* show_hidden -> whether to show hidden files by default or not.
* iso_mount_point -> mount point of any iso (for example /home/$your_username). Mount point will be "$iso_mount_point/iso_name".

PKGBUILD for Archlinux is here! Soon on AUR.

It requires ncurses, libconfig and (optionally) fuseiso.

![Alt text](ncursesfm.png?raw=true)