## TODO

### Rewriting

#### Cmake

- [x] Port to cmake
- [x] Gettext support
- [x] Install support
- [x] systemd is a mandatory dep
- [x] Libconfig mandatory
- [ ] port to libpopt instead of getopt (?)
- [x] Switch to sr.ht CI

- [ ] Release

#### Libmodule

- [ ] Port to libmodule
- [ ] Release

#### Ui Changes/improvements

- [ ] Port to ncurses panel/menu.h
- [ ] let user switch modality from within each other (eg: from device_mode to bookmarks)
- [ ] rename bookmarks to "Places" and add mounted fs
- [ ] new mode: job's mode -> to check and review all queued jobs
- [ ] add a config to start with 2 tabs and to select for each tab the modality -> by default 2 tabs -> first tab files and second tab Places
- [ ] add a manpage and drop helper message (simplify changing modality too) (?)

### Trash support
- [ ] add trash support using Trashd daemon (optional, if trashd is not present, trash support won't be loaded), with new key (d?)
