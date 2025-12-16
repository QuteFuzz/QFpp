#ifndef COLLECTION_H
#define COLLECTION_H

class Qubit;
class Bit;

class Qubit_definition;
class Bit_definition;

template<typename T>
concept Allowed_Type = 
    std::is_same_v<T, Qubit> ||
    std::is_same_v<T, Bit> || 
    std::is_same_v<T, Qubit_definition> || 
    std::is_same_v<T, Bit_definition>;

template<Allowed_Type T>
struct Collection {

    public:

        Collection(){}

        void add(const T& elem){
            coll.push_back(std::make_shared<T>(elem));
        }

        std::shared_ptr<T> at(size_t index) {
            if(index < coll.size()){
                return coll.at(index);

            } else {
                return nullptr;

            }
        }

        size_t size() const {
            return coll.size();
        }

        size_t get_num_of(const U8& scope) const {
            size_t sum = 0;

            for(const auto& elem : coll){
                if (elem->get_scope() & scope) sum += 1;
            }

            return sum;
        }

        void reset(){
            if constexpr (std::is_same_v<T, Qubit> || std::is_same_v<T, Bit>){
                for(auto& elem : coll){
                    elem->reset();
                }
            }
        }

        std::vector<std::shared_ptr<T>>::iterator begin(){
            return coll.begin();
        }

        std::vector<std::shared_ptr<T>>::iterator end(){
            return coll.end();
        }

        std::vector<std::shared_ptr<T>>::const_iterator begin() const {
            return coll.begin();
        }

        std::vector<std::shared_ptr<T>>::const_iterator end() const {
            return coll.end();
        }

    private:
        std::vector<std::shared_ptr<T>> coll = {};
};


#endif
