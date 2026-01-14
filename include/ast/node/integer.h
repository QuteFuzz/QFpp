#ifndef INTEGER_H
#define INTEGER_H

#include <node.h>

class Integer : public Node {

    public:
        using Node::Node;

        Integer() : 
            Node(std::to_string(42))
        {}

        Integer(int i) : 
            Node(std::to_string(i)),
            num(i)
        {}

        void operator++(int){
            num += 1;
            content = std::to_string(num);
        }

        bool operator==(const Integer& other) const {
            return num == other.get_num();
        }

        inline int get_num() const {
            return num;
        }

    private:
        int num;

};


#endif
