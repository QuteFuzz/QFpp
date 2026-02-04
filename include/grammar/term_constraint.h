#ifndef TERM_CONSTRAINT_H
#define TERM_CONSTRAINT_H

#include <lex.h>

struct Range {
    unsigned int min;
    unsigned int max;
};

enum class Term_constraint_kind {
    NONE,
    RANGE_FIXED_MAX,
    RANGE_RAND_MAX
};

struct Term_constraint {

    public:
        Term_constraint(){}

        Term_constraint(Term_constraint_kind _kind, unsigned int _min, unsigned int _max) :
            kind(_kind),
            min(_min),
            max(_max)
        {}

        Range resolve(){
            if(kind == Term_constraint_kind::NONE){
                return {0, 1};
            
            } else if(kind == Term_constraint_kind::RANGE_FIXED_MAX){
                return {min, max};

            } else if(kind == Term_constraint_kind::RANGE_RAND_MAX){
                return {min, random_uint(max, min)};

            } else {
                throw std::runtime_error("Given term constraint kind not supported");
            }
        }

        friend std::ostream& operator<<(std::ostream& stream, Term_constraint constraint){
            if(constraint.kind == Term_constraint_kind::RANGE_FIXED_MAX){
                stream << "[" << constraint.min << "-" << constraint.max << "]";

            } else if(constraint.kind == Term_constraint_kind::RANGE_RAND_MAX){
                stream << "[" << constraint.min << "- RANDOM(" << constraint.min << "," << constraint.max << ")";

            } else {
                throw std::runtime_error("Given term constraint kind not supported");
            }

            return stream;
        }

    private:
        Term_constraint_kind kind = Term_constraint_kind::NONE;
        unsigned int min;
        unsigned int max;

};


#endif
