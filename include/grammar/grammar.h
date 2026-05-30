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
#include <expr.h>

/// @brief `branch` feeds `rule` with newly contructed branches
struct Current {
    std::shared_ptr<Rule> rule = nullptr;
    Branch branch;
    Print_mode print_mode = Print_mode::DEFAULT;

    Current(){}

    Current(std::shared_ptr<Rule> _rule) :
        rule(_rule)
    {}
};

class Grammar{

    public:
        Grammar(){}

        Grammar(const fs::path& filename, std::vector<Token>& meta_grammar_tokens);

        std::string dig_to_syntax(const std::string& rule_name) const;

        std::shared_ptr<Rule> get_rule_pointer_if_exists(const std::string& name, const Scope& scope) const;

        std::shared_ptr<Rule> get_rule_pointer(const Token& token, const Scope& scope);

        Token_kind parse_token();

        inline void build_grammar(Token_kind until = _EOF){
            while(parse_token() != until);
        }

        void print_rules() const;

        void print_tokens() const;

        bool is_rule(const std::string& rule_name, const Scope& scope);

        inline std::string get_name(){return name;}

        inline std::string get_path(){return path.string();}

        friend std::ostream& operator<<(std::ostream& stream, const Grammar& grammar){
            for(const auto& p : grammar.rule_pointers){
                std::cout << *p << std::endl;
            }

            return stream;
        }

    private:
        [[noreturn]]
        void grammar_error(const std::string& msg, std::source_location loc = std::source_location::current());

        bool check(const std::string& expected, std::source_location loc = std::source_location::current());

        void consume(int n = 1);

        void consume(const std::string& expected, std::source_location loc = std::source_location::current());

        void peek();

        void back();

        void add_term_to_current_branch(const Term& term);

        void add_term_to_current_branch(const Token& token);

        void add_branch_to_current_rule();

        void add_expr_to_last_term();

        Current& current();

        template<typename NextFunc>
        std::unique_ptr<Expr> parse_binary_op(NextFunc parse_next, std::initializer_list<std::string> valid_ops);

        std::unique_ptr<Expr> parse_block();

        std::unique_ptr<Expr> expr();

        std::unique_ptr<Expr> for_expr(); 

        std::unique_ptr<Expr> if_expr();

        std::unique_ptr<Expr> logic_expr();

        std::unique_ptr<Expr> additive_expr();

        std::unique_ptr<Expr> multiplicative_expr();

        std::unique_ptr<Expr> factor();

        void complete_rule();
        
        /*
            moving through the tokens
        */
        std::vector<Token> tokens;
        size_t num_tokens = 0;
        size_t token_pointer = 0;
        Token curr_token;
        Token next_token;
        Token prev_token;

        // built expr
        std::shared_ptr<Expr> curr_expr;

        /*
            current rule and branch being built
        */
        std::stack<Current> stack;

        // default scopes for all rules
        Scope rule_def_scope = Scope::GLOB;
        Scope rule_decl_scope = Scope::GLOB;

        unsigned int new_rule_counter = 0;
        std::vector<std::shared_ptr<Rule>> rule_pointers;

        Lexer lexer;
        std::string name;
        fs::path path;
};

#endif
