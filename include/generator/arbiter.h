#ifndef ARBITER_H
#define ARBITER_H

#include <mutate.h>
#include <functional>
#include <cmath>
#include <limits>

using Mutation_factory = std::function<std::unique_ptr<Mutation_rule>(Ast_entry&, std::shared_ptr<Grammar>, float)>;

struct Arm {

    public:
        Arm(std::string _mutation_name, Mutation_factory _factory);

        inline std::string get_name() const { return mutation_name; }

        inline float get_blockwise_ratio() const { return blockwise_ratio; }

        inline unsigned int get_total_mutation_trials() const { return total_mutation_trials; }

        inline unsigned int get_n_discovered_cells() const { return n_discovered_cells; }

        float success_rate() const;

        // UCB1 score — returns inf for untried arms so they get tried first
        float ucb_score(unsigned int total_trials) const;

        // each application results in discovery of a new cell, hill climb based on that
        void hill_climb(bool cell_discovered);

        void apply(Ast_entry& genome, std::shared_ptr<Grammar> grammar);

    private:
        std::string mutation_name;
        Mutation_factory factory;

        float blockwise_ratio = 0.2f;
        unsigned int total_mutation_trials = 0;
        unsigned int n_discovered_cells = 0;

        // last snapshot for hill-climbing the ratio
        float prev_success_rate = 0.0f;
        float ratio_delta = 0.05f;  // step size for ratio perturbation
};


struct Arbiter {

    public:
        Arbiter(){}

        void add(const std::string& name, Mutation_factory factory);

        size_t select() const;

        size_t apply(Ast_entry& genome, std::shared_ptr<Grammar> grammar);

        void record(size_t arm_idx, bool cell_discovered);

        void print_stats(std::ostream& out) const;

    private:
        std::vector<Arm> arms;
        unsigned int total_trials = 0;
};

#endif
