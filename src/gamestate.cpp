#include "gamestate.hpp"

#include <algorithm>
#include <iterator>

namespace entropyman {
  positions_t GameState::hangman_iter(std::string_view word, char guess) {
    positions_t ret(word.size(), false);
    for (size_t i = 0; i < word.size(); ++i)
      if (word[i] == guess)
        ret[i] = true;
    return ret;
  }

  bool GameState::hangman_compare(std::string_view word, char guess, positions_t const& positions) {
    if (word.size() != positions.size())
      return false;
    for (size_t i = 0; i < word.size(); ++i)
      if ((word[i] == guess) != positions[i])
        return false;
    return true;
  }

  bool GameState::hangman_compare(std::string_view word, std::string_view knowledge) {
    if (word.size() != knowledge.size())
      return false;
    for (size_t i = 0; i < word.size(); ++i)
      if (knowledge[i] != placeholder && word[i] != knowledge[i])
        return false;
    return true;
  }

  GameState GameState::step(char choice, positions_t const& positions) const {
    // noexcept, so this is fine
    GameState ret;

    ret.remaining_letters.reserve(remaining_letters.size());
    std::copy_if(remaining_letters.begin(), remaining_letters.end(), std::back_inserter(ret.remaining_letters), [choice](auto i) { return i != choice; } );

    ret.partial_word = partial_word;

    if (std::all_of(positions.begin(), positions.end(), [](auto i) { return i == false; }))
      ret.remaining_lives = remaining_lives - 1;
    else {
      ret.remaining_lives = remaining_lives;
      for (size_t i = 0; i < positions.size(); ++i)
        if (positions[i])
          // Using at so we don't get haxxored
          ret.partial_word.at(i) = choice;
    }

//    for (auto word : remaining_words)
//      if (hangman_compare(word, choice, positions))
//        ret.remaining_words.emplace_back(word);

    return ret;
  }
}
