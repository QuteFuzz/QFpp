#ifndef UINT_H
#define UINT_H

#include <node.h>

class UInt : public Node {

    public:
        using Node::Node;

        UInt() :
            Node(std::to_string(random_uint(10))),
            num(safe_stoul(content, 0))
        {}

        UInt(unsigned int i) :
            Node(std::to_string(i)),
            num(i)
        {}

        void operator++(int){
            num += 1;
            content = std::to_string(num);
        }

        inline int get_num() const {
            return num;
        }

    private:
        unsigned int num = 0;

};


#endif
