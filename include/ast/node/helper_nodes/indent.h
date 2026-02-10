#ifndef INDENT_H
#define INDENT_H

#include <node.h>

class Indent : public Node {

    public:
        Indent():
            Node("indent", INDENT)
        {}

        Indent(const std::string& str, const Token_kind& kind):
            Node(str, kind)
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
