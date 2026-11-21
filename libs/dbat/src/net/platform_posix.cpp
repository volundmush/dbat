/*
 * The networking's system calls on POSIX systems: see dbat/net/platform.hpp. platform_win32.cpp is the Windows one.
 */
#ifndef _WIN32
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/resource.h>
#include <sys/socket.h>
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
  char host[INET_ADDRSTRLEN];
  if (!inet_ntop(AF_INET, &peer.sin_addr, host, sizeof(host)))
    return accepted{std::move(s), "unknown"};
  return accepted{std::move(s), host};
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

void report_error(const char *what) { perror(what); }

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

} // namespace net
#endif
