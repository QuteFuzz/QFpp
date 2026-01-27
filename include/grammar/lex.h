#ifndef LEX_H
#define LEX_H

#include <regex>
#include <stdio.h>
#include "string.h"
#include <stdlib.h>
#include <fstream>

#include <result.h>
#include <params.h>

enum Token_kind {
    _EOF = 0,

    RULE_KINDS_TOP,                  /// ADD NEW RULES BELOW!
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
    CRX,
    CRY,
    CRZ,
    U3,
    CSWAP,
    SWAP,
    TOFFOLI,
    U,
    PHASED_X,
    BARRIER,
    SUBROUTINE_DEFS,
    CIRCUIT,
    BODY,
    QUBIT_DEFS,
    BIT_DEFS,
    QUBIT_DEF,
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
    PARAMETER_DEF_NAME,
    PARAMETER_DEF,
    QUBIT_DEF_NAME,
    BIT_DEF_NAME,
    QUBIT,
    BIT,
    QUBIT_OP,
    GATE_OP,
    SUBROUTINE_OP,
    GATE_NAME,
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
    SUBROUTINE_OP_ARGS,
    GATE_OP_ARGS,
    SUBROUTINE_OP_ARG,
    COMPOUND_STMT,
    COMPOUND_STMTS,
    INDENTATION_DEPTH,
    INTEGER,
    FLOAT,
    /*
        these aren't used in the lexer, but in the AST node creation. maybe will be used in the lexer if these rules are needed later
    */
    REGISTER_RESOURCE,
    REGISTER_RESOURCE_DEF,
    SINGULAR_RESOURCE,
    SINGULAR_RESOURCE_DEF,
    RESOURCE_DEF,
    RULE_KINDS_BOTTOM,                            /// ADD NEW RULES ABOVE!

    GRAMMAR_SYNTAX_TOP,                           /// ADD GRAMMAR SYNTAX BELOW!
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
    LANGLE_BRACKET,
    RANGLE_BRACKET,
    ZERO_OR_MORE,
    ONE_OR_MORE,
    OPTIONAL,
    ARROW,
    INTERNAL,
    EXTERNAL,
    OWNED,
    GLOBAL,
    SCOPE_RES,
    /*
        Grammar syntax end, add new syntax above
    */
    GRAMMAR_SYNTAX_BOTTOM,                      /// ADD GRAMMAR SYNTAX ABOVE!
};

inline std::string kind_as_str(const Token_kind& kind);

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

struct Token_matcher {

    Token_matcher(const std::string& p, const Token_kind& k){
        pattern = p;
        kind = k;
    }

    Token_matcher(const std::string& p, const Token_kind& k, std::optional<std::string> r){
        pattern = p;
        kind = k;
        replacement = r;
    }

    std::string pattern;
    Token_kind kind;
    std::optional<std::string> replacement = std::nullopt;
};

const std::vector<Token_matcher> TOKEN_RULES = {

    Token_matcher("subroutine_defs", SUBROUTINE_DEFS),
    Token_matcher("circuit", CIRCUIT),
    Token_matcher("subroutine_circuit", CIRCUIT),
    Token_matcher("body", BODY),
    Token_matcher("subroutine_body", BODY),
    Token_matcher("qubit_defs", QUBIT_DEFS),
    Token_matcher("bit_defs", BIT_DEFS),
    Token_matcher("qubit_def", QUBIT_DEF),
    Token_matcher("bit_def", BIT_DEF),
    Token_matcher("register_qubit_def", REGISTER_QUBIT_DEF),
    Token_matcher("singular_qubit_def", SINGULAR_QUBIT_DEF),
    Token_matcher("register_qubit_def_discard", REGISTER_QUBIT_DEF_DISCARD),
    Token_matcher("singular_qubit_def_discard", SINGULAR_QUBIT_DEF_DISCARD),
    Token_matcher("register_bit_def", REGISTER_BIT_DEF),
    Token_matcher("singular_bit_def", SINGULAR_BIT_DEF),
    Token_matcher("circuit_name", CIRCUIT_NAME),
    Token_matcher("float_list", FLOAT_LIST),
    Token_matcher("float_literal", FLOAT_LITERAL),
    Token_matcher("qubit_def_name", QUBIT_DEF_NAME),
    Token_matcher("bit_def_name", BIT_DEF_NAME),
    Token_matcher("qubit", QUBIT),
    Token_matcher("bit", BIT),
    Token_matcher("qubit_op", QUBIT_OP),
    Token_matcher("gate_op", GATE_OP),
    Token_matcher("subroutine_op", SUBROUTINE_OP),
    Token_matcher("gate_name", GATE_NAME),
    Token_matcher("qubit_list", QUBIT_LIST),
    Token_matcher("bit_list", BIT_LIST),
    Token_matcher("qubit_def_list", QUBIT_DEF_LIST),
    Token_matcher("qubit_def_size", QUBIT_DEF_SIZE),
    Token_matcher("bit_def_list", BIT_DEF_LIST),
    Token_matcher("bit_def_size", BIT_DEF_SIZE),
    Token_matcher("singular_qubit", SINGULAR_QUBIT),
    Token_matcher("register_qubit", REGISTER_QUBIT),
    Token_matcher("singular_bit", SINGULAR_BIT),
    Token_matcher("register_bit", REGISTER_BIT),
    Token_matcher("qubit_name", QUBIT_NAME),
    Token_matcher("bit_name", BIT_NAME),
    Token_matcher("qubit_index", QUBIT_INDEX),
    Token_matcher("bit_index", BIT_INDEX),
    Token_matcher("subroutine", SUBROUTINE),
    Token_matcher("circuit_id", CIRCUIT_ID),
    Token_matcher("if_stmt", IF_STMT),
    Token_matcher("else_stmt", ELSE_STMT),
    Token_matcher("elif_stmt", ELIF_STMT),
    Token_matcher("disjunction", DISJUNCTION),
    Token_matcher("conjunction", CONJUNCTION),
    Token_matcher("inversion", INVERSION),
    Token_matcher("expression", EXPRESSION),
    Token_matcher("compare_op_bitwise_or_pair", COMPARE_OP_BITWISE_OR_PAIR),
    Token_matcher("subroutine_op_args", SUBROUTINE_OP_ARGS),
    Token_matcher("gate_op_args", GATE_OP_ARGS),
    Token_matcher("subroutine_op_arg", SUBROUTINE_OP_ARG),
    Token_matcher("compound_stmt", COMPOUND_STMT),
    Token_matcher("compound_stmts", COMPOUND_STMTS),
    Token_matcher("indentation_depth", INDENTATION_DEPTH),
    Token_matcher("parameter_def_name", PARAMETER_DEF_NAME),
    Token_matcher("parameter_def", PARAMETER_DEF),

    Token_matcher("h", H),
    Token_matcher("x", X),
    Token_matcher("y", Y),
    Token_matcher("z", Z),
    Token_matcher("rz", RZ),
    Token_matcher("rx", RX),
    Token_matcher("ry", RY),
    Token_matcher("u1", U1),
    Token_matcher("s", S),
    Token_matcher("sdg", SDG),
    Token_matcher("t", T),
    Token_matcher("tdg", TDG),
    Token_matcher("v", V),
    Token_matcher("vdg", VDG),
    Token_matcher("phased_x", PHASED_X),
    Token_matcher("project_z", PROJECT_Z),
    Token_matcher("measure_and_reset", MEASURE_AND_RESET),
    Token_matcher("measure", MEASURE),
    Token_matcher("cx", CX),
    Token_matcher("cy", CY),
    Token_matcher("cz", CZ),
    Token_matcher("ccx", CCX),
    Token_matcher("u2", U2),
    Token_matcher("cnot", CNOT),
    Token_matcher("ch", CH),
    Token_matcher("crz", CRZ),
    Token_matcher("crx", CRX),
    Token_matcher("cry", CRY),
    Token_matcher("u3", U3),
    Token_matcher("cswap", CSWAP),
    Token_matcher("swap", SWAP),
    Token_matcher("toffoli", TOFFOLI),
    Token_matcher("u", U),
    Token_matcher("barrier", BARRIER),
    Token_matcher("FLOAT", FLOAT),
    Token_matcher("INTEGER", INTEGER),

    Token_matcher("LPAREN", SYNTAX, "("),
    Token_matcher("RPAREN", SYNTAX, ")"),
    Token_matcher("LBRACK", SYNTAX, "["),
    Token_matcher("RBRACK", SYNTAX, "]"),
    Token_matcher("LBRACE", SYNTAX, "{"),
    Token_matcher("RBRACE", SYNTAX, "}"),
    Token_matcher("COMMA", SYNTAX, ","),
    Token_matcher("SPACE", SYNTAX, " "),
    Token_matcher("DOT", SYNTAX, "."),
    Token_matcher("SINGLE_QUOTE", SYNTAX, "\'"),
    Token_matcher("DOUBLE_QUOTE", SYNTAX, "\""),
    Token_matcher("EQUALS", SYNTAX, "="),
    Token_matcher("NEWLINE", SYNTAX, "\n"),
    Token_matcher("INDENT", INDENT),
    Token_matcher("DEDENT", DEDENT),
    Token_matcher("EXTERNAL", EXTERNAL),
    Token_matcher("INTERNAL", INTERNAL),
    Token_matcher("OWNED", OWNED),
    Token_matcher("GLOBAL", GLOBAL),

    Token_matcher("::", SCOPE_RES),
    Token_matcher("->", ARROW),
    Token_matcher("+=", RULE_APPEND),

    /*
        single character tokens
    */
    Token_matcher("=", RULE_START),
    Token_matcher(":", RULE_START),
    Token_matcher("|", SEPARATOR),
    Token_matcher(";", RULE_END),
    Token_matcher("(", LPAREN),
    Token_matcher(")", RPAREN),
    Token_matcher("[", LBRACK),
    Token_matcher("]", RBRACK),
    Token_matcher("{", LBRACE),
    Token_matcher("}", RBRACE),
    Token_matcher("*", ZERO_OR_MORE),
    Token_matcher("?", OPTIONAL),
    Token_matcher("+", ONE_OR_MORE),
    Token_matcher("<", LANGLE_BRACKET),
    Token_matcher(">", RANGLE_BRACKET),
};

const std::string FULL_REGEX =
    R"([a-zA-Z_][a-zA-Z0-9_]*|[0-9]+(\.[0-9]+)?|#[^\n]*|\(\*|\*\)|\".*?\"|\'.*?\'|->|::|\+=|.)";


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
        (kind == SCOPE_RES) ||
        (kind == LBRACE) ||
        (kind == ARROW);
}

inline std::string kind_as_str(const Token_kind& kind) {
    for (auto tm : TOKEN_RULES){
        if(tm.kind == kind){
            return tm.pattern;
        }
    }

    return "";
}

#endif
