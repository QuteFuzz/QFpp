#include <utils.h>
#include <sstream>
#include <params.h>

std::ofstream get_stream(fs::path output_dir, std::string file_name){
    fs::create_directory(output_dir);
    fs::path path = output_dir / file_name;
    std::ofstream stream(path.string());

    INFO("Writing to " + path.string());

    return stream;
}

void lower(std::string& str){
    std::transform(str.begin(), str.end(), str.begin(),
        [](char c){return std::tolower(c);}
    );
}

std::mt19937& rng(){
    static std::mt19937 random_gen;
    return random_gen;
}

/// @brief Random unsigned integer within some range
/// @param max value inclusive
/// @param min value inclusive
/// @return
unsigned int random_uint(unsigned int max, unsigned int min){
    if(min < max){
        std::uniform_int_distribution<unsigned int> uint_dist(min, max);
        return uint_dist(rng());

    } else {
        return min;
    }
}


unsigned int safe_stoul(const std::string& str, unsigned int default_value) {
    try {
        unsigned int ret = std::stoul(str);
        return ret;
    } catch (const std::invalid_argument& e) {
        return default_value;
    }
}

/// @brief Find all possible combinations that can be chosen from numbers in [0, n-1]
/// Knew the solution had something to do with counting in binary, but I didn't come up with this algorithm myself
/// https://stackoverflow.com/questions/12991758/creating-all-possible-k-combinations-of-n-items-in-c
/// @param n
/// @param r
/// @return
std::vector<std::vector<int>> n_choose_r(const int n, const int r){
    std::vector<std::vector<int>> res;

    if(n >= r){
        std::string bitmask(r, 1);
        bitmask.resize(n, 0);

        do{
            std::vector<int> comb;

            for(int i = 0; i < n; i++){
                if (bitmask[i]) comb.push_back(i);
            }

            res.push_back(comb);

        } while(std::prev_permutation(bitmask.begin(), bitmask.end()));
    }

    return res;
}

int vector_sum(std::vector<int> in){
    int res = 0;

    for(size_t i = 0; i < in.size(); i++){
        res += in[i];
    }

    return res;
}

int vector_max(std::vector<int> in){
    int max = 0;

    for(size_t i = 0; i < in.size(); i++){
        max = std::max(max, in[i]);
    }

    return max;
}


void pipe_to_command(std::string command, std::string write){
    FILE* pipe = popen(command.c_str(), "w");

    if(!pipe){
        throw std::runtime_error(ANNOT("Failed to open pipe to command " + command));
    }

    fwrite(write.c_str(), sizeof(char), write.size(), pipe);

    if(pclose(pipe)){
        throw std::runtime_error(ANNOT("Command " + command + " failed"));
    }
}

std::string pipe_from_command(std::string command){
    FILE* pipe = popen(command.c_str(), "r");

    if(!pipe){
        ERROR("Failed to open pipe to command " + command);
    }

    std::array<char, 1024> buffer;
    std::string result = "";

    while(fgets(buffer.data(), buffer.size(), pipe) != nullptr){
        result += buffer.data();
    }

    if(pclose(pipe)){
        ERROR("Command " + command + " failed");
    }

    INFO("Run command " + command);

    return result;
}

std::string escape(const std::string& str){
    std::ostringstream oss;
    oss << std::quoted(str);
    return oss.str();
}

std::string random_hex_colour(){

    std::uniform_int_distribution<int> int_dist(0, 255);

    std::ostringstream ss;

    ss << "#"
    << std::hex << std::setfill('0')
    << std::setw(2) << int_dist(rng())
    << std::setw(2) << int_dist(rng())
    << std::setw(2) << int_dist(rng());

    return ss.str();
}

std::string escape_string(const std::string& input) {
    std::string output;
    for (char c : input) {
        switch (c) {
            case '\n': output += "\\n"; break;
            case '\t': output += "\\t"; break;
            case '\r': output += "\\r"; break;
            case '\\': output += "\\\\"; break;
        }
    }
    return output;
}

void render(std::function<void(std::ostringstream&)> extend_dot_string, const fs::path& render_path){
    std::ostringstream dot_string;

    dot_string << "digraph G {\n";

    extend_dot_string(dot_string);

    dot_string << "}\n";

    // std::cout << dot_string.str() << std::endl;

    const std::string str = render_path.string();
    std::string command = "dot -Tpng -o " + str;

    pipe_to_command(command, dot_string.str());
}

