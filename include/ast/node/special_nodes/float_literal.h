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

        /// @brief Random float within some range. Uses its own generator to seed based on node counter
        /// @param max value inclusive
        /// @param min value inclusive
        /// @return
        float random_float(float max, float min){
            if(min < max){
                static std::mt19937 random_gen;
                random_gen.seed(node_counter);
                std::uniform_real_distribution<float> float_dist(min, max);
                return float_dist(random_gen);

            } else {
                return min;
            }
        }

};


#endif
