#ifndef UINT_H
#define UINT_H

#include <node.h>

class UInt : public Node {

    public:
        using Node::Node;

        UInt() :
            Node(std::to_string(random_uint(10, 1)))
        {}

        UInt(unsigned int i) :
            Node(std::to_string(i))
        {}

        void operator++(int){
            str = std::to_string(get_num() + 1);
        }

        bool operator>(unsigned int i){
            return get_num() > i;
        }

        inline unsigned int get_num() const {
            return safe_stoul(str, 0);
        }

    private:

};


#endif
