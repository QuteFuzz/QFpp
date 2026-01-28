#ifndef RUN_H
#define RUN_H

#include <sstream>
#include <set>
#include <iomanip>

#include <generator.h>
#include <run_utils.h>

class Run{

    static const fs::path OUTPUT_DIR;

    public:
        Run(const std::string& _grammars_dir);

        inline bool is_grammar(const std::string& name){
            return generators.find(name) != generators.end();
        }

        void help();

        void set_grammar(Control& control);

        void tokenise(const std::string& command, const char& delim);

        void remove_all_in_dir(const fs::path& dir);

        void loop();

        inline void setup_output_dir(const std::string& grammar_name) {
            current_output_dir = OUTPUT_DIR / grammar_name;

            if(fs::exists(current_output_dir)){
                remove_all_in_dir(current_output_dir);
            } else {
                fs::create_directory(current_output_dir);
            }
        }

    private:
        fs::path grammars_dir;
        fs::path current_output_dir;

        std::unordered_map<std::string, std::shared_ptr<Generator>> generators;
        std::shared_ptr<Generator> current_generator = nullptr;
        std::vector<std::string> tokens;
        unsigned int n_programs;
};

#endif
