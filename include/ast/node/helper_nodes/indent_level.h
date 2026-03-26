#ifndef INDENT_LEVEL_H
#define INDENT_LEVEL_H

#include <node.h>
#include <clone_mixin.h>

class Indent_level : public Cloneable<Indent_level> {

    public:
        Indent_level():
            Cloneable<Indent_level>("indent_level", INDENT_LEVEL)
        {
            print_mode = Print_mode::INDENT_LEVEL;
        }

    private:
};

#endif
