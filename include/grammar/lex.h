#ifndef LEX_H
#define LEX_H

#include <regex>
#include <stdio.h>
#include "string.h"
#include <stdlib.h>
#include <fstream>

#include <result.h>

enum Token_kind {
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

inline bool is_wildcard(const Token_kind& kind) {
    return 
        (kind ==  OPTIONAL) || 
        (kind == ZERO_OR_MORE) || 
        (kind == ONE_OR_MORE)
        ;
}

inline bool is_kind_of_rule(const Token_kind& kind){ 
    return 
        (RULE_KINDS_TOP < kind) && 
        (RULE_KINDS_BOTTOM > kind)
        ;
}

inline bool is_quiet(const Token_kind& kind){
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
    Token_kind kind;

    bool operator==(const Token& other) const {
        return (value == other.value) && (kind == other.kind);
    }

    friend std::ostream& operator<<(std::ostream& stream, const Token t){
        if(t.kind == SYNTAX) std::cout << t.kind << " " << std::quoted(t.value);        
        else std::cout << t.kind << " " << t.value;
        
        return stream;
    }
};

struct Regex_matcher {

    Regex_matcher(const std::string& p, const Token_kind& k, bool match_exact = true){
        pattern = p;
        kind = k;

        if(match_exact){
            pattern = "^" + pattern + "$";
        }
    }

    Regex_matcher(const std::string& p, const Token_kind& k, std::optional<std::string> v, bool match_exact = true){
        pattern = p;
        kind = k;
        value = v;

        if(match_exact){
            pattern = "^" + pattern + "$";
        }
    }

    std::string pattern;
    Token_kind kind;

    std::optional<std::string> value = std::nullopt;
};

const std::vector<Regex_matcher> TOKEN_RULES = {

    Regex_matcher(R"(subroutine_defs)", SUBROUTINE_DEFS),
    Regex_matcher(R"(block|subroutine_block)", BLOCK, false),
    Regex_matcher(R"(body|subroutine_body)", BODY, false),
    Regex_matcher(R"(qubit_defs)", QUBIT_DEFS),
    Regex_matcher(R"(qubit_defs_discard)", QUBIT_DEFS_DISCARD),
    Regex_matcher(R"(bit_defs)", BIT_DEFS),
    Regex_matcher(R"(qubit_def)", QUBIT_DEF),
    Regex_matcher(R"(qubit_def_discard)", QUBIT_DEF_DISCARD),
    Regex_matcher(R"(bit_def)", BIT_DEF),
    Regex_matcher(R"(register_qubit_def)", REGISTER_QUBIT_DEF),
    Regex_matcher(R"(singular_qubit_def)", SINGULAR_QUBIT_DEF),
    Regex_matcher(R"(register_qubit_def_discard)", REGISTER_QUBIT_DEF_DISCARD),
    Regex_matcher(R"(singular_qubit_def_discard)", SINGULAR_QUBIT_DEF_DISCARD),
    Regex_matcher(R"(register_bit_def)", REGISTER_BIT_DEF),
    Regex_matcher(R"(singular_bit_def)", SINGULAR_BIT_DEF),
    Regex_matcher(R"(circuit_name)", CIRCUIT_NAME),
    Regex_matcher(R"(float_list)", FLOAT_LIST),
    Regex_matcher(R"(float_literal)", FLOAT_LITERAL),
    Regex_matcher(R"(main_circuit_name)", MAIN_CIRCUIT_NAME),
    Regex_matcher(R"(qubit_def_name)", QUBIT_DEF_NAME),
    Regex_matcher(R"(bit_def_name)", BIT_DEF_NAME),
    Regex_matcher(R"(qubit)", QUBIT),
    Regex_matcher(R"(bit)", BIT),
    Regex_matcher(R"(qubit_op)", QUBIT_OP),
    Regex_matcher(R"(gate_op)", GATE_OP),
    Regex_matcher(R"(subroutine_op)", SUBROUTINE_OP),
    Regex_matcher(R"(gate_name)", GATE_MAME),
    Regex_matcher(R"(qubit_list)", QUBIT_LIST),
    Regex_matcher(R"(bit_list)", BIT_LIST),
    Regex_matcher(R"(qubit_def_list)", QUBIT_DEF_LIST),
    Regex_matcher(R"(qubit_def_size)", QUBIT_DEF_SIZE),
    Regex_matcher(R"(bit_def_list)", BIT_DEF_LIST),
    Regex_matcher(R"(bit_def_size)", BIT_DEF_SIZE),
    Regex_matcher(R"(singular_qubit)", SINGULAR_QUBIT),
    Regex_matcher(R"(register_qubit)", REGISTER_QUBIT),
    Regex_matcher(R"(singular_bit)", SINGULAR_BIT),
    Regex_matcher(R"(register_bit)", REGISTER_BIT),
    Regex_matcher(R"(qubit_name)", QUBIT_NAME),
    Regex_matcher(R"(bit_name)", BIT_NAME),
    Regex_matcher(R"(qubit_index)", QUBIT_INDEX),
    Regex_matcher(R"(bit_index)", BIT_INDEX),
    Regex_matcher(R"(subroutine)", SUBROUTINE),
    Regex_matcher(R"(circuit_id)", CIRCUIT_ID),
    Regex_matcher(R"(INDENT)", INDENT),
    Regex_matcher(R"(DEDENT)", DEDENT),
    Regex_matcher(R"(if_stmt)", IF_STMT),
    Regex_matcher(R"(else_stmt)", ELSE_STMT),
    Regex_matcher(R"(elif_stmt)", ELIF_STMT),
    Regex_matcher(R"(disjunction)", DISJUNCTION),
    Regex_matcher(R"(conjunction)", CONJUNCTION),
    Regex_matcher(R"(inversion)", INVERSION),
    Regex_matcher(R"(expression)", EXPRESSION),
    Regex_matcher(R"(compare_op_bitwise_or_pair)", COMPARE_OP_BITWISE_OR_PAIR),
    Regex_matcher(R"(NUMBER)", NUMBER),
    Regex_matcher(R"(subroutine_op_args)", SUBROUTINE_OP_ARGS),
    Regex_matcher(R"(gate_op_args)", GATE_OP_ARGS),
    Regex_matcher(R"(subroutine_op_arg)", SUBROUTINE_OP_ARG),
    Regex_matcher(R"(compound_stmt)", COMPOUND_STMT),
    Regex_matcher(R"(compound_stmts)", COMPOUND_STMTS),
    Regex_matcher(R"(indentation_depth)", INDENTATION_DEPTH),

    Regex_matcher(R"(h)", H),
    Regex_matcher(R"(x)", X),
    Regex_matcher(R"(y)", Y),
    Regex_matcher(R"(z)", Z),
    Regex_matcher(R"(rz)", RZ),
    Regex_matcher(R"(rx)", RX),
    Regex_matcher(R"(ry)", RY),
    Regex_matcher(R"(u1)", U1),
    Regex_matcher(R"(s)", S),
    Regex_matcher(R"(sdg)", SDG),
    Regex_matcher(R"(t)", T),
    Regex_matcher(R"(tdg)", TDG),
    Regex_matcher(R"(v)", V),
    Regex_matcher(R"(vdg)", VDG),
    Regex_matcher(R"(phased_x)", PHASED_X),
    Regex_matcher(R"(project_z)", PROJECT_Z),
    Regex_matcher(R"(measure_and_reset)", MEASURE_AND_RESET),
    Regex_matcher(R"(measure)", MEASURE),
    Regex_matcher(R"(cx)", CX),
    Regex_matcher(R"(cy)", CY),
    Regex_matcher(R"(cz)", CZ),
    Regex_matcher(R"(ccx)", CCX),
    Regex_matcher(R"(u2)", U2),
    Regex_matcher(R"(cnot)", CNOT),
    Regex_matcher(R"(ch)", CH),
    Regex_matcher(R"(crz)", CRZ),
    Regex_matcher(R"(u3)", U3),
    Regex_matcher(R"(cswap)", CSWAP),
    Regex_matcher(R"(toffoli)", TOFFOLI),
    Regex_matcher(R"(u)", U),
    Regex_matcher(R"(barrier)", BARRIER),

    Regex_matcher(R"(LPAREN)", SYNTAX, std::make_optional<std::string>("(")),
    Regex_matcher(R"(RPAREN)", SYNTAX, std::make_optional<std::string>(")")),
    Regex_matcher(R"(LBRACK)", SYNTAX, std::make_optional<std::string>("[")),
    Regex_matcher(R"(RBRACK)", SYNTAX, std::make_optional<std::string>("]")),
    Regex_matcher(R"(LBRACE)", SYNTAX, std::make_optional<std::string>("{")),
    Regex_matcher(R"(RBRACE)", SYNTAX, std::make_optional<std::string>("}")),
    Regex_matcher(R"(COMMA)", SYNTAX, std::make_optional<std::string>(",")),
    Regex_matcher(R"(SPACE)", SYNTAX, std::make_optional<std::string>(" ")),
    Regex_matcher(R"(DOT)", SYNTAX, std::make_optional<std::string>(".")),
    Regex_matcher(R"(SINGLE_QUOTE)", SYNTAX, std::make_optional<std::string>("\'")),
    Regex_matcher(R"(DOUBLE_QUOTE)", SYNTAX, std::make_optional<std::string>("\"")),
    Regex_matcher(R"(EQUALS)", SYNTAX, std::make_optional<std::string>("=")),
    Regex_matcher(R"(NEWLINE)", SYNTAX, std::make_optional<std::string>("\n")),
    Regex_matcher(R"(\".*?\"|\'.*?\')", SYNTAX, false),

    Regex_matcher(R"(EXTERNAL(::)?)", EXTERNAL, false),
    Regex_matcher(R"(INTERNAL(::)?)", INTERNAL, false),
    Regex_matcher(R"(OWNED(::)?)", OWNED, false),

    Regex_matcher(R"([a-zA-Z_]+)", RULE, false),

    Regex_matcher(R"(\(\*)", MULTI_COMMENT_START, false),
    Regex_matcher(R"(\*\))", MULTI_COMMENT_END, false),
    Regex_matcher(R"(=|:)", RULE_START, false),
    Regex_matcher(R"(\+=)", RULE_APPEND, false),
    Regex_matcher(R"(\|)", SEPARATOR, false),
    Regex_matcher(R"(;)", RULE_END, false),
    Regex_matcher(R"(\()", LPAREN, false),
    Regex_matcher(R"(\))", RPAREN, false),
    Regex_matcher(R"(\[)", LBRACK, false),
    Regex_matcher(R"(\])", RBRACK, false),
    Regex_matcher(R"(\{)", LBRACE, false),
    Regex_matcher(R"(\})", RBRACE, false),
    Regex_matcher(R"(\*)", ZERO_OR_MORE, false),
    Regex_matcher(R"(\?)", OPTIONAL, false),
    Regex_matcher(R"(\+)", ONE_OR_MORE, false),
    Regex_matcher(R"(\-\>)", ARROW, false),
    Regex_matcher(R"(#)", COMMENT, false),
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

        std::vector<Token> get_tokens(){
            std::vector<Token> tokens = result.get_ok();
            return tokens;
        }

    private:
        Result<std::vector<Token>> result;
        std::string _filename = "bnf.bnf"; 
        bool ignore = false;
        
};

#endif


