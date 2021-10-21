#define _GNU_SOURCE 1
#include <arpa/inet.h>
#include <err.h>
#include <errno.h>
#include <getopt.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/un.h>

#include <sys/stat.h>
#include <sys/types.h>

#include <pwd.h>

#define UNIXEXEC_VERSION "0.1.0"

#ifndef UNIX_PATH_MAX
#define UNIX_PATH_MAX (sizeof(((struct sockaddr_un *)0)->sun_path))
#endif

typedef struct {
  int verbose;
  int unlink;
} unixexec_state_t;

static const struct option long_options[] = {
    {"no-unlink", no_argument, NULL, 'U'},
    {"verbose", no_argument, NULL, 'v'},
    {"help", no_argument, NULL, 'h'},
    {NULL, 0, NULL, 0},
};

static int unixexec_listen(const unixexec_state_t *up, const char *path,
                           size_t pathlen);
static int unixexec_unlink(const unixexec_state_t *up, const char *path);
static int setlocalenv(const char *path);
static int setremoteenv(int fd);
static int setenvuint(const char *key, u_int64_t val);
static noreturn void usage(void);

int main(int argc, char *argv[]) {
  unixexec_state_t up = {0};
  int lfd;
  int fd;
  struct sockaddr_storage sa = {0};
  socklen_t salen = sizeof(sa);
  const char *path;
  int ch;

  up.unlink = 1;

  while ((ch = getopt_long(argc, argv, "+hUv", long_options, NULL)) != -1) {
    switch (ch) {
    case 'U':
      up.unlink = 0;
      break;
    case 'v':
      up.verbose++;
      break;
    case 'h':
    default:
      usage();
    }
  }

  argc -= optind;
  argv += optind;

  if (argc < 2) {
    usage();
  }

  path = argv[0];

  lfd = unixexec_listen(&up, path, strlen(path));
  if (lfd == -1)
    err(111, "listen: %s", path);

  fd = accept(lfd, (struct sockaddr *)&sa, &salen);
  if (fd == -1)
    err(111, "accept");

  if (setlocalenv(path) == -1)
    err(111, "setlocalenv");

  if (setremoteenv(fd) == -1)
    err(111, "setremoteenv");

  if ((dup2(fd, STDOUT_FILENO) == -1) || (dup2(fd, STDIN_FILENO) == -1))
    err(111, "dup2");

  (void)execvp(argv[1], argv + 1);

  err(127, "execvp");
}

static int unixexec_listen(const unixexec_state_t *up, const char *path,
                           size_t pathlen) {
  struct sockaddr_un sa = {0};
  size_t salen;
  int fd;

  if (pathlen >= UNIX_PATH_MAX) {
    errno = ENAMETOOLONG;
    return -1;
  }

  fd = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);

  if (fd == -1)
    return -1;

  sa.sun_family = AF_UNIX;

  (void)memcpy(sa.sun_path, path, pathlen);
  salen = SUN_LEN(&sa);

  if (unixexec_unlink(up, path) == -1)
    return -1;

  if (bind(fd, (struct sockaddr *)&sa, salen) == -1)
    return -1;

  if (listen(fd, 1) == -1) {
    int errnum = errno;
    (void)close(fd);
    errno = errnum;
    return -1;
  }

  return fd;
}

static int unixexec_unlink(const unixexec_state_t *up, const char *path) {
  struct stat sb = {0};

  if (!up->unlink)
    return 0;

  if (stat(path, &sb) == -1)
    return (errno == ENOENT) ? 0 : -1;

  if ((sb.st_mode & S_IFMT) != S_IFSOCK) {
    errno = ENOTSOCK;
    return -1;
  }

  return unlink(path);
}

static int setlocalenv(const char *path) {
  struct passwd *pw;

  if (path == NULL)
    return -1;

  if (setenv("PROTO", "UNIX", 1) == -1)
    return -1;

  if (setenv("UNIXLOCALPATH", path, 1) == -1)
    return -1;

  if (setenvuint("UNIXLOCALUID", getuid()) == -1)
    return -1;

  pw = getpwuid(getuid());
  if (pw != NULL) {
    if (setenv("UNIXLOCALUSER", pw->pw_name, 1) == -1)
      return -1;
  }

  if (setenvuint("UNIXLOCALGID", getgid()) == -1)
    return -1;

  if (setenvuint("UNIXLOCALPID", (unsigned)getpid()) == -1)
    return -1;

  return 0;
}

static int setremoteenv(int fd) {
  struct passwd *pw;

#if defined(__linux__)
  struct ucred peer;
#elif defined(__OpenBSD__)
  struct sockpeercred peer;
#elif defined(__FreeBSD__)
  struct {
    uid_t uid;
    gid_t gid;
  } peer;
#endif

#if defined(__linux__) || defined(__OpenBSD__) || defined(__FreeBSD__)
#if defined(__linux__) || defined(__OpenBSD__)
  socklen_t peerlen = sizeof(peer);

  if (getsockopt(fd, SOL_SOCKET, SO_PEERCRED, &peer, &peerlen) == -1)
    return -1;

  if (setenvuint("UNIXREMOTEPID", (unsigned)peer.pid) == -1)
    return -1;
#elif defined(__FreeBSD__)
  if (getpeereid(fd, &peer.uid, &peer.gid) == -1)
    return -1;
#endif

  if (setenvuint("UNIXREMOTEEUID", peer.uid) == -1)
    return -1;

  pw = getpwuid(peer.uid);
  if (pw != NULL) {
    if (setenv("UNIXREMOTEUSER", pw->pw_name, 1) == -1)
      return -1;
  }

  if (setenvuint("UNIXREMOTEEGID", peer.gid) == -1)
    return -1;
#endif

  return 0;
}

static int setenvuint(const char *key, u_int64_t val) {
  char buf[21];
  int rv;

  rv = snprintf(buf, sizeof(buf), "%lu", val);
  if (rv < 0 || (unsigned)rv > sizeof(buf))
    return -1;

  return setenv(key, buf, 1);
}

static noreturn void usage(void) {
  errx(EXIT_FAILURE,
       "[OPTION] <SOCKETPATH> <COMMAND> <...>\n"
       "version: %s\n"
       "-U, --no-unlink           do not unlink the socket before binding\n"
       "-v, --verbose             write additional messages to stderr\n"
       "-h, --help                usage summary",
       UNIXEXEC_VERSION);
}
