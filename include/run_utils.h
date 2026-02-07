#ifndef RUN_UTILS_H
#define RUN_UTILS_H

enum Clamp_dir {
    NO_CLAMP,
    CLAMP_DOWN,
    CLAMP_UP
};

template <typename T>
struct Expected{
    std::string rule_name;
    Scope scope;
    T value;
    T dflt;
    Clamp_dir cd;

    Expected(std::string _rule_name, T _dflt, Clamp_dir _cd = NO_CLAMP) :
        rule_name(_rule_name),
        scope(Scope::GLOB),
        value(_dflt),
        dflt(_dflt),
        cd(_cd)
    {}

    Expected(std::string _rule_name, Scope _scope, T _dflt) :
        rule_name(_rule_name),
        scope(_scope),
        value(_dflt),
        dflt(_dflt)
    {}
};

struct Control {
    unsigned int GLOBAL_SEED_VAL;
    bool render;
    bool swarm_testing;
    bool run_mutate;
    bool step;
    std::string ext;

    std::vector<Expected<unsigned int>> expected_values;
    std::vector<Expected<std::shared_ptr<Rule>>> expected_rules;

    unsigned int get_value(std::string name) const {
        for(auto& exp : expected_values){
            if(exp.rule_name == name){
                return exp.value;
            }
        }

        throw std::runtime_error("Expected value " + name + " not found in control");
    }

    std::shared_ptr<Rule> get_rule(std::string name, Scope scope = Scope::GLOB) const {
        for(auto& exp : expected_rules){
            if((exp.rule_name == name) && (exp.scope == scope)){
                return exp.value;
            }
        }

        throw std::runtime_error("Expected rule " + name + " " + STR_SCOPE(scope) + " not found in control");
    }
};

void init_global_seed(Control& control, std::optional<unsigned int> user_seed = std::nullopt);



#endif
