#ifndef PARSER_H
#define PARSER_H

#include <lex.h>
#include "node.h"
#include "qf_term.h"
#include <errno.h>
#include <math.h>
#include <unordered_map>
#include <rule.h>
#include <cctype>
#include <algorithm>

/// @brief `branch` feeds `rule` with newly contructed branches
struct Current {
    std::shared_ptr<Rule> rule;
    Branch branch;

    Current(){}

    Current(std::shared_ptr<Rule> _rule) :
        rule(_rule)
    {}
};

class Grammar{

    public:
        Grammar(){}

        Grammar(const fs::path& filename, std::vector<Token>& meta_grammar_tokens);

        inline void error(const std::string& msg, Token token){
            std::cout << *stack.top().rule << std::endl;
            std::cout << "Token: " << token << std::endl;
            throw std::runtime_error(msg);
        }

        void consume(int n);

        void consume(const Token_kind kind);

        void consume(const std::string& val);

        void peek();

        std::string dig_to_syntax(const std::string& rule_name) const;

        std::shared_ptr<Rule> get_rule_pointer_if_exists(const std::string& name, const Scope& scope) const;

        std::shared_ptr<Rule> get_rule_pointer(const Token& token, const Scope& scope);

        void add_term_to_current_branch(const Token& token);

        void add_branch_to_current_rule();

        void add_constraint_to_last_term(const Term_constraint& constraint);

        void build_grammar();

        void print_rules() const;

        void print_tokens() const;

        bool is_rule(const std::string& rule_name, const Scope& scope);

        inline std::string get_name(){return name;}

        inline std::string get_path(){return path.string();}

        /// complete rule (does stack pop to return to home if needed)
        inline void complete_rule(){
            add_branch_to_current_rule();
            stack.pop();
        }

        inline void set_meta_func(const Token_kind& kind){
            if (kind == NAME){
                rule_decl_meta_func = Meta_func::NAME;
            } else {
                throw std::runtime_error("Unknown meta function");
            }
        }

        friend std::ostream& operator<<(std::ostream& stream, const Grammar& grammar){
            for(const auto& p : grammar.rule_pointers){
                std::cout << *p << std::endl;
            }

            return stream;
        }

        // inline void set_control(const Control& _control){
        //     control = _control;
        // }

    private:
        /*
            moving through the tokens
        */
        std::vector<Token> tokens;
        size_t num_tokens = 0;
        size_t token_pointer = 0;
        Result<Token> curr_token;
        Result<Token> next_token;
        Token prev_token;

        /*
            current rule and branch being built
        */
        std::stack<Current> stack;

        // default scopes for all rules
        Scope rule_def_scope = Scope::GLOB;
        Scope rule_decl_scope = Scope::GLOB;
        Meta_func rule_decl_meta_func = Meta_func::NONE;
        bool setting_term_constraint = false;

        // unsigned int nesting_depth_base = 0;
        unsigned int new_rule_counter = 0;
        std::vector<std::shared_ptr<Rule>> rule_pointers;

        Lexer lexer;
        std::string name;
        fs::path path;
};

#endif
