#ifndef FLOAT_H
#define FLOAT_H

#include <node.h>
#include <clone_mixin.h>

class Float : public Cloneable<Float> {

    public:

        Float() :
            Cloneable<Float>(std::to_string(random_float(10.0, 0.0)))
        {
            rng().seed(random_uint());
        }

        Float(float n) :
            Cloneable<Float>(std::to_string(n)),
            num(n)
        {}

        float get_num() const {return num;}

    private:
        float num;
};


#endif
