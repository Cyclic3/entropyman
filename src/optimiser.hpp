#pragma once

#include "gamestate.hpp"

//#include <map>
//#include <ranges>
#include <cmath>
#include <map>
#include <memory>
#include <numeric>
#include <set>
#include <span>
#include <variant>
#include <list>

namespace entropyman {
//  class Optimiser {
//  public:
//    struct Node {
//      std::map<std::string, std::vector<std::string_view>, std::less<>> layouts;
//      size_t loss;

//      Node remove_char(char guess, std::string_view empty_str) const {
//        Node ret;
//        ret.loss = loss;
//        for (auto& [old_layout, old_wordlist] : layouts) {
//          for (std::string_view word : old_wordlist) {
//            auto positions = GameState::hangman_iter(word, guess);
//            if (std::all_of(positions.begin(), positions.end(), [](auto i){ return !i; }))
//              ++ret.loss;
//            auto new_layout = old_layout;
//            apply_positions(guess, positions, new_layout);

//            ret.layouts[new_layout].emplace_back(word);
//          }
//        }

//        if (auto iter = ret.layouts.find(empty_str); iter != ret.layouts.end())
//          ret.loss += iter->second.size();
//        else
//          ret.loss = loss;

//        return ret;
//      }
//    };
//    struct EvalRes {
//      char best;
//      double entropy;
//      size_t loss;
//    };

//  private:
//    std::vector<std::map<std::string, Node, std::less<>>> nodes;
//    std::string empty_str;
//    std::string current_chars;
//    std::string current_knowledge;

//  private:
//    void populate(std::string remaining_chars, Node const& node) {
//      for (size_t i = 0; i < remaining_chars.size(); ++i) {
//        std::string new_remaining;
//        new_remaining.reserve(remaining_chars.size() - 1);
//        std::copy(remaining_chars.begin(), remaining_chars.begin() + i, std::back_inserter(new_remaining));
//        std::copy(remaining_chars.begin() + i + 1, remaining_chars.end(), std::back_inserter(new_remaining));
//        auto [target_node, is_new] = lookup_or_create(new_remaining);
//        if (!is_new)
//          continue;

//        char guess = remaining_chars[i];
//        // If we get here, we must fill in this node ourselves
//        *target_node = node.remove_char(guess, empty_str);
//      }
//    }
//  public:
//    /// Returns a tuple of the node and a boolean indicating if a new node was created for it
//    std::pair<Node*, bool> lookup_or_create(std::string_view remaining_chars) {
//      auto block = nodes.at(remaining_chars.size());
//      auto iter = block.lower_bound(remaining_chars);
//      if (iter != block.end() && iter->first == remaining_chars)
//        return {&iter->second, false};
//      return {&block.emplace_hint(iter)->second, true};
//    }

//    void prune(char discarded_char, positions_t const& positions) {
//      current_chars.insert(std::lower_bound(current_chars.begin(), current_chars.end(), discarded_char), discarded_char);
//      for (size_t i = 0; i < )
//        current_knowledge[i]


//      // First, we prune the previous block
//      {
//        auto& block = nodes.at(current_chars.size() - 1);
//        auto path = block.extract(current_chars);
//        block.clear();
//        block.insert(std::move(path));
//      }
//      // Next, we prune future nodes that are now excluded
//      for (auto i = current_chars.size(); i < nodes.size(); ++i) {
//        auto& block = nodes[i];
//        for (auto iter = block.begin(); iter != block.end();) {
//          if (std::binary_search(iter->first.begin(), iter->first.end(), discarded_char)) {
//            auto old_iter = iter;
//            ++iter;
//            block.erase(old_iter);
//            continue;
//          }

//          ++iter;
//        }
//      }
//    }

//  public:
//    Optimiser(GameState const& state, std::span<std::string_view> words);
//  };

  class Optimiser {
  private:
    struct NodeSpec;
    class Node;
    template<typename T>
    static constexpr bool node_comparable_v = std::is_same_v<std::decay_t<T>, Optimiser::Node> || std::is_same_v<std::decay_t<T>, Optimiser::NodeSpec>;

    template<typename A, typename B>
    friend std::enable_if_t<node_comparable_v<A> && node_comparable_v<B>, std::strong_ordering> operator<=>(A const& a, B const& b);

    using cache_t = std::map<NodeSpec, std::weak_ptr<Node>, std::less<>>;
    static void clean_cache(cache_t& cache) {
      for (auto iter = cache.begin(); iter != cache.end();) {
        if (iter->second.expired())
          cache.erase(iter++);
        else
          ++iter;
      }
    }
  private:
    struct NodeSpec {
      size_t remaining_lives;
      std::string knowledge;
      std::string remaining_chars;
      template<typename T>
      std::strong_ordering operator<=>(T const& b) const noexcept {
        auto& a = *this;
        if (auto _1 = a.remaining_lives <=> b.remaining_lives; !std::is_eq(_1))
          return _1;
        else if (auto _2 = a.remaining_chars <=> b.remaining_chars; !std::is_eq(_2))
          return _2;
        else
          return a.knowledge <=> b.knowledge;
      };
    };

    struct NodeSpecRef {
      size_t remaining_lives;
      std::string_view knowledge;
      std::string_view remaining_chars;
      template<typename T>
      std::strong_ordering operator<=>(T const& b) const noexcept {
        auto& a = *this;
        if (auto _1 = a.remaining_lives <=> b.remaining_lives; !std::is_eq(_1))
          return _1;
        else if (auto _2 = a.remaining_chars <=> b.remaining_chars; !std::is_eq(_2))
          return _2;
        else
          return a.knowledge <=> b.knowledge;
      }
    };
    class Node {
    private:
      using leaf_t = std::vector<std::string_view>;
      struct parent_t {
        size_t n_words;
        std::map<char, std::map<positions_t, std::shared_ptr<Node>, std::less<>>> children;
      };

    private:
      std::string knowledge;
      std::string remaining_chars;
      size_t remaining_lives;
      std::variant<leaf_t, parent_t> data;
      // TODO: allow threading
      mutable struct {
        uint64_t last_generation = 0;
        char best_option = placeholder;
        double value;
      } cached_entropy;

    private:
      double get_remaining_entropy_inner(uint64_t generation) const noexcept {
        auto count = get_count();
        // Win good
        if (is_won())
          return 0;
        // Death bad
        if (remaining_lives == 0)
          return (1ull<<32);

        return std::visit([generation, this](auto& x) {
          if constexpr (std::is_same_v<std::decay_t<decltype(x)>, leaf_t>) {
            return std::log2(x.size());
          }
          else {
            double best = std::numeric_limits<double>::infinity();
            for (auto& [guess, i] : x.children) {
              double new_best = 0;
              for (auto& [_, result] : i)
                new_best += result->get_remaining_entropy(generation) * result->get_count();
              if (new_best < best) {
                best = new_best;
                cached_entropy.best_option = guess;
              }
            }
            return best / x.n_words;
          }
        }, data);
      }

      double get_remaining_entropy(uint64_t generation) const noexcept {
        if (cached_entropy.last_generation >= generation)
          return cached_entropy.value;
        else {
          cached_entropy.last_generation = generation;
          return cached_entropy.value = get_remaining_entropy_inner(generation);
        }
      }
    public: //private:
//      std::pair<char, double> optimise_final(cache_t& cache, uint64_t generation) {
//        expand(cache);
//        char current_best = placeholder;
//        double min_entropy = std::numeric_limits<double>::infinity();
//        for (auto& [guess,results] : std::get<parent_t>(data).children) {
//          // Uncomment if you are boring (sorry)
//          size_t n_words = 0;
//          for (auto i : results)
//            n_words += i.second->get_count();
//          if (n_words == 0)
//            continue;
//          double curr_entropy = 0.;
//          for (auto& result : results)
//            curr_entropy += result.second->get_remaining_entropy(generation) * result.second->get_count();
//          curr_entropy /= get_count();
//          auto x = results.size();
//          if (curr_entropy < min_entropy) {
//            current_best = guess;
//            min_entropy = curr_entropy;
//          }
//        }
//        if (current_best == placeholder)
//          throw std::runtime_error{"All optimisation paths pessimal"};
//        return {current_best, min_entropy};
//      }
    public:
      std::pair<char, double> get_best() const {
        if (!std::holds_alternative<parent_t>(data))
          throw std::logic_error{"Best before expansion!"};
        double entropy = get_remaining_entropy();
        return {cached_entropy.best_option, entropy};
      }
      constexpr size_t get_count() const noexcept {
        return std::visit([](auto x) -> size_t {
          if constexpr (std::is_same_v<std::decay_t<decltype(x)>, leaf_t>)
            return x.size();
          else
            return x.n_words;
        }, data);
      }
      constexpr auto get_lives() const noexcept { return remaining_lives; }
      constexpr bool is_won() const noexcept { return knowledge.find(placeholder) == knowledge.npos; }
      constexpr std::optional<double> get_cached_entropy() {
        return cached_entropy.last_generation == 0 ? std::nullopt : std::optional<double>{cached_entropy.value};
      }

      constexpr uint64_t get_next_generation() const noexcept {
        return cached_entropy.last_generation + 1;
      }
      double get_remaining_entropy() const noexcept {
        return get_remaining_entropy(cached_entropy.last_generation + 1);
      }

      bool is_better_than(Node const& other, uint64_t generation) const noexcept {
        auto _1 = get_remaining_entropy(generation) <=> other.get_remaining_entropy(generation);
        if (std::is_lt(_1))
          return true;
        else if (std::is_gt(_1))
          return false;
        else
          return remaining_lives > other.remaining_lives;
      }

      constexpr bool is_expanded() const noexcept { return std::holds_alternative<parent_t>(data); }
      decltype(parent_t::children)& expand(cache_t& cache) {
        if (remaining_lives == 0)
          throw std::logic_error{"Tried to expand dead node"};
        if (is_won())
          throw std::logic_error{"Tried to expand won node"};

        leaf_t words;
        {
          auto x = std::get_if<leaf_t>(&data);
          if (x == nullptr)
            return std::get<parent_t>(data).children;
          words = std::move(*x);
        }
        parent_t& parent = data.emplace<parent_t>();
        parent.n_words = words.size();

        for (size_t guess_idx = 0; guess_idx != remaining_chars.size(); ++guess_idx) {
          auto guess = remaining_chars[guess_idx];
          std::list remaining_words(words.begin(), words.end());

          std::string new_remaining_chars = remaining_chars;
          new_remaining_chars.erase(guess_idx, 1);

          while (remaining_words.size() != 0) {
            auto word = remaining_words.front();
            remaining_words.pop_front();

            auto pos = GameState::hangman_iter(word, guess);
            std::string new_knowledge = knowledge;
            apply_positions(guess, pos, new_knowledge);
            size_t new_lives = (new_knowledge == knowledge) ? remaining_lives - 1 : remaining_lives;

            if (auto iter = cache.find(NodeSpecRef{new_lives, new_knowledge, new_remaining_chars}); iter != cache.end()) {
              if (auto ptr = iter->second.lock()) {
                parent.children[guess][pos] = std::move(ptr);
                continue;
              }
            }

            auto [iter, is_new] = parent.children[guess].emplace(pos, std::make_shared<Node>(std::move(new_knowledge), new_remaining_chars, new_lives, leaf_t{}));
            if (!is_new) {
              throw std::logic_error{"Somehow encountered word twice???"};
            }

            auto& target = parent.children[guess][pos];
            auto& target_words = std::get<leaf_t>(target->data);
            target_words.emplace_back(word);
            for (auto iter = remaining_words.begin(); iter != remaining_words.end();) {
              if (!GameState::hangman_compare(*iter, guess, pos)) {
                ++iter;
                continue;
              }
              target_words.emplace_back(*iter);
              remaining_words.erase(iter++);
            }
            cache.emplace(NodeSpec{target->remaining_lives, target->knowledge, target->remaining_chars}, target);
          }
        }
        return parent.children;
      }
      std::shared_ptr<Node> get(char guess, positions_t const& pos, cache_t& cache) {
        if (!std::holds_alternative<parent_t>(data))
          expand(cache);
        auto res = std::get<parent_t>(data).children.at(guess).at(pos);
        return std::get<parent_t>(data).children.at(guess).at(pos);
      }

    public:
      template<typename A, typename B, typename C>
      inline Node(A&& knowledge_, B&& remaining_chars_, size_t remaining_lives_, C&& words) :
        knowledge{std::forward<A>(knowledge_)}, remaining_chars{std::forward<B>(remaining_chars_)}, remaining_lives{remaining_lives_},
        data{std::forward<C>(words)} {}
    };

//  private:

  private:
    static constexpr auto opt_cmp = [](std::shared_ptr<Node> const& i, std::shared_ptr<Node> const& j)  {return i->get_remaining_entropy() < j->get_remaining_entropy();};
    using opt_queue_t = std::set<std::shared_ptr<Node>, decltype(opt_cmp)>;
  public:
    cache_t cache;
    std::shared_ptr<Node> root;

  private:
//    template<typename T>
//    void optimise_inner(T&& expanded, size_t depth = 5, opt_queue_t& opt) {
//      std::set<std::pair<double, std::shared_ptr<Node>>, decltype(cmp)> ;
//      if (depth == 0 || cache.size() > 200000)
//        return;
//      for (auto& [guess, responses] : expanded)
//        for (auto& [pos, child] : responses)
//          if (child->get_lives() > 0)
//            child->expand(cache);
//      for (auto& [guess, responses] : expanded)
//        for (auto& [pos, child] : responses)
//          if (child->get_lives() > 0)
//            optimise_inner(child->expand(cache), depth - 1);
//    }

  public:
    constexpr size_t get_cache_size() const noexcept { return cache.size(); }

    void update(char guess, positions_t const& pos) {
      root = root->get(guess, pos, cache);
      clean_cache(cache);
    }

    std::pair<char, double> optimise() {
      uint64_t max_steps = 100000;

      opt_queue_t opt_queue{root};

      for (uint64_t step = 0; step < max_steps && !opt_queue.empty(); ++step) {
        auto iter = opt_queue.begin();
        auto best = *iter;
        opt_queue.erase(iter);

        if (best->is_expanded() || best->is_won()) {
          --step;
          continue;
        }
        for (auto& [guess, responses] : best->expand(cache))
          for (auto& [pos, child] : responses)
            if (child->get_lives() > 0 && child->get_count() > 0 && !child->is_expanded() && !child->is_won())
              opt_queue.emplace(child);
      }

//      optimise_inner(root->expand(cache));

      return root->get_best();
    }

  public:
    Optimiser(GameState const& state, std::vector<std::string_view> words) {
      root = std::make_shared<Node>(state.partial_word, state.remaining_letters, state.remaining_lives, std::move(words));
    }
  };

  template<typename A, typename B>
  constexpr std::enable_if_t<Optimiser::node_comparable_v<A> && Optimiser::node_comparable_v<B>, std::strong_ordering> operator<=>(A const& a, B const& b) {
    auto _1 = a.remaining_lives <=> b.remaining_lives;
    if (!std::is_eq(_1))
      return _1;
    else
      return a.knowledge <=> b.knowledge;
  }


}
