## TODO

**GENERIC**:

- [ ] symlink() support? (easy)
- [ ] sshfs/nfs support? (mid)
- [ ] new mode: job's mode -> to check and review all queued jobs (mid)
- [x] update changelog
- [ ] update doc
- [x] libnotify support (easy)
- [x] drop libx11 requirement
- [ ] update wiki pages

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

