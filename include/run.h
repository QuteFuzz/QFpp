#ifndef RUN_H
#define RUN_H

#include <sstream>
#include <set>
#include <iomanip>

#include <generator.h>

class Run{

    public:
        Run(const std::string& _grammars_dir);

        inline bool is_grammar(const std::string& name){
            return generators.find(name) != generators.end();
        }

        void help();

        void set_grammar();

        void tokenise(const std::string& command, const char& delim);

        void remove_all_in_dir(const fs::path& dir);

        void loop();

    private:
        fs::path grammars_dir;
        std::unordered_map<std::string, std::shared_ptr<Generator>> generators;
        std::shared_ptr<Generator> current_generator = nullptr;

        std::vector<std::string> tokens;

        std::optional<unsigned int> n_programs;

        fs::path output_dir;
};

#endif

