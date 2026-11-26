/*
 * A client connecting and sending whatever bytes the fuzzer likes, from the first prompt on: each 0xff ends one write
 * to the socket, and each 0xfe lets ten pulses pass. Between writes the client reads what the game sent, as a real one
 * would. Whatever arrives, and however it is split, the game must stay consistent and keep going.
 */
#include <array>
#include <unistd.h>

#include "dbat/comm.hpp"
#include "dbat/descriptor_utils.hpp"
#include "dbat/game_world.hpp"
#include "dbat/iterate.hpp"
#include "dbat/world_audit.hpp"

#include "../harness.hpp"
#include "fuzz.hpp"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *, size_t);

constexpr int MAX_WRITES = 300; /* so that one input can't take the whole of a timeout */

static void audit_problem(void *, const char *problem)
{
  file_print(stderr, "AUDIT: {}\n", problem);
  abort();
}

static bool connected(GameWorld *g, struct descriptor_data *d)
{
  for (descriptor_data *other : each_desc(g))
    if (other == d)
      return true;
  return false;
}

/* What the game sent the client, read and thrown away */
static void read_output(GameWorld *g, struct descriptor_data *d, int peer)
{
  std::array<char, 65536> buffer;
  if (connected(g, d) && !desc_output(d).empty())
    game_send_output(g, d);
  while (read(peer, buffer.data(), sizeof(buffer)) > 0) {
  }
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
  fuzz_can_refuse = true;
  if (setjmp(fuzz_refused)) {
    th_abandon();
    fuzz_can_refuse = false;
    return 0;
  }
  GameWorld *g              = th_boot_at(1800000000);
  int peer                  = -1;
  struct descriptor_data *d = th_connect(g, &peer);

  size_t used = 0;
  for (int writes = 0; d && used < size && writes < MAX_WRITES && connected(g, d); writes++) {
    size_t start = used;
    while (used < size && data[used] != 0xff && data[used] != 0xfe)
      used++;
    bool pulse = used < size && data[used] == 0xfe;
    if (used > start && !th_send(g, d, peer, reinterpret_cast<const char *>(data + start), used - start))
      break; /* the game closed it */
    used++;
    if (connected(g, d))
      read_output(g, d, peer);
    if (pulse)
      th_pulses(g, 10);
    if (world_audit(g, audit_problem, nullptr))
      abort();
  }
  close(peer);
  th_pulses(g, 20); /* the game notices the hang-up, and carries on */
  if (world_audit(g, audit_problem, nullptr))
    abort();
  th_shutdown(g);
  fuzz_can_refuse = false;
  return 0;
}
