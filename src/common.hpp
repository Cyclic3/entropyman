#pragma once

#include <string_view>
#include <vector>

namespace entropyman {
  constexpr size_t standard_initial_lives = 9;
  constexpr std::string_view standard_letters = "abcdefghijklmnopqrstuvwxyz";
  constexpr char placeholder = '_';

  using positions_t = std::vector<bool>;
  template<typename T>
  inline void apply_positions(char c, positions_t const& pos, T& target) {
    for (size_t i = 0; i < pos.size(); ++i)
      if (pos[i])
        target.at(i) = c;
  }
}
