#ifndef INDENT_LEVEL_H
#define INDENT_LEVEL_H

#include <node.h>

class Indent_level : public Node {

    public:
        Indent_level():
            Node("indent_level", INDENT_LEVEL)
        {}

        void print_program(std::ostream& stream, unsigned int indent_level) const override {
            stream << indent_level;
        }

    private:
};

#endif
