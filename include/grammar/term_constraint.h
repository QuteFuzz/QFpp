#ifndef TERM_CONSTRAINT_H
#define TERM_CONSTRAINT_H

#include <lex.h>

struct Context;

struct Range {
    unsigned int min;
    unsigned int max;
};

enum class Term_constraint_kind {
    NONE,
    FIXED_MAX,
    RANDOM_MAX,
    DYNAMIC_MAX
};

struct Term_constraint {

    public:
        Term_constraint(){}

        Term_constraint(Token_kind _dependency, int _offset) :
            kind(Term_constraint_kind::DYNAMIC_MAX),
            min(1),
            max(0),     // unknown
            dependency(_dependency),
            offset(_offset)
        {}

        Term_constraint(Term_constraint_kind _kind, unsigned int _min, unsigned int _max) :
            kind(_kind),
            min(_min),
            max(_max)
        {}

        Range resolve(std::function<unsigned int(Token_kind)> lookup){
            if(kind == Term_constraint_kind::NONE){
                return {0, 1};
            
            } else if(kind == Term_constraint_kind::FIXED_MAX){
                return {min, max};

            } else if(kind == Term_constraint_kind::RANDOM_MAX){
                return {min, random_uint(max, min)};

            } else if(kind == Term_constraint_kind::DYNAMIC_MAX){
                int val = lookup(dependency) + offset;

                if(val < 0){
                    val = 0;
                }
                return {min, (unsigned int)val};
            
            } else {
                throw std::runtime_error("Given term constraint kind not supported");
            }
        }

        friend std::ostream& operator<<(std::ostream& stream, Term_constraint constraint){
            
            if (constraint.kind == Term_constraint_kind::NONE){
            
            } else if(constraint.kind == Term_constraint_kind::FIXED_MAX){
                stream << "[" << constraint.min << "-" << constraint.max << "]";

            } else if(constraint.kind == Term_constraint_kind::RANDOM_MAX){
                stream << "[" << constraint.min << "- RANDOM(" << constraint.min << "," << constraint.max << ")";

            } else if(constraint.kind == Term_constraint_kind::DYNAMIC_MAX){
                stream << "[" << constraint.min << "- DYNAMIC]";
            
            } else {
                throw std::runtime_error("Given term constraint kind not supported");
            }

            return stream;
        }

    private:
        Term_constraint_kind kind = Term_constraint_kind::NONE;
        unsigned int min;
        unsigned int max;

        Token_kind dependency;
        int offset;
};


#endif
