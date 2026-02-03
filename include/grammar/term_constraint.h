#ifndef TERM_CONSTRAINT_H
#define TERM_CONSTRAINT_H

#include <lex.h>

struct Term_constraint {

    std::variant<std::monostate, Token_kind, unsigned int> value;

    Term_constraint(){}

    Term_constraint(Token_kind token_kind) :
        value(token_kind)
    {}

    Term_constraint(unsigned int count) :
        value(count)
    {}

    bool is_empty(){
        return std::holds_alternative<std::monostate>(value);
    }

    friend std::ostream& operator<<(std::ostream& stream, Term_constraint constraint){
        if(!constraint.is_empty()){
            if(std::holds_alternative<unsigned int>(constraint.value)){
                stream << "[" << std::get<unsigned int>(constraint.value) << "]";
            } else if(std::holds_alternative<Token_kind>(constraint.value)){
                stream << "[COUNT<" << std::get<Token_kind>(constraint.value) << ">]";
            }
        }

        return stream;
    }

};


#endif
