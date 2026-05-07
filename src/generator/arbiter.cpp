#include <arbiter.h>

Arm::Arm(std::string _mutation_name, Mutation_factory _factory):
    mutation_name(_mutation_name),
    factory(_factory)
{}

float Arm::success_rate() const {
    return (total_mutation_trials == 0) ? 0.0f
        : (float)n_discovered_cells / (float)total_mutation_trials;
}

// UCB1 score — returns inf for untried arms so they get tried first
float Arm::ucb_score(unsigned int total_trials) const {
    if (total_mutation_trials == 0){
        return std::numeric_limits<float>::max();
    } else {
        return success_rate()
        + std::sqrt((2.0f * std::log((float)total_mutation_trials)) / (float)total_trials);
    }
}

// each application results in discovery of a new cell, hill climb based on that
void Arm::hill_climb(bool cell_discovered) {
    n_discovered_cells += cell_discovered;
    total_mutation_trials += 1;

    float current_sr = success_rate();
    
    if (current_sr <= prev_success_rate) {
        ratio_delta *= -0.5f;
    }

    blockwise_ratio = std::clamp(blockwise_ratio + ratio_delta, 0.1f, 1.0f);

    prev_success_rate = current_sr;
}

void Arm::apply(Ast_entry& genome, std::shared_ptr<Grammar> grammar){
    factory(genome, grammar, blockwise_ratio)->apply();
}


void Arbiter::add(const std::string& name, Mutation_factory factory) {
    arms.push_back(Arm(name, std::move(factory)));
}

// Returns index of arm selected by UCB1
size_t Arbiter::select() const {
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

size_t Arbiter::apply(Ast_entry& genome, std::shared_ptr<Grammar> grammar) {
    size_t idx = select();

    total_trials += 1;        
    arms[idx].apply(genome, grammar);

    return idx;
}

void Arbiter::record(size_t arm_idx, bool cell_discovered) {
    arms[arm_idx].hill_climb(cell_discovered);
}

void Arbiter::print_stats(std::ostream& out) const {
    std::cout << std::endl;
    out << "=== Mutation pass stats ===" << std::endl;
    for (const auto& arm : arms) {
        out << arm.get_name()
            << CYAN("  trials = ") << arm.get_total_mutation_trials()
            << CYAN("  n_discovered_cells = ") << arm.get_n_discovered_cells()
            << CYAN("  success rate = ") << arm.success_rate()
            << CYAN("  ratio = ") << arm.get_blockwise_ratio()
            << std::endl;
    }
    out << CYAN("==========================") << std::endl;
    std::cout << std::endl;
}
