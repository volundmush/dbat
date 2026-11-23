#pragma once
#include <array>
#include <string>
#include <string_view>

constexpr int MAX_ACCOUNT_SLOTS = 5; /* character slots per account */

/* A user account: what its file (lib/user) holds, a line each, in this order. */
struct account {
  std::string user;                                 /* the account's name                     */
  std::string email;                                /* with each @ stored as <AT>             */
  std::string pass;                                 /* the password                           */
  int total{};                                      /* the character slots it may use          */
  int rpp{};                                        /* its RPP                                 */
  std::array<std::string, MAX_ACCOUNT_SLOTS> slots; /* each slot's character, or "Empty"      */
  int level{};                                      /* above 0 gets past the lockout list     */
  int customfile{};                                 /* whether its customs file has been made */
  int rbank{};                                      /* the RPP bank (unused)                   */
};

/* Calls f on each of a's fields, in the file's order. */
template <class Account, class F>
void each_account_field(Account &a, F &&f)
{
  f(a.user);
  f(a.email);
  f(a.pass);
  f(a.total);
  f(a.rpp);
  for (auto &slot : a.slots)
    f(slot);
  f(a.level);
  f(a.customfile);
  f(a.rbank);
}

/* Reads the account file at path over a, field by field; false if it can't be opened */
bool read_account_file(const std::string &path, account &a);
/* Writes a to name's account file; false, and logged, if it couldn't */
bool write_account(std::string_view name, const account &a);
/* True if name is on the lockout list (the "lockout" intro file) */
bool lockRead(std::string_view name);
