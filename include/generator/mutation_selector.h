#ifndef MUTATION_SELECTOR_H
#define MUTATION_SELECTOR_H

#include <mutate.h>
#include <functional>
#include <cmath>
#include <limits>

/*
    this file was largerly written by Claude
*/

struct Mutation_arm {
    std::string name;
    float blockwise_ratio;
    unsigned int n_applications = 0;
    unsigned int n_new_placements = 0;

    // last snapshot for hill-climbing the ratio
    float prev_success_rate = 0.0f;
    float ratio_delta = 0.05f;  // step size for ratio perturbation

    float success_rate() const {
        return (n_applications == 0) ? 0.0f
            : (float)n_new_placements / (float)n_applications;
    }

    // UCB1 score — returns inf for untried arms so they get tried first
    float ucb_score(unsigned int total_trials) const {
        if (n_applications == 0)
            return std::numeric_limits<float>::max();
        return success_rate()
            + std::sqrt(2.0f * std::log((float)total_trials) / (float)n_applications);
    }

    void record(unsigned int new_placements) {
        n_applications++;

        n_new_placements += new_placements;

        // hill-climb blockwise_ratio based on whether success improved
        float current_sr = success_rate();
        if (current_sr >= prev_success_rate) {
            // improvement: continue in same direction
            blockwise_ratio = std::clamp(blockwise_ratio + ratio_delta, 0.1f, 1.0f);
        } else {
            // regression: reverse direction and take a smaller step
            ratio_delta *= -0.5f;
            blockwise_ratio = std::clamp(blockwise_ratio + ratio_delta, 0.1f, 1.0f);
        }

        prev_success_rate = current_sr;
    }
};

// Factory type: given an Ast_entry, Grammar, and blockwise_ratio, returns a Mutation_rule
using Mutation_factory = std::function<std::unique_ptr<Mutation_rule>(Ast_entry&, std::shared_ptr<Grammar>, float)>;

struct Mutation_selector {
    std::vector<Mutation_arm> arms;
    std::vector<Mutation_factory> factories;
    unsigned int total_trials = 0;

    void add(const std::string& name, float initial_ratio, Mutation_factory factory) {
        arms.push_back({name, initial_ratio});
        factories.push_back(std::move(factory));
    }

    // Returns index of arm selected by UCB1
    size_t select() const {
        size_t best = 0;
        float best_score = -1.0f;
        for (size_t i = 0; i < arms.size(); i++) {
            float s = arms[i].ucb_score(total_trials + 1);
            if (s > best_score) {
                best_score = s;
                best = i;
            }
        }
        return best;
    }

    // Apply selected mutation, return the arm index so archive can report back
    size_t apply(Ast_entry& genome, std::shared_ptr<Grammar> grammar) {
        size_t idx = select();
        total_trials++;
        factories[idx](genome, grammar, arms[idx].blockwise_ratio)->apply();
        return idx;
    }

    void record(size_t arm_idx, unsigned int new_placements) {
        arms[arm_idx].record(new_placements);
    }

    void print_stats(std::ostream& out) const {
        std::cout << std::endl;
        out << "=== Mutation pass stats ===" << std::endl;
        for (const auto& arm : arms) {
            out << arm.name
                << CYAN("  applications = ") << arm.n_applications
                << CYAN("  new placements = ") << arm.n_new_placements
                << CYAN("  success rate = ") << arm.success_rate()
                << CYAN("  ratio = ") << arm.blockwise_ratio
                << std::endl;
        }
        out << CYAN("==========================") << std::endl;
        std::cout << std::endl;
    }
};

#endif
