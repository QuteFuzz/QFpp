#ifndef PARSER_H
#define PARSER_H

#include <lex.h>
#include "node.h"
#include "term.h"
#include <errno.h>
#include <math.h>
#include <unordered_map>
#include <rule.h>
#include <cctype>
#include <algorithm>

class Grammar{

    public:
        Grammar(){}

        Grammar(const fs::path& filename, std::vector<Token>& meta_grammar_tokens);

        void consume(int n);

        void consume(const Token_kind kind);

        void peek();

        std::string dig_to_syntax(const std::string& rule_name) const;

        std::shared_ptr<Rule> get_rule_pointer_if_exists(const std::string& name, const U8& scope = NO_SCOPE) const;

        std::shared_ptr<Rule> get_rule_pointer(const Token& token, const U8& scope = NO_SCOPE);

        inline void reset_current_branches(){current_branches.clear();}

        void add_current_branches_to_rule();

        /// we just completed a rule, add the current branch to the rule
        /// Called at end of rule, and at each branch seprator
        inline void complete_rule(){ add_current_branches_to_rule(); }

        inline void increment_nesting_depth_base(){
            if(nesting_depth == nesting_depth_base) {
                nesting_depth_base += 1;
                nesting_depth = nesting_depth_base;
            }
        }

        void extend_current_branches(const Token& wildcard);

        void add_term_to_current_branches(const Token& tokens);

        void add_term_to_branch(const Token& token, Branch& branch);

        void build_grammar();

        friend std::ostream& operator<<(std::ostream& stream, const Grammar& grammar){
            for(const auto& p : grammar.rule_pointers){
                std::cout << *p << std::endl;
            }

            return stream;
        }

        void print_rules() const;

        void print_tokens() const;

        bool is_rule(const std::string& rule_name, const U8& scope);

        inline std::string get_name(){return name;}

        inline std::string get_path(){return path.string();}

        inline void set_control(const Control& _control){
            control = _control;
        }

    private:
        std::vector<Token> tokens;
        size_t num_tokens = 0;
        size_t token_pointer = 0;
        Result<Token> curr_token;
        Result<Token> next_token;
        Token prev_token;

        std::string range_start = "", range_end = "";

        std::vector<Branch> current_branches;
        std::shared_ptr<Rule> current_rule = nullptr;

        U8 rule_def_scope = NO_SCOPE;
        U8 rule_decl_scope = NO_SCOPE;

        unsigned int nesting_depth_base = 0;
        unsigned int nesting_depth = nesting_depth_base;
        std::vector<std::shared_ptr<Rule>> rule_pointers;

        Lexer lexer;
        std::string name;
        fs::path path;

        Control control;
};

#endif
