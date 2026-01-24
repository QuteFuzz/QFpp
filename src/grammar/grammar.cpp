#include <grammar.h>
#include <term.h>
#include <params.h>

Grammar::Grammar(const fs::path& filename, std::vector<Token>& meta_grammar_tokens): lexer(filename.string()), name(filename.stem()), path(filename) {

    tokens = append_vectors(meta_grammar_tokens, lexer.get_tokens());

    num_tokens = tokens.size();

    consume(0); // prepare current token
    peek(); // prepare next token
}

void Grammar::consume(int n){

    token_pointer += n;

    if(token_pointer == num_tokens){
        curr_token.set_error("Out of tokens!");
    } else {
        curr_token.set_ok(tokens[token_pointer]);
    }
}

void Grammar::consume(const Token_kind kind){

    if(curr_token.get_ok().kind == kind){
        consume(1);
    } else {
        curr_token.set_error("Expected syntax not matched");
    }
}

void Grammar::peek(){
    if((token_pointer + 1) == num_tokens){
        next_token.set_error("Cannot peek!");
    } else {
        next_token.set_ok(tokens[token_pointer+1]);
    }

}

/// Return value of given rule name if the value is 1 branch, with 1 syntax term (string or digit) 
std::string Grammar::dig(const std::string& rule_name) const {
    std::shared_ptr<Rule> rule_ptr = get_rule_pointer_if_exists(rule_name); 

    if(rule_ptr == nullptr) return "";

    auto branches = rule_ptr->get_branches();
    
    if (branches.size() == 1){
        auto terms = branches[0].get_terms();

        if(terms.size() == 1){
            if(is_kind_of_rule(terms[0].get_kind())){
                return dig(terms[0].get_string());
            } else {
                return terms[0].get_syntax();
            }
        } else {
            return "";
        }
    } else {
        return "";
    }
}

/// @brief Only return rule pointer if already exists
/// @param name 
/// @param scope 
/// @return 
std::shared_ptr<Rule> Grammar::get_rule_pointer_if_exists(const std::string& name, const U8& scope) const {

    for(const auto& rp : rule_pointers){
        if(rp->matches(name, scope)){return rp;}
    }

    return nullptr;
}

/// @brief If rule pointer doesn't exist, create it first then return
/// @param token 
/// @param scope 
/// @return 
std::shared_ptr<Rule> Grammar::get_rule_pointer(const Token& token, const U8& scope){
    auto dummy = std::make_shared<Rule>(token, scope);

    for(const auto& rp : rule_pointers){
        if(*rp == *dummy){return rp;}
    }

    rule_pointers.push_back(dummy);

    return dummy;
}

/// @brief Convert a single token into a term and add it to the given branch
/// @param token
void Grammar::add_term_to_branch(const Token& token, Branch& branch){

    if(token.kind == SYNTAX){
        branch.add(Term(token.value, token.kind, nesting_depth));

    } else if (is_kind_of_rule(token.kind)){
        /*
            each term within the branch of a rule has a scope associated with it
            if explicitly specified like EXTERNAL::term, then the term takes on that scope, otherwise, it
            takes on the scope of the current rule (i.e the rule def)
        */
        U8 scope = (rule_decl_scope == NO_SCOPE) && (current_rule != nullptr) ? current_rule->get_scope() : rule_decl_scope;

        branch.add(Term(get_rule_pointer(token, scope), token.kind, nesting_depth));

    } else {
        throw std::runtime_error(ANNOT("Build branch should only be called on syntax or rule tokens!"));
    }

    if((current_rule != nullptr) && (token.value == current_rule->get_name()) && is_kind_of_rule(token.kind)){
        branch.set_recursive_flag();
    }
}

/// @brief The token into a term and add it to all current branches
/// @param tokens
void Grammar::add_term_to_current_branches(const Token& token){
    if(current_branches.size() == 0){
        Branch b;

        add_term_to_branch(token, b);
        current_branches.push_back(b);

    } else {
        for(Branch& current_branch : current_branches){
            add_term_to_branch(token, current_branch);
        }
    }
}

void Grammar::extend_current_branches(const Token& wildcard){
    // loop through current heads, and multiply
    std::vector<Branch> extensions;
    Branch_multiply basis;

    for(const Branch& current_branch : current_branches){
        if(!current_branch.is_empty()){
            basis.clear();
            current_branch.setup_basis(basis, nesting_depth);

            if(wildcard.kind == OPTIONAL){
                extensions.push_back(Branch(basis.remainders));
                break;

            } else if (wildcard.kind == ZERO_OR_MORE){
                extensions.push_back(Branch(basis.remainders));
            }

            // use basis to get extensions depending on the wildcard being processed
            for(unsigned int mult = 2; mult <= QuteFuzz::WILDCARD_MAX; ++mult){

                auto terms = append_vectors(basis.remainders, multiply_vector(basis.mults, mult));

                Branch extension(terms);
                extensions.push_back(extension);
            }

        }
    }

    increment_nesting_depth_base(); // must be done after wildcards are processed

    current_branches.insert(current_branches.end(), extensions.begin(), extensions.end());
}

void Grammar::build_grammar(){

    if(curr_token.is_ok()){
        Token token = curr_token.get_ok();

        // cannot set here because if curr token is EOF, next should be an error.
        // I set this for only the specific cases where I use it to avoid an extra if statement checking for ok here
        Token next;

        if (token.kind == _EOF) {
            // must not peek if at EOF
            return;
        }

        if(is_kind_of_rule(token.kind) || token.kind == SYNTAX){
            next = next_token.get_ok();

            // rules that are within branches, rules before `RULE_START` are handled at `RULE_START`
            if(current_rule != nullptr){
                add_term_to_current_branches(token);
                rule_decl_scope = NO_SCOPE;
            }

        } else if (token.kind == RULE_START) {
            reset_current_branches();
            current_rule = get_rule_pointer(prev_token, rule_def_scope);
            current_rule->clear();

        } else if (token.kind == RULE_APPEND){
            reset_current_branches();
            current_rule = get_rule_pointer(prev_token, rule_def_scope);

        } else if (token.kind == RULE_END){
            complete_rule(); current_rule = nullptr;

        } else if (token.kind == LPAREN){
            nesting_depth += 1;

        } else if (token.kind == RPAREN){
            nesting_depth -= 1;

            next = next_token.get_ok();

            if(!is_wildcard(next.kind)){
                increment_nesting_depth_base();
            }

        } else if (token.kind == SEPARATOR){
            add_current_branches_to_rule();
            reset_current_branches();

        } else if (is_wildcard(token.kind)){
            extend_current_branches(token);

        } else if (token.kind == RBRACE){
            rule_def_scope = NO_SCOPE;

        } else if (token.kind == EXTERNAL){
            next = next_token.get_ok();

            if(next.kind == SCOPE_RES){
                rule_decl_scope |= EXTERNAL_SCOPE;
            } else {
                rule_def_scope |= EXTERNAL_SCOPE;
            }

        } else if (token.kind == INTERNAL){
            next = next_token.get_ok();

            if(next.kind == SCOPE_RES){
                rule_decl_scope |= INTERNAL_SCOPE;
            } else {
                rule_def_scope |= INTERNAL_SCOPE;
            }

        } else if (token.kind == OWNED){
            next = next_token.get_ok();

            if(next.kind == SCOPE_RES){
                rule_decl_scope |= OWNED_SCOPE;
            } else {
                rule_def_scope |= OWNED_SCOPE;
            }

        } else if (token.kind == GLOBAL){
            next = next_token.get_ok();

            if(next.kind == SCOPE_RES){
                rule_decl_scope |= GLOBAL_SCOPE;
            } else {
                rule_def_scope |= GLOBAL_SCOPE;
            }

        } else if (is_quiet(token.kind)){

        } else {
            throw std::runtime_error(ANNOT("Unknown token: " + token.value));
        }

        prev_token = token;

        consume(1);

        // always peek to prepare for next token
        peek();

        build_grammar();

    } else {
        ERROR(curr_token.get_error());
    }
}

/// @brief Does not include meta-grammar tokens
void Grammar::print_tokens() const {
    lexer.print_tokens();
}

void Grammar::print_rules() const {
    for(const auto& p : rule_pointers){
        std::cout << p->get_name() << " ";
    }
}
