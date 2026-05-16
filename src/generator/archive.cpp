#include <archive.h>
#include <pass.h>
#include <chrono>
#include <ast_utils.h>

void Archive::dump(const fs::path& path){
    std::ofstream stream(path);

    stream << "{\n" << "\"dims\": " << std::endl;

    dummy_info.dump_feature_vecs(stream);

    stream << ",\n" << "\"cells\" : [\n";
    
    for (size_t i = 0; i < archive.size(); i++){
        const Cell& cell = archive[i];
        float q = cell.get_quality();

        stream << "  {";
        stream << "\"index\": " << i << ", ";
        stream << "\"occupied\": " << (cell.empty() ? "false" : "true") << ", ";
        stream << "\"quality\": " << (cell.empty() ? 0.0f : q);
        stream << "}";
        if (i < archive.size() - 1) stream << ",";
        stream << "\n";
    }

    stream << "]}\n";

    INFO("Archive JSON dumped at " + path.string());
}

float Archive::archive_fill_ratio(){
    return (float)filled_archive_indices.size() / (float)archive.size();
};

float Archive::archive_av_quality(){
    float total_quality = 0.0;

    if (archive.size() == 0) return total_quality;

    for (const Cell& cell : archive){
        total_quality += cell.get_quality();
    }

    return total_quality / (float)archive.size();
};

bool Archive::place(const Ast_entry& genome){
    Info info(genome);
    unsigned int archive_index = info.get_archive_index();

    // info.dump_feature_vecs(std::cout);
    // std::cout << std::endl;

    bool new_placement = archive[archive_index].place(genome, info.quality());

    if (new_placement){
        filled_archive_indices.push_back(archive_index);
    }

    return new_placement;
}

void Archive::init_archive(){
    for (auto genome : init_genomes){

        // make the circuit small to varying degrees
        Erase_child(genome, COMPOUND_STMTS, random_float(1.0, 0.0)).apply();

        place(genome);
    }

    std::cout << std::endl;

    INFO("Init archive average quality " + std::to_string(archive_av_quality()));
    INFO("Init archive fill ratio  " + std::to_string(archive_fill_ratio()));
    
    std::cout << std::endl;

    // dump init archive in JSON
    dump(output_dir / "init_archive.json");
}

static void add_family_pairs(Arbiter& arbiter, const std::vector<Token_kind>& family, std::vector<std::vector<Token_kind>>& out) {        
    for (size_t i = 0; i < family.size(); ++i) {
        for (size_t j = 0; j < family.size(); ++j) {
            if (i == j) continue;

            Token_kind a = family[i];
            Token_kind b = family[j];

            std::vector<Token_kind> pair = {a,b};

            out.push_back(pair);

            arbiter.add(std::to_string(a) + "--" + std::to_string(b),
                [pair](Ast_entry& e, std::shared_ptr<Grammar> g, float r) {
                    return std::make_unique<Add_gate_chain>(e, g, r, pair);
                }
            );      
        }
    }
}

static void register_passes_to_arbiter(Arbiter& arbiter) {

    // get all needed gate pairs
    std::vector<std::vector<Token_kind>> interesting_gate_pairs;

    // self inverse pairs, create vector with repeated token kind
    for (const auto& token_kind : SELF_INVERSE_PAIRS){
        std::vector<Token_kind> pair = {token_kind, token_kind};

        interesting_gate_pairs.push_back(pair);

        arbiter.add(std::to_string(token_kind) + "--" + std::to_string(token_kind),
            [pair](Ast_entry& e, std::shared_ptr<Grammar> g, float r) {
                return std::make_unique<Add_gate_chain>(e, g, r, pair);
            }
        );
    }

    // other inverse pairs, add pair and its reverse
    for (size_t i = 0; i < INVERSE_PAIRS.size(); i++){
        std::vector<Token_kind> pair = INVERSE_PAIRS[i];

        interesting_gate_pairs.push_back(pair);   
        
        arbiter.add(std::to_string(pair[0]) + "--" + std::to_string(pair[1]),
            [pair](Ast_entry& e, std::shared_ptr<Grammar> g, float r) {
                return std::make_unique<Add_gate_chain>(e, g, r, pair);
            }
        );
    }

    // commutation bases
    add_family_pairs(arbiter, X_FAMILY, interesting_gate_pairs);
    add_family_pairs(arbiter, Y_FAMILY, interesting_gate_pairs);
    add_family_pairs(arbiter, Z_FAMILY, interesting_gate_pairs);
    
    arbiter.add("Prune",
        [](Ast_entry& e, std::shared_ptr<Grammar>, float r) {
            return std::make_unique<Erase_child>(e, COMPOUND_STMTS, r);
        }
    );

    arbiter.add("remove inverse pairs",
        [](Ast_entry& e, std::shared_ptr<Grammar>, float r) {
            return std::make_unique<Remove_gate_chain>(e, r, is_inverse_pair);
        }
    );

    arbiter.add("remove commutative pairs",
        [](Ast_entry& e, std::shared_ptr<Grammar>, float r) {
            return std::make_unique<Remove_gate_chain>(e, r, is_commutative_pair);
        }
    );

    arbiter.add("combine some passes",
        [interesting_gate_pairs](Ast_entry& e, std::shared_ptr<Grammar> g, float r) {
            std::vector<std::unique_ptr<Pass>> rules;

            for (const auto& pair : interesting_gate_pairs){
                // pick half the pairs randomly for combine pass
                if(random_float(1.0, 0.0) < 0.5){
                    rules.push_back(std::make_unique<Add_gate_chain>(e, g, r, pair));
                }
            }
            return std::make_unique<Combine>(e, r, std::move(rules));
        }
    );
}

void Archive::fill_archive(std::shared_ptr<Grammar> grammar){
    unsigned int max_evals = 200000;
    unsigned int total_evals = 0;

    unsigned int patience = 2000;
    unsigned int evals_since_discovery = 0;

    Arbiter arbiter;
    register_passes_to_arbiter(arbiter);

    while((total_evals < max_evals) && (evals_since_discovery < patience)){
        unsigned int n_filled_cells = filled_archive_indices.size();
        unsigned int random_index = filled_archive_indices[random_uint(n_filled_cells - 1)];

        Cell cell = archive[random_index];

        Ast_entry genome = cell.get_genome().clone();

        #if 0
            // testbed for mutations

            genome.get_ast()->print_program(std::cout);
            std::cout << "\n================" << std::endl;
            getchar();

            Add_gate_chain(genome, grammar, 1.0, std::vector<Token_kind>{CX, CX}).apply();
            Add_gate_chain(genome, grammar, 1.0, std::vector<Token_kind>{CCX, CCX}).apply();
            Remove_gate_chain(genome, 1.0, is_inverse_pair).apply();

            std::vector<std::unique_ptr<Pass>> rules;

            rules.push_back(std::make_unique<Add_gate_chain>(genome, grammar, 1.0, std::vector<Token_kind>{CX, CX}));
            rules.push_back(std::make_unique<Add_gate_chain>(genome, grammar, 1.0, std::vector<Token_kind>{CCX, CCX}));

            Combine(genome, 1.0, std::move(rules)).apply();

            genome.get_ast()->print_program(std::cout);
            std::cout << "\n================" << std::endl;
            getchar();
        #else
            size_t Pass_idx = arbiter.apply(genome, grammar);
            bool cell_discovered = place(genome);
            arbiter.record(Pass_idx, cell_discovered);

            if (cell_discovered){
                evals_since_discovery = 0;

                INFO("fill ratio = " + std::to_string(archive_fill_ratio()));
                INFO("av quality = " + std::to_string(archive_av_quality()));
        
                std::cout << std::endl;

            } else {
                evals_since_discovery += 1;
            }

        #endif

        total_evals += 1;
    }

    std::cout << std::endl;

    INFO("Final archive average quality " + std::to_string(archive_av_quality()));
    INFO("Final archive fill ratio " + std::to_string(archive_fill_ratio()));

    dump(output_dir / "final_archive.json");

    arbiter.print_stats(std::cout);
}

std::vector<Ast_entry> Archive::get_best_genomes(){
    std::vector<Ast_entry> out;

    for(const Cell& cell : archive){
        if (!cell.empty()){
            out.push_back(cell.get_genome());
        }
    }

    return out;
}
