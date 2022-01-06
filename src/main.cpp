//#include <cmath>
//#include <charconv>
//#include <fstream>
//#include <iostream>
//#include <map>
//#include <memory>
//#include <span>
//#include <string>
//#include <string_view>
//#include <variant>
//#include <vector>

#include "gamestate.hpp"
#include "optimiser.hpp"

#include <iostream>
#include <fstream>
#include <charconv>

int main(int argc, char** argv) {
  if (argc != 3) {
    std::cout << argv[0] << " <wordlist> <length>" << std::endl;
    return 1;
  }

  std::ifstream word_list_f{argv[1]};
  if (!word_list_f)
    throw std::runtime_error{"Unable to open wordlist"};

  size_t word_len;
  {
    auto res = std::from_chars(argv[2], nullptr, word_len, 10);
    if (static_cast<int>(res.ec) != 0)
      throw std::runtime_error{"Invalid word length"};
  }


  std::string line;

  std::vector<std::string> word_list;

  {
    while (std::getline(word_list_f, line))
      word_list.emplace_back(std::move(line));
  }

  /// XXX: references `word_list`
  std::vector<std::string_view> filtered_words;
  for (auto&& i : std::move(word_list))
    if (i.size() == word_len)
      filtered_words.emplace_back(std::move(i));

  auto state = entropyman::GameState(word_len);

  auto optimiser = entropyman::Optimiser(state, filtered_words);

  while (true) {
    std::cout << "Lives: " << state.remaining_lives << "\n"
              << "State: " << state.partial_word << "\n"
              << std::flush;

    auto [choice, entropy] = optimiser.optimise();
    std::cout << "Choice: " << choice << "\n"
              << "Words: " << optimiser.root->get_count() << "\n"
              << "Entropy: " << entropy << "\n"
              << "Cache: " << optimiser.get_cache_size() << "\n"
              << std::flush;

    std::cout << "\nResult: " << std::flush;
    std::getline(std::cin, line);
//        line = "teddy";

    entropyman::positions_t positions(word_len, false);
    for (size_t i = 0; i < line.size(); ++i)
      positions[i] = (line[i] == choice);

    state = state.step(choice, positions);
    optimiser.update(choice, positions);

    if (state.has_won()) {
      std::cout << "WIN!!!" << std::endl;
      break;
    }
    if (state.has_lost()) {
      std::cout << "LOSE!!!" << std::endl;
      break;
    }
  }
}
