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
    SUBROUTINE_DEFS,
    CIRCUIT,
    BODY,
    PARAM_DEF,
    QUBIT_DEF,
    BIT_DEF,
    REGISTER_QUBIT_DEF,
    REGISTER_BIT_DEF,
    REGISTER_PARAM_DEF,
    SINGULAR_QUBIT_DEF,
    SINGULAR_BIT_DEF,
    SINGULAR_PARAM_DEF,
    QUBIT,
    BIT,
    PARAM,
    REGISTER_QUBIT,
    REGISTER_BIT,
    REGISTER_PARAM,
    SINGULAR_QUBIT,
    SINGULAR_BIT,
    SINGULAR_PARAM,
    FLOAT_LITERAL,
    QUBIT_OP,
    GATE_OP,
    EXPR,
    SUBROUTINE_OP,
    GATE_NAME,
    SUBROUTINE,
    CIRCUIT_ID,
    COMPARE_OP_BITWISE_OR_PAIR,
    COMPOUND_STMT,
    COMPOUND_STMTS,
    RESOURCE_DEF,

    RULE_KINDS_BOTTOM,                            /// ADD NEW RULES ABOVE!

    META_FUNC_TOP,                                /// ADD META FUNCS BELOW!
    CIRCUIT_NAME,
    NUM_QUBITS,
    NUM_BITS,
    NUM_FLOATS,
    INDENT,
    LINE_INDENT,
    INDENT_LEVEL,
    UNIFORM,
    INTEGER,
    FLOAT,
    NAME,
    SIZE,
    INDEX,
    META_FUNC_BOTTOM,                             /// ADD META FUNCS ABOVE!

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
    /*
        special rules
    */
    Token_matcher("subroutine_defs", SUBROUTINE_DEFS),
    Token_matcher("circuit", CIRCUIT),
    Token_matcher("subroutine_circuit", CIRCUIT),
    Token_matcher("body", BODY),
    Token_matcher("subroutine_body", BODY),
    Token_matcher("qubit_def", QUBIT_DEF),
    Token_matcher("bit_def", BIT_DEF),
    Token_matcher("param_def", PARAM_DEF),
    Token_matcher("register_qubit_def", REGISTER_QUBIT_DEF),
    Token_matcher("singular_qubit_def", SINGULAR_QUBIT_DEF),
    Token_matcher("register_bit_def", REGISTER_BIT_DEF),
    Token_matcher("singular_bit_def", SINGULAR_BIT_DEF),
    Token_matcher("register_param_def", REGISTER_PARAM_DEF),
    Token_matcher("singular_param_def", SINGULAR_PARAM_DEF),
    Token_matcher("qubit", QUBIT),
    Token_matcher("bit", BIT),
    Token_matcher("param", PARAM),
    Token_matcher("singular_qubit", SINGULAR_QUBIT),
    Token_matcher("register_qubit", REGISTER_QUBIT),
    Token_matcher("singular_bit", SINGULAR_BIT),
    Token_matcher("register_bit", REGISTER_BIT),
    Token_matcher("singular_param", SINGULAR_PARAM),
    Token_matcher("register_param", REGISTER_PARAM),
    Token_matcher("float_literal", FLOAT_LITERAL),
    Token_matcher("qubit_op", QUBIT_OP),
    Token_matcher("gate_op", GATE_OP),
    Token_matcher("subroutine_op", SUBROUTINE_OP),
    Token_matcher("gate_name", GATE_NAME),
    Token_matcher("singular_qubit", SINGULAR_QUBIT),
    Token_matcher("register_qubit", REGISTER_QUBIT),
    Token_matcher("singular_bit", SINGULAR_BIT),
    Token_matcher("register_bit", REGISTER_BIT),
    Token_matcher("subroutine", SUBROUTINE),
    Token_matcher("circuit_id", CIRCUIT_ID),
    Token_matcher("compare_op_bitwise_or_pair", COMPARE_OP_BITWISE_OR_PAIR),
    Token_matcher("compound_stmt", COMPOUND_STMT),
    Token_matcher("compound_stmts", COMPOUND_STMTS),
    Token_matcher("subroutine_compound_stmts", COMPOUND_STMTS),
    Token_matcher("classical_expr", EXPR),
    Token_matcher("bool_expr", EXPR),
    Token_matcher("uint_expr", EXPR),
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

    /*
        scopes
    */
    Token_matcher("EXTERNAL", EXTERNAL),
    Token_matcher("INTERNAL", INTERNAL),

    /**
        meta functions
    */
    Token_matcher("FLOAT", FLOAT),
    Token_matcher("INTEGER", INTEGER),
    Token_matcher("INDENT", INDENT),
    Token_matcher("LINE_INDENT", LINE_INDENT),
    Token_matcher("INDENT_LEVEL", INDENT_LEVEL),
    Token_matcher("UNIFORM", UNIFORM),
    Token_matcher("NAME", NAME),
    Token_matcher("INDEX", INDEX),
    Token_matcher("SIZE", SIZE),
    Token_matcher("CIRCUIT_NAME", CIRCUIT_NAME),
    Token_matcher("NUM_QUBITS", NUM_QUBITS),
    Token_matcher("NUM_BITS", NUM_BITS),
    Token_matcher("NUM_FLOATS", NUM_FLOATS),
    // meta functions ish, that get immediately converted into syntax because we know before hand what the replacement should be
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

    /*
        multi char tokens
    */
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
    R"([a-zA-Z_][a-zA-Z0-9_]*|[0-9]+(\.[0-9]+)?|#[^\n]*|\(\*|\*\)|\".*?\"|\'.*?\'|->|::|\+=|>=|<=|.)";


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

        void lex(){
            std::string input, matched_string;
            std::vector<Token> tokens;
            std::ifstream stream(_filename);

            std::regex full_pattern(FULL_REGEX, std::regex::icase);

            std::sregex_iterator end;

            bool in_multiline_comment = false;

            while(std::getline(stream, input)){

                std::sregex_iterator begin(input.begin(), input.end(), full_pattern);
                std::sregex_iterator end;

                for(std::sregex_iterator i = begin; i != end; ++i){
                    std::string text = i->str();

                    /*
                        ignore comments
                    */

                    if (text == "(*") {
                        in_multiline_comment = true;
                        continue; // Skip this token
                    }

                    if (text == "*)") {
                        in_multiline_comment = false;
                        continue; // Skip this token
                    }

                    if (in_multiline_comment) {
                        continue;
                    }

                    if (text[0] == '#') {
                        continue; // Skip the whole comment string
                    }

                    if (isspace(text[0])) {
                        continue;
                    }

                    // find exact qf tokens
                    bool found_keyword = false;

                    for(const Token_matcher& tm : TOKEN_RULES) {
                        if (text == tm.pattern) {
                            tokens.push_back(Token{tm.replacement.value_or(text), tm.kind});
                            found_keyword = true;
                            break;
                        }
                    }

                    if (found_keyword) continue;


                    // classify generics
                    if (isalpha(text[0]) || text[0] == '_') {
                        tokens.push_back(Token{text, RULE});

                    } else if (isdigit(text[0])) {
                        tokens.push_back(Token{text, SYNTAX});

                    } else {
                        tokens.push_back(Token{remove_outer_quotes(text), SYNTAX});
                    }
                }
            }

            tokens.push_back(Token{.value = "", .kind = _EOF});
            result.set_ok(tokens);
        }

        void print_tokens() const {

            if(result.is_error()){
                ERROR(result.get_error());

            } else {
                std::vector<Token> tokens = result.get_ok();

                for(size_t i = 0; i < tokens.size(); ++i){
                    std::cout << tokens[i] << std::endl;
                }
            }
        }


        std::vector<Token> get_tokens(){
            std::vector<Token> tokens = result.get_ok();
            return tokens;
        }

    private:
        Result<std::vector<Token>> result;
        std::string _filename = "bnf.bnf";
        bool ignore = false;

};

inline bool is_kind_of_rule(const Token_kind& kind){
    return
        (RULE_KINDS_TOP < kind) &&
        (RULE_KINDS_BOTTOM > kind)
        ;
}

inline bool is_meta(const Token_kind& kind){
    return
        (META_FUNC_TOP < kind) &&
        (META_FUNC_BOTTOM > kind);
}

inline bool is_quiet(const Token_kind& kind){
    return
        (kind == SCOPE_RES) ||
        (kind == ARROW) ||
        (kind == LINE_INDENT) ||
        (kind == INDENT);
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
