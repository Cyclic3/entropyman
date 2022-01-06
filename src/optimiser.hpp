#pragma once

#include "gamestate.hpp"

#include <chrono>
#include <cmath>
#include <list>
#include <map>
#include <memory>
#include <numeric>
#include <optional>
#include <queue>
#include <set>
#include <span>
#include <variant>

namespace entropyman {
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
        bool stale = true;
        char best_option = placeholder;
        double value;
      } cached_entropy;
      std::weak_ptr<Node> parent;

    private:
      double get_remaining_entropy_inner() const noexcept {
        // Win good
        if (is_won())
          return 0;
        // Death bad
        if (remaining_lives == 0)
          return (1ull<<32);

        return std::visit([this](auto& x) {
          if constexpr (std::is_same_v<std::decay_t<decltype(x)>, leaf_t>) {
            return std::log2(x.size());
          }
          else {
            double best = std::numeric_limits<double>::infinity();
            for (auto& [guess, i] : x.children) {
              double new_best = 0;
              for (auto& [_, result] : i)
                new_best += result->get_remaining_entropy() * result->get_count();
              if (new_best < best) {
                best = new_best;
                cached_entropy.best_option = guess;
              }
            }
            return best / x.n_words;
          }
        }, data);
      }

      void get_remaining_words_inner(std::vector<std::string_view>& vec) const {
        std::visit([&vec](auto& x) {
          if constexpr (std::is_same_v<std::decay_t<decltype(x)>, leaf_t>)
            vec.insert(vec.end(), x.begin(), x.end());
          else {
            // Minimise expansion
            size_t min_width = std::numeric_limits<size_t>::max();
            auto best_element = x.children.end();
            for (auto iter = x.children.begin(); iter != x.children.end(); ++iter) {
              if (iter->second.size() >= min_width)
                continue;
              best_element = iter;
              min_width = best_element->second.size();
            }
            if (best_element == x.children.end())
              throw std::runtime_error{"Tried to enumerate words from empty parent"};
            for (auto& i : best_element->second)
              i.second->get_remaining_words_inner(vec);
          }
        }, data);
      }
      void mark_stale() noexcept {
        cached_entropy.stale = true;
        if (auto ptr = parent.lock())
          ptr->mark_stale();
      }

    public:
      std::vector<std::string_view> get_remaining_words() const {
        std::vector<std::string_view> ret;
        get_remaining_words_inner(ret);
        return ret;
      }
      std::pair<char, double> get_best() const {
        if (!std::holds_alternative<parent_t>(data))
          throw std::logic_error{"Best before expansion!"};
        double entropy = get_remaining_entropy();
        return {cached_entropy.best_option, entropy};
      }
      constexpr size_t get_count() const noexcept {
        return std::visit([](auto& x) -> size_t {
          if constexpr (std::is_same_v<std::decay_t<decltype(x)>, leaf_t>)
            return x.size();
          else
            return x.n_words;
        }, data);
      }
      constexpr auto get_lives() const noexcept { return remaining_lives; }
      constexpr bool is_won() const noexcept { return knowledge.find(placeholder) == knowledge.npos; }
      double get_remaining_entropy() const noexcept {
        if (!cached_entropy.stale)
          return cached_entropy.value;

        cached_entropy.stale = false;
        return cached_entropy.value = get_remaining_entropy_inner();
      }

      bool is_better_than(Node const& other) const noexcept {
        auto _1 = get_remaining_entropy() <=> other.get_remaining_entropy();
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

        // Now we are changing the type of node, we have to mark it stale
        mark_stale();

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
    static constexpr auto opt_cmp = [](std::shared_ptr<Node> const& i, std::shared_ptr<Node> const& j) {
      if (i == j)
        return false;
      // <= so that
      return i->get_remaining_entropy() <= j->get_remaining_entropy();
    };
    using opt_queue_t = std::set<std::shared_ptr<Node>, decltype(opt_cmp)>;
  public:
    cache_t cache;
    std::shared_ptr<Node> root;

  public:
    constexpr size_t get_cache_size() const noexcept { return cache.size(); }

    void update(char guess, positions_t const& pos) {
      root = root->get(guess, pos, cache);
      clean_cache(cache);
    }

    std::pair<char, double> optimise() {
      if (root->get_remaining_entropy() == 0) {
        root->expand(cache);
        return root->get_best();
      }

      opt_queue_t opt_queue{root};

      auto start = std::chrono::steady_clock::now();
      std::chrono::steady_clock::time_point stop;
      static constexpr std::chrono::seconds think_time{5};

      for (uint64_t step = 0; !opt_queue.empty() && (step % 1000 != 0 || std::chrono::steady_clock::now() - start < think_time); ++step) {
        auto front = opt_queue.extract(opt_queue.begin());
        auto& best = front.value();

        if (best->is_won())
          continue;
        for (auto& [guess, responses] : best->expand(cache))
          for (auto& [pos, child] : responses)
            if (child->get_lives() > 0 && child->get_count() > 0 /*&& !child->is_expanded()*/ && !child->is_won())
              opt_queue.emplace(child);
      }
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
