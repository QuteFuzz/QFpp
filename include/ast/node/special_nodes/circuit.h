#ifndef CIRCUIT_H
#define CIRCUIT_H

#include <node.h>
#include <resource_def.h>
#include <resource.h>
#include <params.h>
#include <run_utils.h>

class Circuit : public Node {

    public:

        Circuit() :
            Node("circuit", CIRCUIT),
            owner("dummy_circuit")
        {}

        /// @brief Generating a random circuit from scratch
        Circuit(std::string owner_name, bool _is_subroutine) :
            Node("circuit", CIRCUIT),
            owner(owner_name),
            is_subroutine(_is_subroutine)
        {}

        inline bool owned_by(std::string other){return other == owner;}

        inline std::string get_owner(){return owner;}

        inline bool check_if_subroutine(){return is_subroutine;}

        template <typename T>
        inline Ptr_coll<T> get_coll() const {
            if constexpr (std::is_same_v<T, Resource>) {
                return resources;

            } else if constexpr (std::is_same_v<T, Resource_def>) {
                return resource_defs;

            } else {
                throw std::runtime_error("Unknown collection type in circuit");
            }
        }

        template <typename T>
        inline Ptr_coll<T> get_coll(Resource_kind rk) const {
            auto pred = [rk](const auto& elem){ return elem->get_resource_kind() == rk; };
            return filter<T>(get_coll<T>(), pred);
        }

        void store_resource_def(std::shared_ptr<Resource_def> def){
            auto name = def->get_name();
            auto scope = def->get_scope();
            auto rk = def->get_resource_kind();
            auto size = def->get_size();

            for(size_t i = 0; i < (size_t)size->get_num(); i++){
                // Register_resource reg_resource(*name, UInt(std::to_string(i)));
                resources.push_back(std::make_shared<Resource>(*name, UInt(std::to_string(i)), scope, rk, def->is_reg()));
            }

            resource_defs.push_back(def);
        }

        inline void reset(){
            for (const auto& elem : resources){
                elem->reset();
            }
        }

        void print_info() const;

    private:
        std::string owner;
        bool is_subroutine = false;
        Ptr_coll<Resource> resources;
        Ptr_coll<Resource_def> resource_defs;
};


#endif
