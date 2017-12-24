## TODO

### Rewriting
- [ ] let user switch modality from within each other (eg: from device_mode to bookmarks) [MID]
- [ ] rename bookmarks to "Places" and add mounted fs + trash there. [HIGH PRIORITY]
- [ ] new mode: job's mode -> to check and review all queued jobs [LOW]
- [ ] add a config to start with 2 tabs and to select for each tab the modality -> by default 2 tabs -> first tab files and second tab Places [HIGH]
- [ ] use c++ + cmake
- [ ] use dbus-c++ instead of sd-bus to drop systemd dep

### Trash support
- [ ] add trash support using Trashd daemon (optional, if trashd is not present, trash support won't be loaded), with new key (d?) [HIGH]

### Ideas/Longterm
- [ ] switch to libyui UI lib (https://github.com/libyui/libyui)?
