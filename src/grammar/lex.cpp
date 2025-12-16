#include <lex.h>

void Lexer::lex(){
    std::string input, matched_string;
    std::vector<Token> tokens;
    std::ifstream stream(_filename);
    
    std::regex full_pattern(FULL_REGEX, std::regex::icase);

    std::sregex_iterator end;

    while(std::getline(stream, input)){
        
        std::sregex_iterator begin(input.begin(), input.end(), full_pattern);

        for(std::sregex_iterator i = begin; (i != end); ++i){
            std::smatch match = *i;
            matched_string = match.str();

            if(string_is(matched_string, R"(#)")){
                break;

            } else if(string_is(matched_string, R"(\(\*)")){
                ignore = true;
                break;
            
            } else if (string_is(matched_string, R"(\*\))")){
                ignore = false;
                break;

            } else {
            
                for(const Regex_matcher rm : TOKEN_RULES){
                    if(string_is(matched_string, rm.pattern)){

                        if(rm.kind == SYNTAX){
                            tokens.push_back(Token{remove_outer_quotes(rm.value.value_or(matched_string)), rm.kind});                        
                        
                        } else {
                            tokens.push_back(Token{rm.value.value_or(matched_string), rm.kind});                        
                        }

                        break;
                    }
                }
            }
        }
    }

    tokens.push_back(Token{.value = "", .kind = _EOF});        
    result.set_ok(tokens);
}

void Lexer::print_tokens() const {

    if(result.is_error()){
        ERROR(result.get_error()); 
        
    } else {
        std::vector<Token> tokens = result.get_ok();

        for(size_t i = 0; i < tokens.size(); ++i){
            std::cout << tokens[i] << std::endl;
        }
    }
}