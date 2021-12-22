#pragma once

#include "common.hpp"

#include <array>
#include <string>

namespace entropyman {
  class GameState {
  private:
    using fast_guesses_t = std::array<char, 8>;
  //  struct Node {
  //    std::shared_ptr<GameState> state;
  //    std::map<char, std::variant<double, Node>> choices = {};
  //    std::optional<double> cached_entropy = std::nullopt;
  //  };

  public:
//    std::vector<std::string_view> remaining_words;
    std::string partial_word;
    size_t remaining_lives;
    std::string remaining_letters;

  public:
    static positions_t hangman_iter(std::string_view word, char guess);
    static bool hangman_compare(std::string_view word, char guess, positions_t const& positions);
    static bool hangman_compare(std::string_view word, std::string_view knowledge);

  public:
    size_t optimise_brute_force_inner(fast_guesses_t guesses, size_t pos);
    char optimise();
    GameState step(char choice, positions_t const& positions) const;

    bool has_won() const noexcept { return partial_word.find(placeholder) == partial_word.npos; }
    bool has_lost() const noexcept { return remaining_lives <= 0; }

  public:
    inline GameState(size_t word_len, size_t initial_lives = standard_initial_lives,
              decltype(remaining_letters) letters = std::string{standard_letters}) noexcept :
      partial_word(word_len, placeholder), remaining_lives{initial_lives}, remaining_letters{std::move(letters)} {}
//    GameState(GameState const&) = default;
//    GameState(GameState&&) = default;
  private:
    GameState() = default;
  };

//  char GameState::optimise() {
//    // When we have only one choice, we have 0 entropy to gain.
//    //
//    // As such, we must treat this case differently
//    if (remaining_words.size() == 1) {
//      auto word = remaining_words.front();
//      for (auto letter : remaining_letters)
//        if (word.find(letter) != word.npos)
//          return letter;
//      abort();
//    }

//    // If there are less than ~1e6 positions left, then a brute force is feasible
//  //  double max_remaining = 1;
//  //  {
//  //    const size_t spaces = std::count(partial_word.begin(), partial_word.end(), placeholder);
//  //    for (size_t i = 0; i < spaces; ++i) {
//  //      max_remaining *= (remaining_letters.size() - i);
//  //    }
//  //  }


//    double current_best_weight = -1;
//    char current_best_choice = placeholder;
//    for (char choice : remaining_letters) {
//      std::map<positions_t, size_t> counters;
//      for (auto& word: remaining_words)
//        ++counters[hangman_iter(word, choice)];

//      double this_weight = 0;
//      for (auto i : counters) {
//        auto p = i.second/static_cast<double>(remaining_words.size());
//        this_weight -= p * std::log2(p);
//      }

//      if (this_weight > current_best_weight) {
//        current_best_weight = this_weight;
//        current_best_choice = choice;
//      }
//    }

//    if (current_best_choice == placeholder)
//      abort();
//    return current_best_choice;
//  }
}
