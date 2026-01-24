#include <run.h>
#include <ast.h>
#include <lex.h>
#include <params.h>

const fs::path Run::OUTPUT_DIR = fs::path(__FILE__).parent_path().parent_path() / fs::path(QuteFuzz::OUTPUTS_FOLDER_NAME);

Run::Run(const std::string& _grammars_dir) : grammars_dir(_grammars_dir) {
    std::vector<Token> meta_grammar_tokens;

    fs::create_directory(OUTPUT_DIR);

    // build all grammars
    try{

        if(fs::exists(grammars_dir) && fs::is_directory(grammars_dir)){
            /*
                find meta grammar
            */
            for(auto& file : fs::directory_iterator(grammars_dir)){

                if(file.is_regular_file() && (file.path().stem() == QuteFuzz::META_GRAMMAR_NAME)){
                    Lexer lexer(file.path().string());
                    meta_grammar_tokens = std::move(lexer.get_tokens());

                    // remove EOF from meta grammar's tokens
                    meta_grammar_tokens.pop_back();
                    break;
                }
            }

            /*
                parse all other grammars, appending the tokens grammar to each one
            */
            for(auto& file : fs::directory_iterator(grammars_dir)){

                if(file.is_regular_file() && (file.path().extension() == ".qf") && (file.path().stem() != QuteFuzz::META_GRAMMAR_NAME)){

                    Grammar grammar(file, meta_grammar_tokens);
                    grammar.build_grammar();

                    std::string name = grammar.get_name();
                    std::cout << "Built " << name << std::endl;

                    generators[name] = std::make_shared<Generator>(grammar);
                }
            }
        }

    } catch (const fs::filesystem_error& error) {
        std::cout << error.what() << std::endl;
    }

}

void Run::set_grammar(Control& control){

    std::string grammar_name = tokens[0], entry_name;
    tokenise(tokens[1], ',');

    entry_name = tokens[0];

    U8 scope = 0;

    for(const auto& t : tokens){
        scope |= ((t == "E") & EXTERNAL_SCOPE);
        scope |= ((t == "I") & INTERNAL_SCOPE);
        scope |= ((t == "O") & OWNED_SCOPE);
    }

    current_generator = generators[grammar_name];
    current_generator->set_grammar_entry(entry_name, scope);

    setup_output_dir(grammar_name);

    control.ext = current_generator->get_grammar()->dig("EXTENSION");

    if(control.ext == ""){
        throw std::runtime_error(ANNOT("Grammar " + grammar_name + " does not define an extension"));
    }

    std::shared_ptr<Grammar> current_grammar = current_generator->get_grammar();

    for(auto& exp : control.expected_rules){
        exp.value = current_grammar->get_rule_pointer_if_exists(exp.rule_name, exp.scope); 

        if(exp.value == nullptr){
            throw std::runtime_error("Rule " + exp.rule_name + STR_SCOPE(exp.scope) + "MUST be defined either by default in the meta-grammar, or redefined in the input grammar");
        }
    }

    for(auto& exp : control.expected_values){
        if(exp.cd == CLAMP_UP){
            exp.value = std::max(safe_stoul(current_grammar->dig(exp.rule_name), exp.dflt), exp.dflt);
        } else {
            exp.value = std::min(safe_stoul(current_grammar->dig(exp.rule_name), exp.dflt), exp.dflt);
        }
    }

    current_generator->set_grammar_control(control);
}

void Run::tokenise(const std::string& command, const char& delim){

    std::stringstream ss(command);
    std::string token;

    tokens.clear();

    while(std::getline(ss, token, delim)){
        tokens.push_back(token);
    }
}

void Run::remove_all_in_dir(const fs::path& dir){
    if(fs::exists(dir) && fs::is_directory(dir)){
        for(const auto& entry : fs::directory_iterator(dir)){
            fs::remove_all(entry.path());
        }
    }
}

void Run::help(){
    std::cout << "-> Type enter to write to a file" << std::endl;
    std::cout << "-> \"grammar_name grammar_entry\" : command to set grammar " << std::endl;
    std::cout << "  These are the known grammar rules: " << std::endl;

    for(const auto& generator : generators){
        std::cout << generator.second << std::endl;
    }
}

void Run::loop(){

    std::string current_command;
    Control qf_control = {
        .GLOBAL_SEED_VAL = 0,
        .render = false,
        .swarm_testing = false,
        .run_mutate = false,
        .ext = ".text",
        .expected_values = {
            Expected<unsigned int>("MIN_NUM_QUBITS", QuteFuzz::MIN_QUBITS, CLAMP_UP),
            Expected<unsigned int>("MIN_NUM_BITS", QuteFuzz::MIN_BITS, CLAMP_UP),
            Expected<unsigned int>("MAX_NUM_QUBITS", QuteFuzz::MAX_QUBITS, CLAMP_DOWN),
            Expected<unsigned int>("MAX_NUM_BITS", QuteFuzz::MAX_BITS, CLAMP_DOWN),
            Expected<unsigned int>("MAX_NUM_SUBROUTINES", QuteFuzz::MAX_NUM_SUBROUTINES, CLAMP_DOWN),
            Expected<unsigned int>("NESTED_MAX_DEPTH", QuteFuzz::NESTED_MAX_DEPTH, CLAMP_DOWN)
        },
        .expected_rules = {
            Expected<std::shared_ptr<Rule>>("qubit_def", INTERNAL_SCOPE, nullptr),
            Expected<std::shared_ptr<Rule>>("qubit_def", EXTERNAL_SCOPE, nullptr),
            Expected<std::shared_ptr<Rule>>("qubit_def", GLOBAL_SCOPE, nullptr),
            Expected<std::shared_ptr<Rule>>("bit_def", INTERNAL_SCOPE, nullptr),
            Expected<std::shared_ptr<Rule>>("bit_def", EXTERNAL_SCOPE, nullptr),
            Expected<std::shared_ptr<Rule>>("bit_def", GLOBAL_SCOPE, nullptr)
        },
    };

    init_global_seed(qf_control);

    while(true){
        std::cout << "> ";

        std::getline(std::cin, current_command);
        tokenise(current_command, ' ');

        if(tokens.size() == 2){
            if (is_grammar(tokens[0])){
                set_grammar(qf_control);
            } else if (tokens[0] == "seed") {
                init_global_seed(qf_control, safe_stoul(tokens[1], 0));
                INFO("Global seed set to " + std::to_string(qf_control.GLOBAL_SEED_VAL));
            }

        } else if(current_command == "h"){
            help();

        } else if (current_command == "render"){
            qf_control.render = !qf_control.render;
            INFO("Rendering " + FLAG_STATUS(qf_control.render));
    
        } else if (current_command == "swarm_testing") {
                qf_control.swarm_testing = !qf_control.swarm_testing;
                INFO("Swarm testing mode " + FLAG_STATUS(qf_control.swarm_testing));

        } else if (current_command == "mutate"){
            qf_control.run_mutate = !qf_control.run_mutate;
            INFO("Mutation mode " + FLAG_STATUS(qf_control.run_mutate)); 

        } else if (current_command == "quit"){
            current_generator.reset();
            generators.clear();
            break;

        } else if(current_generator != nullptr){
            if (current_command == "pt") {
                current_generator->print_tokens();

            } else if (current_command == "pg") {
                current_generator->print_grammar();

            } else if ((n_programs = safe_stoul(current_command, 1))){
                remove_all_in_dir(current_output_dir);

                for(size_t build_counter = 0; build_counter < n_programs.value_or(0); build_counter++){
                    unsigned int seed = random_uint(UINT32_MAX);

                    std::ofstream stream = get_stream(current_output_dir, "regression_seed.txt");
                    stream << qf_control.GLOBAL_SEED_VAL << std::endl;
                    stream.close();

                    fs::path current_circuit_dir = current_output_dir / ("circuit" + std::to_string(build_counter));
                    current_generator->ast_to_program(current_circuit_dir, qf_control, seed);
                }

                init_global_seed(qf_control);
            }

        } else {
            std::cout << "\"" << current_command << "\" is unknown" << std::endl;

        }
    }
}
