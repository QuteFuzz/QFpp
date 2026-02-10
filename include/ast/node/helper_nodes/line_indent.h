#ifndef LINE_INDENT_H
#define LINE_INDENT_H

#include <node.h>

class Line_indent : public Node {

    public:
        Line_indent():
            Node("line_indent", LINE_INDENT)
        {}

        Line_indent(const std::string& str, const Token_kind& kind):
            Node(str, kind)
        {}

        void print_program(std::ostream& stream, unsigned int indent_level) const override {
            unsigned int inner_level = indent_level;
            std::string tabs(inner_level, '\t');

            stream << tabs;

            for(const std::shared_ptr<Node>& child : children){
                child->print_program(stream, inner_level);
            }
        }

    private:

};

#endif
