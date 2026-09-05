//+----------------------------------------------------------------------------+
//| Description:  Magic Set Editor - Program to make card games                |
//| Copyright:    (C) Twan van Laarhoven and the other MSE developers          |
//| License:      GNU General Public License 2 or later (see file COPYING)     |
//+----------------------------------------------------------------------------+

#pragma once

// ----------------------------------------------------------------------------- : Includes

#include <random>
#include <array>

// ----------------------------------------------------------------------------- : UID

// Initialize RNG
inline std::mt19937_64& get_generator() {
  static std::mt19937_64 gen = [] {
    std::random_device rd;
    std::seed_seq seed{rd(), rd(), rd(), rd()};
    return std::mt19937_64(seed);
  }();
  return gen;
}

// Generate a string consisting of 32 uniformly random digits.
static String generate_uid() {
  std::mt19937_64& gen = get_generator();
  static std::uniform_int_distribution<int> dis(0, 9);

  std::array<char, 32> buf;
  for (auto& c : buf) {
    c = '0' + dis(gen);
  }
  return String(buf.data(), buf.size());
}
