#pragma once
#include <array>

/* What creating a character has chosen whose effects wait for the end, when apply_draft() (src/net/chargen.cpp)
   makes them: the rolls and gains. Choices that are only themselves (race, sex, looks, sensei, the android model,
   genomes and traits) are set on the character as they're made. */
struct chargen_draft {
  int size{-1};                 /* CON_HW: the size range picked */
  std::array<int, 9> answers{}; /* CON_Q1 to CON_Q9: the answer picked to each, from 0 */
  int age{-1};                  /* CON_QX: the age picked */
  int alignment{};              /* CON_ALIGN: 0 keeps what the answers made it, or which one, from 1 */
  bool forget{};                /* CON_SKILLS: the race and sensei skills given up for practice sessions */
};
