## TODO

### Rewriting
- [ ] Every modality will be "loaded" and will set a char (eg: 'm') and a function (eg: 'change_modality(device_mode)') in a hasmap
- [ ] when pressing a char, hashmap.get(char) will return correct function to be called
- [ ] let user switch modality from within each other (eg: from device_mode to bookmarks)
- [ ] let 'w' close current tab no matter if it is the first
- [ ] rename bookmarks to "Places" and add mounted fs + trash there.
- [ ] new mode: job's mode -> to check and review all queued jobs
- [ ] add a config to start with 2 tabs and to select for each tab the modality

### Trash support
- [ ] add trash support using Trashd daemon (optional, if trashd is not present, trash support won't be loaded), with new key (d?)

### Ideas/Longterm
- [ ] Port to C++?
- [ ] port to cmake?
- [ ] switch to libyui UI lib (https://github.com/libyui/libyui)?
- [ ] switch to dbus-c++ instead of sd-bus from systemd (and drop systemd dep)?
