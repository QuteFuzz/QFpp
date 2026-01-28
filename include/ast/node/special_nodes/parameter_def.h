#ifndef PARAMETER_DEF_H
#define PARAMETER_DEF_H

#include <node.h>
#include <variable.h>

class Parameter_def : public Node {

    public:
        Parameter_def() :
            Node("parameter_def", PARAMETER_DEF)
        {}

        std::shared_ptr<Variable> get_name(){
            return std::make_shared<Variable>(_name);
        }

    private:
        Variable _name;

};

#endif
