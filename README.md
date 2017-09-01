rig
===

It's a thing, that I'm working on.

utilities
---------

- **always** - Runs another command as a child process, re-execing
  it if/when it exits.
- **supervise** - Runs all the executable scripts in a single
  directory, if they aren't already running.
- **init** - Waits for inherited child processes; starts processes
  from a flat file (/etc/inittab).
- **logto** - Timestamps log streams and writes them to disk.
  Handles log file rotation based on file size.
- **runas** - Exec another program as a different UID + GID.
- **locked** - Exec another program once a lock is held.
- **every** - Exec another program on a given periodic schedule.

contributing
------------

Before you submit a pull request, open an issue on Github so we
can talk about it.  I've got plans for this thing, and I haven't
written them all down yet.
