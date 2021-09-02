# SYNOPSIS

unixexec [*OPTION*] [*PATH*] *COMMAND* *...*

# DESCRIPTION

unixexec: a minimal, [UCSPI](https://jdebp.uk/FGA/UCSPI.html) inetd for
Unix sockets

`unixexec` attaches the stdin/stdout of a command to a Unix socket:

* immediately exec(3)'s the command: the data is not proxied via another
  process

* does not multiplex access to a socket

# EXAMPLES

## echo server

```
$ unixexec /tmp/sock cat

$ nc -U /tmp/sock
```

## Supervised using daemontools

An echo server allowing 3 concurrent connections:

    service/
    ├── echo1
    │   └── run
    ├── echo2
    │   └── run
    └── echo3
        └── run

* service/echo1/run

``` sh
#!/bin/sh

exec unixsock /tmp/echo1.socket cat
```

* service/echo2/run

``` sh
#!/bin/sh

exec unixsock /tmp/echo2.socket cat
```

* service/echo3/run

``` sh
#!/bin/sh

exec unixsock /tmp/echo3.socket cat
```

Then run:

    svscan service

# Build

    make

    #### static executable using musl
    ./musl-make

# OPTIONS

-U, --no-unlink
: do not unlink the socket before binding

-v, --verbose
: write additional messages to stderr

-h, --help
: usage summary

# ENVIRONMENT VARIABLES

PROTO
: protocol, always set to UNIX

UNIXLOCALPATH
: socket path

UNIXLOCALUID
: user ID of unixexec process

UNIXLOCALUSER
: user name of unixexec process

UNIXLOCALGID
: group ID of unixexec process

UNIXLOCALPID
: process ID of unixexec process

UNIXREMOTEPID
: process ID of connecting process (support: Linux, OpenBSD)

UNIXREMOTEEUID
: user ID of connecting process

UNIXREMOTEUSER
: user name of connecting process

UNIXREMOTEEGID
: group ID of connecting process
