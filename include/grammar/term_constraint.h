#ifndef TERM_CONSTRAINT_H
#define TERM_CONSTRAINT_H

#include <lex.h>

struct Context;

enum class Term_constraint_kind {
    NONE,
    RANDOM_MAX,
    DYNAMIC_MAX
};

struct Term_constraint {

    public:
        Term_constraint(){}

        Term_constraint(Token_kind _dependency, int _offset) :
            kind(Term_constraint_kind::DYNAMIC_MAX),
            rand_min(0),
            rand_max(0),     
            dependency(_dependency),
            offset(_offset)
        {}

        Term_constraint(Term_constraint_kind _kind, unsigned int _rand_min, unsigned int _rand_max) :
            kind(_kind),
            rand_min(_rand_min),
            rand_max(_rand_max)
        {}

        unsigned int resolve(std::function<unsigned int(Token_kind)> lookup){
            if(kind == Term_constraint_kind::NONE){
                return 1;
            
            } else if(kind == Term_constraint_kind::RANDOM_MAX){
                return random_uint(rand_max, rand_min);

            } else if(kind == Term_constraint_kind::DYNAMIC_MAX){
                int val = lookup(dependency) + offset;

                if(val < 0){
                    val = 0;
                }
                return (unsigned int)val;
            
            } else {
                throw std::runtime_error("Given term constraint kind not supported");
            }
        }

        friend std::ostream& operator<<(std::ostream& stream, const Term_constraint& constraint) {
            
            if (constraint.kind == Term_constraint_kind::NONE){
            
            } else if(constraint.kind == Term_constraint_kind::RANDOM_MAX){
                stream << " [0 " << "- RANDOM(" << constraint.rand_min << "," << constraint.rand_max << ")]";

            } else if(constraint.kind == Term_constraint_kind::DYNAMIC_MAX){
                stream << " [0 - DYNAMIC]";
            
            } else {
                throw std::runtime_error("Given term constraint kind not supported");
            }

            return stream;
        }

    private:
        Term_constraint_kind kind = Term_constraint_kind::NONE;
        unsigned int rand_min;
        unsigned int rand_max;

        Token_kind dependency;
        int offset;
};


#endif
