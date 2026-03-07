#ifndef INDENT_LEVEL_H
#define INDENT_LEVEL_H

#include <node.h>
#include <clone_mixin.h>

class Indent_level : public Cloneable<Indent_level> {

    public:
        Indent_level():
            Cloneable<Indent_level>("indent_level", INDENT_LEVEL)
        {}

        void print_program(std::ostream& stream, unsigned int indent_level) const override {
            stream << indent_level;
        }

    private:
};

#endif
