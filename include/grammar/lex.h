#ifndef LEX_H
#define LEX_H

#include <regex>
#include <stdio.h>
#include "string.h"
#include <stdlib.h>
#include <fstream>

#include <result.h>

namespace Token {

    enum Kind {
        _EOF = 0,
        RULE_KINDS_TOP,
/*
        RULE_KINDS_TOP: add new rule types below
*/
        RULE,
        H,
        X,
        Y,
        Z,
        RZ,
        RX,
        RY,
        U1,
        S,
        SDG,
        T,
        TDG,
        V,
        VDG,
        PHASEDXPOWGATE,
        PROJECT_Z,
        MEASURE_AND_RESET,
        MEASURE,
        CX,
        CY,
        CZ,
        CCX,
        U2,
        CNOT,
        CH,
        CRZ,
        U3,
        CSWAP,
        TOFFOLI,
        U,
        PHASED_X,
        BARRIER,
        SUBROUTINE_DEFS,
        BLOCK,
        BODY,
        QUBIT_DEFS,
        QUBIT_DEFS_DISCARD,
        BIT_DEFS,
        QUBIT_DEF,
        QUBIT_DEF_DISCARD,
        BIT_DEF,
        REGISTER_QUBIT_DEF,
        REGISTER_QUBIT_DEF_DISCARD,
        SINGULAR_QUBIT_DEF,
        SINGULAR_QUBIT_DEF_DISCARD,
        REGISTER_BIT_DEF,
        SINGULAR_BIT_DEF,
        CIRCUIT_NAME,
        FLOAT_LIST,
        FLOAT_LITERAL,
        MAIN_CIRCUIT_NAME,
        QUBIT_DEF_NAME,
        BIT_DEF_NAME,
        QUBIT,
        BIT,
        QUBIT_OP,
        GATE_OP,
        SUBROUTINE_OP,
        GATE_MAME,
        QUBIT_LIST,
        BIT_LIST,
        QUBIT_DEF_LIST,
        QUBIT_DEF_SIZE,
        BIT_DEF_LIST,
        BIT_DEF_SIZE,
        SINGULAR_QUBIT,
        REGISTER_QUBIT,
        SINGULAR_BIT,
        REGISTER_BIT,
        QUBIT_NAME,
        BIT_NAME,
        QUBIT_INDEX,
        BIT_INDEX,
        SUBROUTINE,
        CIRCUIT_ID,
        INDENT,
        DEDENT,
        IF_STMT,
        ELSE_STMT,
        ELIF_STMT,
        DISJUNCTION,
        CONJUNCTION,
        INVERSION,
        EXPRESSION,
        COMPARE_OP_BITWISE_OR_PAIR,
        NUMBER,
        SUBROUTINE_OP_ARGS,
        GATE_OP_ARGS,
        SUBROUTINE_OP_ARG,
        COMPOUND_STMT,
        COMPOUND_STMTS,
        INDENTATION_DEPTH,
        /*
            these aren't used in the lexer, but in the AST node creation. maybe will be used in the lexer if these rules are needed later
        */
        REGISTER_RESOURCE,  
        REGISTER_RESOURCE_DEF,
        SINGULAR_RESOURCE,
        SINGULAR_RESOURCE_DEF,
        RESOURCE_DEF,
        /*
            ----------------------------------------------------------------------------------------------------------------------------------
        */
    
        /*
            RULE_KINDS_BOTTOM: add new rule kinds above
        */
        RULE_KINDS_BOTTOM,

        GRAMMAR_SYNTAX_TOP,
        /*
            Tokens that aren't special rule types, but rather, are syntax used in the language
            Add new syntax below
        */
        SEPARATOR,
        RULE_START,
        RULE_APPEND,
        RULE_END,
        SYNTAX,
        LPAREN,
        LBRACK,
        LBRACE,
        RPAREN,
        RBRACK,
        RBRACE,
        ZERO_OR_MORE,
        ONE_OR_MORE,
        OPTIONAL,
        ARROW,
        INTERNAL,
        EXTERNAL,
        OWNED,
        COMMENT,
        MULTI_COMMENT_START,
        MULTI_COMMENT_END,

        /*
            Grammar syntax end, add new syntax above
        */
        GRAMMAR_SYNTAX_BOTTOM,
    };

    inline bool is_wildcard(const Kind& kind) {
        return 
            (kind ==  OPTIONAL) || 
            (kind == ZERO_OR_MORE) || 
            (kind == ONE_OR_MORE)
            ;
    }

    inline bool is_kind_of_rule(const Kind& kind){ 
        return 
            (RULE_KINDS_TOP < kind) && 
            (RULE_KINDS_BOTTOM > kind)
            ;
    }

    inline bool is_quiet(const Kind& kind){
        return 
            (kind == MULTI_COMMENT_START)|| 
            (kind == MULTI_COMMENT_END) || 
            (kind == LBRACK) || 
            (kind == RBRACK) ||
            (kind == LBRACE) ||
            (kind == COMMENT) || 
            (kind == ARROW);
    }

    struct Token{
        std::string value;
        Kind kind;

        bool operator==(const Token& other) const {
            return (value == other.value) && (kind == other.kind);
        }

        friend std::ostream& operator<<(std::ostream& stream, const Token t){
            if(t.kind == SYNTAX) std::cout << t.kind << " " << std::quoted(t.value);        
            else std::cout << t.kind << " " << t.value;
            
            return stream;
        }
    };

    struct Rule {

        Rule(const std::string& p, const Kind& k, bool match_exact = true){
            pattern = p;
            kind = k;

            mod_pattern(match_exact);
        }

        Rule(const std::string& p, const Kind& k, std::optional<std::string> v, bool match_exact = true){
            pattern = p;
            kind = k;
            value = v;

            mod_pattern(match_exact);
        }

        void mod_pattern(const bool& match_exact){
        
            if(match_exact){
                pattern = "^" + pattern + "$";
            }
        }

        std::string pattern;
        Kind kind;

        std::optional<std::string> value = std::nullopt;
    };
    
}

namespace Lexer {

    const std::vector<Token::Rule> TOKEN_RULES = {

        Token::Rule(R"(subroutine_defs)", Token::SUBROUTINE_DEFS),
        Token::Rule(R"(block)", Token::BLOCK),
        Token::Rule(R"(body)", Token::BODY),
        Token::Rule(R"(qubit_defs)", Token::QUBIT_DEFS),
        Token::Rule(R"(qubit_defs_discard)", Token::QUBIT_DEFS_DISCARD),
        Token::Rule(R"(bit_defs)", Token::BIT_DEFS),
        Token::Rule(R"(qubit_def)", Token::QUBIT_DEF),
        Token::Rule(R"(qubit_def_discard)", Token::QUBIT_DEF_DISCARD),
        Token::Rule(R"(bit_def)", Token::BIT_DEF),
        Token::Rule(R"(register_qubit_def)", Token::REGISTER_QUBIT_DEF),
        Token::Rule(R"(singular_qubit_def)", Token::SINGULAR_QUBIT_DEF),
        Token::Rule(R"(register_qubit_def_discard)", Token::REGISTER_QUBIT_DEF_DISCARD),
        Token::Rule(R"(singular_qubit_def_discard)", Token::SINGULAR_QUBIT_DEF_DISCARD),
        Token::Rule(R"(register_bit_def)", Token::REGISTER_BIT_DEF),
        Token::Rule(R"(singular_bit_def)", Token::SINGULAR_BIT_DEF),
        Token::Rule(R"(circuit_name)", Token::CIRCUIT_NAME),
        Token::Rule(R"(float_list)", Token::FLOAT_LIST),
        Token::Rule(R"(float_literal)", Token::FLOAT_LITERAL),
        Token::Rule(R"(main_circuit_name)", Token::MAIN_CIRCUIT_NAME),
        Token::Rule(R"(qubit_def_name)", Token::QUBIT_DEF_NAME),
        Token::Rule(R"(bit_def_name)", Token::BIT_DEF_NAME),
        Token::Rule(R"(qubit)", Token::QUBIT),
        Token::Rule(R"(bit)", Token::BIT),
        Token::Rule(R"(qubit_op)", Token::QUBIT_OP),
        Token::Rule(R"(gate_op)", Token::GATE_OP),
        Token::Rule(R"(subroutine_op)", Token::SUBROUTINE_OP),
        Token::Rule(R"(gate_name)", Token::GATE_MAME),
        Token::Rule(R"(qubit_list)", Token::QUBIT_LIST),
        Token::Rule(R"(bit_list)", Token::BIT_LIST),
        Token::Rule(R"(qubit_def_list)", Token::QUBIT_DEF_LIST),
        Token::Rule(R"(qubit_def_size)", Token::QUBIT_DEF_SIZE),
        Token::Rule(R"(bit_def_list)", Token::BIT_DEF_LIST),
        Token::Rule(R"(bit_def_size)", Token::BIT_DEF_SIZE),
        Token::Rule(R"(singular_qubit)", Token::SINGULAR_QUBIT),
        Token::Rule(R"(register_qubit)", Token::REGISTER_QUBIT),
        Token::Rule(R"(singular_bit)", Token::SINGULAR_BIT),
        Token::Rule(R"(register_bit)", Token::REGISTER_BIT),
        Token::Rule(R"(qubit_name)", Token::QUBIT_NAME),
        Token::Rule(R"(bit_name)", Token::BIT_NAME),
        Token::Rule(R"(qubit_index)", Token::QUBIT_INDEX),
        Token::Rule(R"(bit_index)", Token::BIT_INDEX),
        Token::Rule(R"(subroutine)", Token::SUBROUTINE),
        Token::Rule(R"(circuit_id)", Token::CIRCUIT_ID),
        Token::Rule(R"(INDENT)", Token::INDENT),
        Token::Rule(R"(DEDENT)", Token::DEDENT),
        Token::Rule(R"(if_stmt)", Token::IF_STMT),
        Token::Rule(R"(else_stmt)", Token::ELSE_STMT),
        Token::Rule(R"(elif_stmt)", Token::ELIF_STMT),
        Token::Rule(R"(disjunction)", Token::DISJUNCTION),
        Token::Rule(R"(conjunction)", Token::CONJUNCTION),
        Token::Rule(R"(inversion)", Token::INVERSION),
        Token::Rule(R"(expression)", Token::EXPRESSION),
        Token::Rule(R"(compare_op_bitwise_or_pair)", Token::COMPARE_OP_BITWISE_OR_PAIR),
        Token::Rule(R"(NUMBER)", Token::NUMBER),
        Token::Rule(R"(subroutine_op_args)", Token::SUBROUTINE_OP_ARGS),
        Token::Rule(R"(gate_op_args)", Token::GATE_OP_ARGS),
        Token::Rule(R"(subroutine_op_arg)", Token::SUBROUTINE_OP_ARG),
        Token::Rule(R"(compound_stmt)", Token::COMPOUND_STMT),
        Token::Rule(R"(compound_stmts)", Token::COMPOUND_STMTS),
        Token::Rule(R"(indentation_depth)", Token::INDENTATION_DEPTH),

        Token::Rule(R"(h)", Token::H),
        Token::Rule(R"(x)", Token::X),
        Token::Rule(R"(y)", Token::Y),
        Token::Rule(R"(z)", Token::Z),
        Token::Rule(R"(rz)", Token::RZ),
        Token::Rule(R"(rx)", Token::RX),
        Token::Rule(R"(ry)", Token::RY),
        Token::Rule(R"(u1)", Token::U1),
        Token::Rule(R"(s)", Token::S),
        Token::Rule(R"(sdg)", Token::SDG),
        Token::Rule(R"(t)", Token::T),
        Token::Rule(R"(tdg)", Token::TDG),
        Token::Rule(R"(v)", Token::V),
        Token::Rule(R"(vdg)", Token::VDG),
        Token::Rule(R"(phased_x)", Token::PHASED_X),
        Token::Rule(R"(project_z)", Token::PROJECT_Z),
        Token::Rule(R"(measure_and_reset)", Token::MEASURE_AND_RESET),
        Token::Rule(R"(measure)", Token::MEASURE),
        Token::Rule(R"(cx)", Token::CX),
        Token::Rule(R"(cy)", Token::CY),
        Token::Rule(R"(cz)", Token::CZ),
        Token::Rule(R"(ccx)", Token::CCX),
        Token::Rule(R"(u2)", Token::U2),
        Token::Rule(R"(cnot)", Token::CNOT),
        Token::Rule(R"(ch)", Token::CH),
        Token::Rule(R"(crz)", Token::CRZ),
        Token::Rule(R"(u3)", Token::U3),
        Token::Rule(R"(cswap)", Token::CSWAP),
        Token::Rule(R"(toffoli)", Token::TOFFOLI),
        Token::Rule(R"(u)", Token::U),
        Token::Rule(R"(barrier)", Token::BARRIER),

        Token::Rule(R"(LPAREN)", Token::SYNTAX, std::make_optional<std::string>("(")),
        Token::Rule(R"(RPAREN)", Token::SYNTAX, std::make_optional<std::string>(")")),
        Token::Rule(R"(LBRACK)", Token::SYNTAX, std::make_optional<std::string>("[")),
        Token::Rule(R"(RBRACK)", Token::SYNTAX, std::make_optional<std::string>("]")),
        Token::Rule(R"(LBRACE)", Token::SYNTAX, std::make_optional<std::string>("{")),
        Token::Rule(R"(RBRACE)", Token::SYNTAX, std::make_optional<std::string>("}")),
        Token::Rule(R"(COMMA)", Token::SYNTAX, std::make_optional<std::string>(",")),
        Token::Rule(R"(SPACE)", Token::SYNTAX, std::make_optional<std::string>(" ")),
        Token::Rule(R"(DOT)", Token::SYNTAX, std::make_optional<std::string>(".")),
        Token::Rule(R"(SINGLE_QUOTE)", Token::SYNTAX, std::make_optional<std::string>("\'")),
        Token::Rule(R"(DOUBLE_QUOTE)", Token::SYNTAX, std::make_optional<std::string>("\"")),
        Token::Rule(R"(EQUALS)", Token::SYNTAX, std::make_optional<std::string>("=")),
        Token::Rule(R"(NEWLINE)", Token::SYNTAX, std::make_optional<std::string>("\n")),
        Token::Rule(R"(\".*?\"|\'.*?\')", Token::SYNTAX, false),

        Token::Rule(R"(EXTERNAL(::)?)", Token::EXTERNAL, false),
        Token::Rule(R"(INTERNAL(::)?)", Token::INTERNAL, false),
        Token::Rule(R"(OWNED(::)?)", Token::OWNED, false),

        Token::Rule(R"([a-zA-Z_]+)", Token::RULE, false),

        Token::Rule(R"(\(\*)", Token::MULTI_COMMENT_START, false),
        Token::Rule(R"(\*\))", Token::MULTI_COMMENT_END, false),
        Token::Rule(R"(=|:)", Token::RULE_START, false),
        Token::Rule(R"(\+=)", Token::RULE_APPEND, false),
        Token::Rule(R"(\|)", Token::SEPARATOR, false),
        Token::Rule(R"(;)", Token::RULE_END, false),
        Token::Rule(R"(\()", Token::LPAREN, false),
        Token::Rule(R"(\))", Token::RPAREN, false),
        Token::Rule(R"(\[)", Token::LBRACK, false),
        Token::Rule(R"(\])", Token::RBRACK, false),
        Token::Rule(R"(\{)", Token::LBRACE, false),
        Token::Rule(R"(\})", Token::RBRACE, false),
        Token::Rule(R"(\*)", Token::ZERO_OR_MORE, false),
        Token::Rule(R"(\?)", Token::OPTIONAL, false),
        Token::Rule(R"(\+)", Token::ONE_OR_MORE, false),
        Token::Rule(R"(\-\>)", Token::ARROW, false),
        Token::Rule(R"(#)", Token::COMMENT, false),
    };

    const std::string FULL_REGEX = [] {
        std::string regex = "(";

        for (size_t i = 0; i < TOKEN_RULES.size(); i++) {
            regex += TOKEN_RULES[i].pattern;

            if (i + 1 < TOKEN_RULES.size()) regex += "|";
        }
        regex += ")";

        return regex;
    }();
            
    class Lexer{
        public:
            Lexer(){}

            Lexer(std::string filename)
                :_filename(filename)
            {
                lex();
            }

            std::string remove_outer_quotes(const std::string& token){
                if ((token.size() > 2) && 
                    (((token.front() == '\"') && (token.back() == '\"')) ||
                    ((token.front() == '\'') && (token.back() == '\'')))
                ){
                    return token.substr(1, token.size() - 2);
                }
                
                return token;      
            }

            inline bool string_is(const std::string& string, const std::string& pattern){
                bool matches = std::regex_match(string, std::regex(pattern));
                return ((ignore == false) && matches) || (string == "*)") ;
            }

            void lex();

            void print_tokens() const;

            std::vector<Token::Token> get_tokens(){
                std::vector<Token::Token> tokens = result.get_ok();
                return tokens;
            }

        private:
            Result<std::vector<Token::Token>> result;
            std::string _filename = "bnf.bnf"; 
            bool ignore = false;
            
    };
}

#endif


