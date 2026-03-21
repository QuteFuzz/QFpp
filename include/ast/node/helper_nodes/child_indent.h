#ifndef CHILD_INDENT_H
#define CHILD_INDENT_H

#include <node.h>
#include <clone_mixin.h>

class Child_indent : public Cloneable<Child_indent> {

    public:
        Child_indent():
            Cloneable<Child_indent>("indent", CHILD_INDENT)
        {
            print_mode = Print_mode::CHILD_INDENT;
        }

        Child_indent(const std::string& str, const Token_kind& kind):
            Cloneable<Child_indent>(str, kind)
        {
            print_mode = Print_mode::CHILD_INDENT;
        }

    private:

};

#endif
