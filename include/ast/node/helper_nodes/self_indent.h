#ifndef SELF_INDENT_H
#define SELF_INDENT_H

#include <node.h>
#include <clone_mixin.h>

class Self_indent : public Cloneable<Self_indent> {

    public:
        Self_indent():
            Cloneable<Self_indent>("self_indent", SELF_INDENT)
        {
            print_mode = Print_mode::SELF_INDENT;
        }

        Self_indent(const std::string& str, const Token_kind& kind):
            Cloneable<Self_indent>(str, kind)
        {
            print_mode = Print_mode::SELF_INDENT;
        }
    
    private:
};

#endif
