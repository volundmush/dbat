/*
 * The networking's system calls on Windows (Winsock): see dbat/net/platform.hpp. platform_posix.cpp is the POSIX one.
 * The messages are those of the POSIX file, so the logs read the same.
 */
#ifdef _WIN32
#include <cerrno>
#include <charconv>
#include <csignal>
#include <cstdio>
#include <cstdlib>

#include <winsock2.h>
#include <ws2tcpip.h>
#include <io.h>

#include "dbat/net/platform.hpp"
#include "dbat/consts/maximums.hpp"
#include "dbat/log.hpp"

static_assert(sizeof(net::poll_set::entry) == sizeof(WSAPOLLFD) &&
              offsetof(net::poll_set::entry, events) == offsetof(WSAPOLLFD, events) &&
              offsetof(net::poll_set::entry, revents) == offsetof(WSAPOLLFD, revents));

namespace {

bool set_sendbuf(SOCKET s)
{
  int opt = MAX_SOCK_BUF;

  if (setsockopt(s, SOL_SOCKET, SO_SNDBUF, reinterpret_cast<const char *>(&opt), sizeof(opt)) == SOCKET_ERROR) {
    net::report_error("SYSERR: setsockopt SNDBUF");
    return false;
  }
  return true;
}

void nonblock(SOCKET s)
{
  u_long on = 1;
  if (ioctlsocket(s, FIONBIO, &on) == SOCKET_ERROR) {
    net::report_error("SYSERR: Fatal error executing nonblock (comm.c)");
    exit(1);
  }
}

in_addr bind_address(std::string_view ip)
{
  in_addr addr{};

  if (ip.empty()) {
    addr.s_addr = htonl(INADDR_ANY);
  } else if (inet_pton(AF_INET, std::string(ip).c_str(), &addr) != 1) {
    mud_log("SYSERR: DFLT_IP of {} appears to be an invalid IP address", ip);
    addr.s_addr = htonl(INADDR_ANY);
  }

  if (addr.s_addr == htonl(INADDR_ANY)) {
    mud_log("Binding to all IP interfaces on this host.");
  } else {
    char text[INET_ADDRSTRLEN] = "";
    inet_ntop(AF_INET, &addr, text, sizeof(text));
    mud_log("Binding only to IP address {}", text);
  }
  return addr;
}

net::process::signal_flags *flags;
void (*on_crash)(int sig);

/* Windows puts a signal back to its default once it is delivered, so the handler takes it again */
void hupsig(int sig)
{
  flags->shutdown = sig;
  std::signal(sig, hupsig);
}

void fatal_signal(int sig)
{
  on_crash(sig);
  std::signal(sig, SIG_DFL);
  std::raise(sig);
}

} // namespace

namespace net {

void socket::close()
{
  if (handle_ != no_handle)
    closesocket(static_cast<SOCKET>(std::exchange(handle_, no_handle)));
}

void startup()
{
  WSADATA data;
  if (int err = WSAStartup(MAKEWORD(2, 2), &data)) {
    std::fprintf(stderr, "SYSERR: WSAStartup: error %d\n", err);
    exit(1);
  }
}

socket listen_on(std::string_view ip, std::uint16_t port)
{
  socket s(::socket(PF_INET, SOCK_STREAM, 0));
  if (!s) {
    report_error("SYSERR: Error creating socket");
    exit(1);
  }
  SOCKET h = static_cast<SOCKET>(s.handle());

  /* SO_REUSEADDR lets another process take a port in use on Windows; SO_EXCLUSIVEADDRUSE is what POSIX's means */
  BOOL opt = TRUE;
  if (setsockopt(h, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, reinterpret_cast<const char *>(&opt), sizeof(opt)) == SOCKET_ERROR) {
    report_error("SYSERR: setsockopt REUSEADDR");
    exit(1);
  }

  set_sendbuf(h);

  linger ld{};
  if (setsockopt(h, SOL_SOCKET, SO_LINGER, reinterpret_cast<const char *>(&ld), sizeof(ld)) == SOCKET_ERROR)
    report_error("SYSERR: setsockopt SO_LINGER"); /* Not fatal I suppose. */

  sockaddr_in sa{};
  sa.sin_family = AF_INET;
  sa.sin_port   = htons(port);
  sa.sin_addr   = bind_address(ip);

  if (bind(h, reinterpret_cast<sockaddr *>(&sa), sizeof(sa)) == SOCKET_ERROR) {
    report_error("SYSERR: bind");
    s.close();
    exit(1);
  }
  nonblock(h);
  if (listen(h, SOMAXCONN) == SOCKET_ERROR) {
    report_error("SYSERR: listen");
    s.close();
    exit(1);
  }
  return s;
}

std::optional<accepted> accept_from(const socket &listener)
{
  sockaddr_in peer{};
  int size = sizeof(peer);

  SOCKET h = accept(static_cast<SOCKET>(listener.handle()), reinterpret_cast<sockaddr *>(&peer), &size);
  if (h == INVALID_SOCKET) {
    report_error("SYSERR: accept");
    return std::nullopt;
  }
  socket s(static_cast<native_handle>(h));
  nonblock(h);

  if (!set_sendbuf(h))
    return std::nullopt;

  BOOL keepalive = TRUE;
  if (setsockopt(h, SOL_SOCKET, SO_KEEPALIVE, reinterpret_cast<const char *>(&keepalive), sizeof(keepalive)) == SOCKET_ERROR)
    report_error("SYSERR: setsockopt SO_KEEPALIVE");

  char host[INET_ADDRSTRLEN];
  if (!inet_ntop(AF_INET, &peer.sin_addr, host, sizeof(host)))
    return accepted{std::move(s), "unknown"};
  return accepted{std::move(s), host};
}

io_result send_some(native_handle h, std::string_view text)
{
  int result = send(static_cast<SOCKET>(h), text.data(), static_cast<int>(text.size()), 0);

  if (result > 0)
    return {io_status::done, static_cast<std::size_t>(result)};
  if (result == 0) {
    mud_log("SYSERR: Huh??  write() returned 0???  Please report this!");
    return {io_status::failed};
  }
  if (WSAGetLastError() == WSAEWOULDBLOCK)
    return {io_status::would_block};
  return {io_status::failed};
}

io_result receive_some(native_handle h, std::span<char> into)
{
  int result = recv(static_cast<SOCKET>(h), into.data(), static_cast<int>(into.size()), 0);

  if (result > 0)
    return {io_status::done, static_cast<std::size_t>(result)};
  if (result == 0) {
    mud_log("WARNING: EOF on socket read (connection broken by peer)");
    return {io_status::closed};
  }
  int err = WSAGetLastError();
  if (err == WSAEWOULDBLOCK || err == WSAEINTR)
    return {io_status::would_block};
  if (err == WSAECONNRESET)
    return {io_status::failed};
  report_error("SYSERR: perform_socket_read: about to lose connection");
  return {io_status::failed};
}

void report_error(const char *what)
{
  int err   = WSAGetLastError();
  char *msg = nullptr;
  FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr,
                 static_cast<DWORD>(err), 0, reinterpret_cast<LPSTR>(&msg), 0, nullptr);
  std::fprintf(stderr, "%s: %s\n", what, msg ? msg : "unknown error");
  LocalFree(msg);
}

void poll_set::clear() { entries_.clear(); }

void poll_set::add(native_handle h, bool want_write)
{
  entries_.push_back({h, static_cast<short>(want_write ? POLLRDNORM | POLLWRNORM : POLLRDNORM), 0});
}

poll_set::outcome poll_set::wait(int timeout_ms)
{
  if (WSAPoll(reinterpret_cast<WSAPOLLFD *>(entries_.data()), static_cast<ULONG>(entries_.size()), timeout_ms) != SOCKET_ERROR)
    return outcome::ready;
  return WSAGetLastError() == WSAEINTR ? outcome::interrupted : outcome::failed;
}

readiness poll_set::at(std::size_t i) const
{
  short r = entries_[i].revents;
  return {.readable = (r & POLLRDNORM) != 0, .writable = (r & POLLWRNORM) != 0, .hung_up = (r & POLLHUP) != 0,
          .failed = (r & (POLLERR | POLLNVAL)) != 0};
}

std::size_t poll_set::size() const { return entries_.size(); }

/* Windows has no limit on sockets to raise: WSAPoll takes any number */
std::optional<long long> raise_descriptor_limit() { return std::nullopt; }

namespace process {

/* Ctrl-C and Ctrl-Break come as SIGINT; there is no SIGHUP, SIGUSR1 or SIGUSR2 */
void install_signals(signal_flags &to, void (*crash)(int sig))
{
  flags    = &to;
  on_crash = crash;
  std::signal(SIGINT, hupsig);
  std::signal(SIGTERM, hupsig);
  for (int sig : {SIGSEGV, SIGFPE, SIGILL, SIGABRT})
    std::signal(sig, fatal_signal);
}

/* There is no timer of the process's own running time to set */
void start_deadlock_timer(int, void (*)()) {}

void stop_deadlock_timer() {}

void write_raw(int fd, std::string_view text) { _write(fd, text.data(), static_cast<unsigned>(text.size())); }

/* The frames' addresses, which tools/symbolize.sh can't resolve, but a debugger can */
void write_backtrace(int fd)
{
  void *frames[62];
  USHORT n = CaptureStackBackTrace(0, 62, frames, nullptr);

  for (USHORT i = 0; i < n; i++) {
    char line[24] = "0x";
    char *end     = std::to_chars(line + 2, line + sizeof(line) - 1, reinterpret_cast<std::uintptr_t>(frames[i]), 16).ptr;
    *end++        = '\n';
    write_raw(fd, std::string_view(line, end));
  }
}

std::expected<std::string, std::string> executable_path() { return std::unexpected("not available on this platform"); }

bool is_socket(native_handle h)
{
  int type    = 0;
  int size    = sizeof(type);

  return getsockopt(static_cast<SOCKET>(h), SOL_SOCKET, SO_TYPE, reinterpret_cast<char *>(&type), &size) == 0;
}

/* A copyover can't happen here, so there is nothing brought across */
std::vector<native_handle> open_sockets() { return {}; }

void replace_process(const std::string &, const std::vector<std::string> &) { errno = ENOSYS; }

} // namespace process

} // namespace net
#endif
