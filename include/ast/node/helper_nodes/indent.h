#ifndef INDENT_H
#define INDENT_H

#include <node.h>
#include <clone_mixin.h>

class Indent : public Cloneable<Indent> {

    public:
        Indent():
            Cloneable<Indent>("indent", INDENT)
        {}

        Indent(const std::string& str, const Token_kind& kind):
            Cloneable<Indent>(str, kind)
        {}

        void print_program(std::ostream& stream, unsigned int indent_level) const override {
            unsigned int inner_level = indent_level + 1;
            std::string tabs(inner_level, '\t');

            for(const std::shared_ptr<Node>& child : children){
                stream << tabs;
                child->print_program(stream, inner_level);
            }
        }

    private:

};

#endif
