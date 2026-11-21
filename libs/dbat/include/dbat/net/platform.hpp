#pragma once
/*
 * The little the networking needs from the operating system: sockets, polling them, the process's limit on open
 * files, and the process itself (signals, the deadlock timer, a crash's backtrace, and what a copyover needs).
 * Everything else in src/net is standard C++. platform_posix.cpp and platform_win32.cpp are the only files that
 * know which system they run on; each compiles to nothing on the other.
 */
#include <csignal>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace net {

/* What the system calls a socket: a file descriptor on POSIX, a SOCKET on Windows. */
#ifdef _WIN32
using native_handle                      = std::uintptr_t;
inline constexpr native_handle no_handle = ~native_handle{0}; /* INVALID_SOCKET */
#else
using native_handle                      = int;
inline constexpr native_handle no_handle = -1;
#endif

/* Owns one socket, and closes it when destroyed. */
class socket {
public:
  socket() = default;
  explicit socket(native_handle h) : handle_(h) {} /* takes over h, a copyover's or a test's */
  socket(socket &&other) noexcept : handle_(std::exchange(other.handle_, no_handle)) {}
  socket &operator=(socket &&other) noexcept
  {
    if (this != &other) {
      close();
      handle_ = std::exchange(other.handle_, no_handle);
    }
    return *this;
  }
  socket(const socket &)            = delete;
  socket &operator=(const socket &) = delete;
  ~socket() { close(); }

  native_handle handle() const { return handle_; }
  explicit operator bool() const { return handle_ != no_handle; }
  /* Gives the handle up without closing it (a copyover keeps its sockets across the exec) */
  native_handle release() { return std::exchange(handle_, no_handle); }
  void close();

private:
  native_handle handle_ = no_handle;
};

/* Once, before any other call here (Winsock wants starting; POSIX needs nothing) */
void startup();

/* The game's listening socket on port, on the address ip or on all of them if ip is empty: logs which, and exits the
   process if it can't be had. */
socket listen_on(std::string_view ip, std::uint16_t port);

/* A connection taken from a listening socket: nonblocking, with keepalive and the game's send buffer size, and the
   numeric address it came from ("unknown" if that can't be read). */
struct accepted {
  socket sock;
  std::string host;
};
/* The next connection waiting on listener, or nothing if accepting failed (which it logs) */
std::optional<accepted> accept_from(const socket &listener);

/* What one try at sending or receiving came to */
enum class io_status {
  done,        /* bytes were moved */
  would_block, /* nothing to read, or no room to write, just now */
  closed,      /* the other end closed the connection (receiving) */
  failed,      /* the connection is broken */
};
struct io_result {
  io_status status;
  std::size_t bytes = 0; /* when done */
};
/* One try at sending some of text: done with how much went, or why none did. A send of nothing at all is logged as
   a SYSERR and counts as failed. */
io_result send_some(native_handle h, std::string_view text);
/* One try at reading into into: done with how much came, or why none did. A closed connection is logged; an error
   other than a reset connection or an interruption (which counts as would_block) is reported as a SYSERR. */
io_result receive_some(native_handle h, std::span<char> into);

/* Prints what what was doing when the last call here failed, and why, as perror() does */
void report_error(const char *what);

/* How a socket stands after a poll */
struct readiness {
  bool readable{}; /* something to read (or a connection to accept) */
  bool writable{}; /* room to write */
  bool hung_up{};  /* the other end has gone */
  bool failed{};   /* the socket is in an error state, or isn't one */
};

/* The sockets to poll, in the order added */
class poll_set {
public:
  enum class outcome { ready, interrupted, failed };

  void clear();
  void add(native_handle h, bool want_write);
  /* Waits up to timeout_ms (-1: until something happens, 0: not at all). interrupted: a signal came first. */
  outcome wait(int timeout_ms);
  readiness at(std::size_t i) const;
  std::size_t size() const;

  /* Laid out as the system's own (pollfd, WSAPOLLFD), which the platform file checks */
  struct entry {
    native_handle fd;
    short events;
    short revents;
  };

private:
  std::vector<entry> entries_;
};

/* Raises the process's limit on open files as far as it can go, as the most connections the game could hold need:
   the limit, or nothing if there is none. Exits the process if the limit can't be read or changed. */
std::optional<long long> raise_descriptor_limit();

namespace process {

/* What the signals have asked of the game loop, which looks at (and clears) them between passes */
struct signal_flags {
  volatile std::sig_atomic_t reread_wizlist{};  /* SIGUSR1: the autowiz system has rewritten the wizlists */
  volatile std::sig_atomic_t emergency_unban{}; /* SIGUSR2: someone locked themselves out */
  volatile std::sig_atomic_t shutdown{};        /* SIGHUP, SIGINT or SIGTERM: its number */
};

/* Sets the handlers up: the requests above go to flags, and a crash (SIGSEGV, SIGBUS, SIGFPE, SIGILL, SIGABRT) calls
   on_crash(sig) inside the handler, then dies of the signal (dumping core up to the hard limit) as it would have.
   Children are reaped, and SIGPIPE and SIGALRM ignored. Windows has only SIGINT, SIGTERM and the crashes. */
void install_signals(signal_flags &flags, void (*on_crash)(int sig));
/* Calls on_expiry() from a signal handler every seconds of the process's own running time (POSIX ITIMER_VIRTUAL);
   nothing on Windows */
void start_deadlock_timer(int seconds, void (*on_expiry)());
/* Stops it, and ignores it should it run out anyway: the timer outlives an exec, and would kill the new process if it
   ran out during boot */
void stop_deadlock_timer();

/* Writes text to the file descriptor fd, safely from inside a signal handler */
void write_raw(int fd, std::string_view text);
/* Writes the call stack to the file descriptor fd, one frame a line, safely from inside a signal handler (once
   install_signals() has run) */
void write_backtrace(int fd);

/* For a copyover. The program this process runs, as whatever file is now at the path it started from (a rebuilt
   binary is picked up), if it can be run: or why not. Linux only; elsewhere, that it isn't available. */
std::expected<std::string, std::string> executable_path();
/* Whether h is a socket */
bool is_socket(native_handle h);
/* The sockets this process has open, after the standard streams, in order (none on Windows) */
std::vector<native_handle> open_sockets();
/* Runs exe with args in this process's place, keeping its sockets, having flushed every output stream. Returns only
   if that failed, with errno saying why. */
void replace_process(const std::string &exe, const std::vector<std::string> &args);

} // namespace process

} // namespace net
