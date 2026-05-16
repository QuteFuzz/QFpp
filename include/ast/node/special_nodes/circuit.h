#ifndef CIRCUIT_H
#define CIRCUIT_H

#include <node.h>
#include <resource_def.h>
#include <resource.h>
#include <params.h>
#include <run_utils.h>
#include <clone_mixin.h>

class Circuit : public Cloneable<Circuit> {

    public:

        Circuit() :
            Cloneable<Circuit>("circuit", CIRCUIT),
            name("dummy_circuit")
        {}

        /// @brief Generating a random circuit from scratch
        Circuit(std::string _name, bool _is_subroutine) :
            Cloneable<Circuit>("circuit", CIRCUIT),
            name(_name),
            is_subroutine(_is_subroutine)
        {}

        inline std::string get_name(){return name;}

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
            auto name = def->get_var_name();
            auto scope = def->get_scope();
            auto rk = def->get_resource_kind();
            auto size = def->get_size();

            for(size_t i = 0; i < (size_t)size->get_num(); i++){
                resources.push_back(std::make_shared<Resource>(*name, UInt(i), scope, rk, def->is_reg()));
            }

            resource_defs.push_back(def);
        }

        inline void reset(Resource_kind rk){
            for (const auto& elem : resources){
                if(elem->get_resource_kind() == rk){
                    elem->reset();
                }
            }
        }

        void print_info() const;

    private:
        std::string name;
        bool is_subroutine = false;
        Ptr_coll<Resource> resources;
        Ptr_coll<Resource_def> resource_defs;
};


#endif
