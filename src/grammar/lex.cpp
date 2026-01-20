#include <lex.h>

void Lexer::lex(){
    std::string input, matched_string;
    std::vector<Token> tokens;
    std::ifstream stream(_filename);

    std::regex full_pattern(FULL_REGEX, std::regex::icase);

    std::sregex_iterator end;

    bool in_multiline_comment = false;

    while(std::getline(stream, input)){
        
        std::sregex_iterator begin(input.begin(), input.end(), full_pattern);
        std::sregex_iterator end;

        for(std::sregex_iterator i = begin; i != end; ++i){
            std::string text = i->str();

            /*
                ignore comments
            */

            if (text == "(*") {
                in_multiline_comment = true;
                continue; // Skip this token
            }

            if (text == "*)") {
                in_multiline_comment = false;
                continue; // Skip this token
            }
            
            if (in_multiline_comment) {
                continue;
            }

            if (text[0] == '#') {
                continue; // Skip the whole comment string
            }
            
            if (isspace(text[0])) {
                continue;
            }

            // find exact qf tokens            
            bool found_keyword = false;
            
            for(const Token_matcher& tm : TOKEN_RULES) {
                if (text == tm.pattern) {
                    tokens.push_back(Token{tm.replacement.value_or(text), tm.kind});
                    found_keyword = true;
                    break;
                }
            }

            if (found_keyword) continue;

            
            // classify generics
            if (isalpha(text[0]) || text[0] == '_') {
                tokens.push_back(Token{text, RULE});

            } else if (isdigit(text[0])) {
                tokens.push_back(Token{text, SYNTAX});

            } else {
                tokens.push_back(Token{remove_outer_quotes(text), SYNTAX});
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
