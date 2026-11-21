#pragma once
/*
 * The little the networking needs from the operating system: sockets, polling them, and the process's limit on open
 * files. Everything else in src/net is standard C++. platform_posix.cpp and platform_win32.cpp are the only files that
 * know which system they run on; each compiles to nothing on the other.
 */
#include <cstddef>
#include <cstdint>
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

} // namespace net
