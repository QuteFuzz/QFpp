#ifndef PARAMETER_DEF_H
#define PARAMETER_DEF_H

#include <node.h>
#include <name.h>

class Parameter_def : public Node {

    public:
        Parameter_def() :
            Node("parameter_def", PARAMETER_DEF)
        {}

        std::shared_ptr<Name> get_name() const override {
            return std::make_shared<Name>(_name);
        }

    private:
        Name _name;

};

#endif
