## TODO

**GENERIC**:

- [ ] symlink() support? (easy)
- [ ] sshfs/nfs support? (mid)
- [ ] new mode: job's mode -> to check and review all queued jobs (mid)
- [ ] update doc
- [ ] add a "helper mode" instead of helper_win (way simpler and probably better)
- [ ] add trash support (https://specifications.freedesktop.org/trash-spec/trashspec-latest.html) (move file to trash, on click see trashed files. From there you can remove them all or only one of them. You can restore them too.)
- [ ] by default, "r" will move to trash instead of delete. User can customize this behaviour through a conf file setting. (move_to_trash = 0;)
- [ ] rename bookmarks to "Places" and add mounted fs + trash there.

### ssh support ideas:
Config file with {
	ip address;
	mountpoint (defaults to "/run/media/$username/");
	name (defaults to "remote$counter");
	port (defaults to 22);
	eventually pem file (?);
}

We will show that folder as eg:
"ssh:remote0"
in device_mode.
When user clicks it, it will be mounted in its mountpoint.

