/*
 * The networking's system calls on POSIX systems: see dbat/net/platform.hpp. platform_win32.cpp is the Windows one.
 */
#ifndef _WIN32
#include <array>
#include <cerrno>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <system_error>

#include <arpa/inet.h>
#include <execinfo.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <unistd.h>

#include "dbat/net/platform.hpp"
#include "dbat/consts/maximums.hpp"
#include "dbat/log.hpp"

static_assert(sizeof(net::poll_set::entry) == sizeof(pollfd) && offsetof(net::poll_set::entry, events) == offsetof(pollfd, events) &&
              offsetof(net::poll_set::entry, revents) == offsetof(pollfd, revents));

namespace {

/* Gives the socket the game's send buffer size */
bool set_sendbuf(int s)
{
  int opt = MAX_SOCK_BUF;

  if (setsockopt(s, SOL_SOCKET, SO_SNDBUF, &opt, sizeof(opt)) < 0) {
    perror("SYSERR: setsockopt SNDBUF");
    return false;
  }
  return true;
}

void nonblock(int s)
{
  int flags = fcntl(s, F_GETFL, 0);
  flags |= O_NONBLOCK;
  if (fcntl(s, F_SETFL, flags) < 0) {
    perror("SYSERR: Fatal error executing nonblock (comm.c)");
    exit(1);
  }
}

/* The address to listen on: ip, or every interface if it is empty or no address at all (which is logged). */
in_addr bind_address(std::string_view ip)
{
  in_addr addr{};

  if (ip.empty()) {
    addr.s_addr = htonl(INADDR_ANY);
  } else if (!inet_aton(std::string(ip).c_str(), &addr)) {
    mud_log("SYSERR: DFLT_IP of {} appears to be an invalid IP address", ip);
    addr.s_addr = htonl(INADDR_ANY);
  }

  if (addr.s_addr == htonl(INADDR_ANY))
    mud_log("Binding to all IP interfaces on this host.");
  else
    mud_log("Binding only to IP address {}", inet_ntoa(addr));
  return addr;
}

/* Where the signal handlers send what they're told: set once, by install_signals() and start_deadlock_timer() */
net::process::signal_flags *flags;
void (*on_crash)(int sig);
void (*on_expiry)();

void reread_wizlists(int) { flags->reread_wizlist = true; }

void unrestrict_game(int) { flags->emergency_unban = true; }

/* Only sets a flag: logging or exiting from inside a signal handler is not safe */
void hupsig(int sig) { flags->shutdown = sig; }

/* clean up our zombie kids to avoid defunct processes */
void reap(int)
{
  while (waitpid(-1, nullptr, WNOHANG) > 0)
    ;
  signal(SIGCHLD, reap);
}

void deadlock(int) { on_expiry(); }

/* Says what it can, then dies of sig, dumping core, as it would have */
void fatal_signal(int sig)
{
  on_crash(sig);
  signal(sig, SIG_DFL);
  raise(sig);
}

} // namespace

namespace net {

void socket::close()
{
  if (handle_ != no_handle)
    ::close(std::exchange(handle_, no_handle));
}

void startup() {}

socket listen_on(std::string_view ip, std::uint16_t port)
{
  socket s(::socket(PF_INET, SOCK_STREAM, 0));
  if (!s) {
    perror("SYSERR: Error creating socket");
    exit(1);
  }

  int opt = 1;
  if (setsockopt(s.handle(), SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
    perror("SYSERR: setsockopt REUSEADDR");
    exit(1);
  }

  set_sendbuf(s.handle());

  linger ld{.l_onoff = 0, .l_linger = 0};
  if (setsockopt(s.handle(), SOL_SOCKET, SO_LINGER, &ld, sizeof(ld)) < 0)
    perror("SYSERR: setsockopt SO_LINGER"); /* Not fatal I suppose. */

  sockaddr_in sa{};
  sa.sin_family = AF_INET;
  sa.sin_port   = htons(port);
  sa.sin_addr   = bind_address(ip);

  if (bind(s.handle(), reinterpret_cast<sockaddr *>(&sa), sizeof(sa)) < 0) {
    perror("SYSERR: bind");
    s.close();
    exit(1);
  }
  nonblock(s.handle());
  if (listen(s.handle(), SOMAXCONN) < 0) {
    perror("SYSERR: listen");
    s.close();
    exit(1);
  }
  return s;
}

std::optional<accepted> accept_from(const socket &listener)
{
  sockaddr_in peer{};
  socklen_t size = sizeof(peer);

  socket s(accept(listener.handle(), reinterpret_cast<sockaddr *>(&peer), &size));
  if (!s) {
    perror("SYSERR: accept");
    return std::nullopt;
  }
  /* keep it from blocking */
  nonblock(s.handle());

  if (!set_sendbuf(s.handle()))
    return std::nullopt;

  /* notice connections that silently went away (NAT timeouts, crashed clients) */
  int keepalive = 1;
  if (setsockopt(s.handle(), SOL_SOCKET, SO_KEEPALIVE, &keepalive, sizeof(keepalive)) < 0)
    perror("SYSERR: setsockopt SO_KEEPALIVE");

  /* The site is the numeric address. No reverse DNS: the lookup blocks the whole game, and the name it
     returns is chosen by whoever controls the address, so it is no basis for bans. */
  std::array<char, INET_ADDRSTRLEN> host;
  if (!inet_ntop(AF_INET, &peer.sin_addr, host.data(), host.size()))
    return accepted{std::move(s), "unknown"};
  return accepted{std::move(s), host.data()};
}

io_result send_some(native_handle h, std::string_view text)
{
  ssize_t result;

  do
    result = write(h, text.data(), text.size());
  while (result < 0 && errno == EINTR);

  if (result > 0)
    return {io_status::done, static_cast<std::size_t>(result)};
  if (result == 0) {
    /* This should never happen! */
    mud_log("SYSERR: Huh??  write() returned 0???  Please report this!");
    return {io_status::failed};
  }
  if (errno == EAGAIN) /* EWOULDBLOCK is the same value on Linux */
    return {io_status::would_block};
  return {io_status::failed};
}

io_result receive_some(native_handle h, std::span<char> into)
{
  ssize_t result = read(h, into.data(), into.size());

  if (result > 0)
    return {io_status::done, static_cast<std::size_t>(result)};
  if (result == 0) {
    mud_log("WARNING: EOF on socket read (connection broken by peer)");
    return {io_status::closed};
  }
  if (errno == EINTR || errno == EAGAIN) /* EWOULDBLOCK is the same value on Linux */
    return {io_status::would_block};
  if (errno == ECONNRESET)
    return {io_status::failed};
  /* We don't know what happened, cut them off. This qualifies for a SYSERR because we have no idea what happened at
     this point. */
  perror("SYSERR: perform_socket_read: about to lose connection");
  return {io_status::failed};
}

void report_error(std::string_view what) { perror(std::string(what).c_str()); }

void poll_set::clear() { entries_.clear(); }

void poll_set::add(native_handle h, bool want_write)
{
  entries_.push_back({h, static_cast<short>(want_write ? POLLIN | POLLOUT : POLLIN), 0});
}

poll_set::outcome poll_set::wait(int timeout_ms)
{
  if (poll(reinterpret_cast<pollfd *>(entries_.data()), entries_.size(), timeout_ms) >= 0)
    return outcome::ready;
  return errno == EINTR ? outcome::interrupted : outcome::failed;
}

readiness poll_set::at(std::size_t i) const
{
  short r = entries_[i].revents;
  return {.readable = (r & POLLIN) != 0, .writable = (r & POLLOUT) != 0, .hung_up = (r & POLLHUP) != 0,
          .failed = (r & (POLLERR | POLLNVAL)) != 0};
}

std::size_t poll_set::size() const { return entries_.size(); }

std::optional<long long> raise_descriptor_limit()
{
  rlimit limit;

  if (getrlimit(RLIMIT_NOFILE, &limit) < 0) {
    perror("SYSERR: calling getrlimit");
    exit(1);
  }
  /* set the current to the maximum */
  limit.rlim_cur = limit.rlim_max;
  if (setrlimit(RLIMIT_NOFILE, &limit) < 0) {
    perror("SYSERR: calling setrlimit");
    exit(1);
  }
  if (limit.rlim_max == RLIM_INFINITY)
    return std::nullopt;
  return static_cast<long long>(limit.rlim_max);
}

namespace process {

/*
 * signal() takes the same handlers that sigaction() would: on Linux it is sigaction() with SA_RESTART, so calls the
 * game makes are restarted, except poll(), which never is (the loop's coma relies on that).
 */
void install_signals(signal_flags &to, void (*crash)(int sig))
{
  flags    = &to;
  on_crash = crash;

  /* user signal 1: reread wizlists.  Used by autowiz system. */
  signal(SIGUSR1, reread_wizlists);
  /* user signal 2: unrestrict game.  Used for emergencies if you lock yourself out of the MUD somehow. */
  signal(SIGUSR2, unrestrict_game);

  signal(SIGHUP, hupsig);
  signal(SIGCHLD, reap);
  signal(SIGINT, hupsig);
  signal(SIGTERM, hupsig);
  signal(SIGPIPE, SIG_IGN);
  signal(SIGALRM, SIG_IGN);

  /* crashes: say so, then die (and dump core) as before */
  std::array<void *, 1> warm;
  backtrace(warm.data(), warm.size()); /* load the unwinder now, so the handler doesn't have to allocate */
  for (int sig : {SIGSEGV, SIGBUS, SIGFPE, SIGILL, SIGABRT})
    signal(sig, fatal_signal);

  /* allow core dumps up to the hard limit */
  rlimit core;
  if (getrlimit(RLIMIT_CORE, &core) == 0) {
    core.rlim_cur = core.rlim_max;
    setrlimit(RLIMIT_CORE, &core);
  }
}

void start_deadlock_timer(int seconds, void (*expiry)())
{
  itimerval every{.it_interval = {.tv_sec = seconds, .tv_usec = 0}, .it_value = {.tv_sec = seconds, .tv_usec = 0}};

  on_expiry = expiry;
  setitimer(ITIMER_VIRTUAL, &every, nullptr);
  signal(SIGVTALRM, deadlock);
}

void stop_deadlock_timer()
{
  itimerval none{};

  signal(SIGVTALRM, SIG_IGN);
  setitimer(ITIMER_VIRTUAL, &none, nullptr);
}

void write_raw(int fd, std::string_view text)
{
  if (::write(fd, text.data(), text.size()) < 0) { /* nothing else we can do */
  }
}

void write_backtrace(int fd)
{
  std::array<void *, 64> frames;
  int n = backtrace(frames.data(), frames.size());

  backtrace_symbols_fd(frames.data(), n, fd);
}

std::expected<std::string, std::string> executable_path()
{
#ifdef __linux__
  std::error_code error;
  std::string exe = std::filesystem::read_symlink("/proc/self/exe", error).string();

  if (error)
    return std::unexpected(error.message());
  /* what a binary rebuilt while running looks like: the new one is what we want */
  constexpr std::string_view deleted = " (deleted)";
  if (exe.size() > deleted.size() && exe.ends_with(deleted))
    exe.resize(exe.size() - deleted.size());
  if (access(exe.c_str(), X_OK) < 0)
    return std::unexpected(std::strerror(errno));
  return exe;
#else
  return std::unexpected("not available on this platform");
#endif
}

bool is_socket(native_handle h)
{
  struct stat info;

  return fstat(h, &info) == 0 && S_ISSOCK(info.st_mode);
}

std::vector<native_handle> open_sockets()
{
  std::vector<native_handle> found;
  rlimit limit;
  int highest = 1024;

  if (getrlimit(RLIMIT_NOFILE, &limit) == 0 && limit.rlim_cur != RLIM_INFINITY)
    highest = static_cast<int>(limit.rlim_cur);
  for (int fd = STDERR_FILENO + 1; fd < highest; fd++)
    if (is_socket(fd))
      found.push_back(fd);
  return found;
}

void replace_process(const std::string &exe, const std::vector<std::string> &args)
{
  std::vector<char *> argv;

  for (const std::string &arg : args)
    argv.push_back(const_cast<char *>(arg.c_str()));
  argv.push_back(nullptr);
  /* descriptors are inherited, but anything still buffered here would be lost */
  std::fflush(nullptr);
  execv(exe.c_str(), argv.data());
}

} // namespace process

} // namespace net
#endif
