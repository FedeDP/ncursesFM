## TODO

**GENERIC**:  

- [ ] symlink() support?
- [x] switch to XDG_CONFIG_HOME for user config (fallback to .config)
- [x] use a mutex when adding new job to job's queue (and when deleting last job from queue, to avoid sync issues: eg while one thread is adding a job, other thread puts job's queue ptr to null)
- [ ] new mode: job's mode -> to check and review all queued jobs
- [x] fix info print about current job not properly updating
- [ ] update doc
