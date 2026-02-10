#include <grammar.h>
#include <qf_term.h>
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

void Grammar::consume(const std::string& val){

    if(curr_token.get_ok().value == val){
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

bool Grammar::is_rule(const std::string& rule_name, const Scope& scope){
    auto other = std::make_pair(rule_name, scope);

    for(const auto& ptr : rule_pointers){
        if(*ptr == other){return true;}
    }

    return false;
}

/// Return value of given rule name if the value is 1 branch, with 1 syntax term (string or digit)
std::string Grammar::dig_to_syntax(const std::string& rule_name) const {
    std::shared_ptr<Rule> rule_ptr = get_rule_pointer_if_exists(rule_name, ALL_SCOPES);

    if(rule_ptr == nullptr) return "";

    auto branches = rule_ptr->get_branches();

    if (branches.size() == 1){
        auto terms = branches[0].get_terms();

        if(terms.size() == 1){
            if(is_kind_of_rule(terms[0].get_node_kind())){
                return dig_to_syntax(terms[0].get_string());
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
std::shared_ptr<Rule> Grammar::get_rule_pointer_if_exists(const std::string& name, const Scope& scope) const {
    auto other = std::make_pair(name, scope);

    for(const auto& rp : rule_pointers){
        if(*rp == other){return rp;}
    }

    return nullptr;
}

/// @brief If rule pointer doesn't exist, create it first then return
/// @param token
/// @param scope
/// @return
std::shared_ptr<Rule> Grammar::get_rule_pointer(const Token& token, const Scope& scope){
    auto dummy = std::make_shared<Rule>(token, scope);

    for(const auto& rp : rule_pointers){
        if(*rp == *dummy){return rp;}
    }

    rule_pointers.push_back(dummy);

    return dummy;
}

void Grammar::add_term_to_current_branch(const Token& token){
    std::shared_ptr<Rule>& current_rule = stack.top().rule;
    Branch& current_branch = stack.top().branch;
    Meta_func current_rule_decl_meta_func = stack.top().rule_decl_meta_func;

    assert(current_rule != nullptr);

    if(token.kind == SYNTAX){
        current_branch.add(Term(token.value, token.kind));

    } else if ((token.kind)){
        /*
            each term within the branch of a rule has a scope associated with it
            if explicitis_kind_of_rulely specified like EXTERNAL::term, then the term takes on that scope, otherwise, it
            takes on the scope of the current rule (i.e the rule def)
        */
        Scope scope = (rule_decl_scope == Scope::GLOB) ? current_rule->get_scope() : rule_decl_scope;
        current_branch.add(Term(get_rule_pointer(token, scope), token.kind, current_rule_decl_meta_func));

    } else if (is_meta(token.kind)){
        assert(current_rule_decl_meta_func == Meta_func::NONE); // this is a meta-func used for node creation, should not have a meta func applied to itself
        assert(rule_decl_scope == Scope::GLOB); // also should not have scope set explictly
        current_branch.add(Term(get_rule_pointer(token, current_rule->get_scope()), token.kind, Meta_func::NONE));

    } else {
        throw std::runtime_error(ANNOT("add_term_to_branch should only be called on syntax or rule tokens!"));
    }

    if((token.value == current_rule->get_name()) && is_kind_of_rule(token.kind)){
        current_branch.set_recursive_flag();
    }
}

void Grammar::add_branch_to_current_rule(){
    Branch& current_branch = stack.top().branch;
    stack.top().rule->add(current_branch);
    stack.top().branch.clear();
}

void Grammar::add_constraint_to_last_term(const Term_constraint& constraint){
    Branch& current_branch = stack.top().branch;

    size_t branch_size = current_branch.size();

    if (branch_size == 0){
        ERROR("Current branch should have at least one term to add constraint to");
    } else {
        current_branch.at(branch_size - 1).add_constraint(constraint);
    }
}

void Grammar::build_grammar(){

    if(curr_token.is_ok()){
        Token token = curr_token.get_ok();

        if (token.kind == _EOF) {
            // must not peek if at EOF
            return;

        } else if (token.kind == LBRACK){
            setting_term_constraint = true;

        } else if (token.kind == RBRACK){
            setting_term_constraint = false;

        } else if (setting_term_constraint) {
            Term_constraint constraint;

            if (is_meta(token.kind)) {
                Token_kind meta_func = token.kind;
                Token lookahead = next_token.get_ok();

                /*
                    here, make sure after parsing the expression, the current token is BEFORE "]" as the extra consume happens at the bottom
                */

                if (lookahead.value == "]") {
                    constraint = Term_constraint(meta_func, "", 0);

                } else if (lookahead.value == "-" || lookahead.value == "+" || lookahead.value == ">=" || lookahead.value == "<="){
                    consume(1); // consume META
                    Token op = curr_token.get_ok();

                    consume(1); // consume op
                    Token num = curr_token.get_ok();

                    constraint = Term_constraint(meta_func, op.value, safe_stoul(num.value, 0));

                } else if (lookahead.value == "(") {
                    consume(2); // consume META and "("

                    Token rand_min = curr_token.get_ok();

                    consume(1);

                    consume(",");

                    Token rand_max = curr_token.get_ok();
                    consume(1);

                    if (meta_func == UNIFORM){
                        constraint = Term_constraint(Term_constraint_kind::RANDOM_MAX, safe_stoul(rand_min.value, 0), safe_stoul(rand_max.value, 1));
                    } else {
                        error("Unknown meta function while setting term constraint", token);
                    }

                } else {
                    error("Unexpected token after meta function while setting term constraint", lookahead);
                }

            } else if (token.kind == SYNTAX){
                // numbers are also treated as SYNTAX
                auto val = safe_stoul(token.value, 1);
                constraint = Term_constraint(Term_constraint_kind::RANDOM_MAX, val, val);

            } else {
                print_tokens();
                error("Token kind cannot be used to set constraint", token);
            }

            add_constraint_to_last_term(constraint);

        } else if (is_meta(token.kind) && (next_token.get_ok().kind != LANGLE_BRACKET)){
            // if next token is `<`, this is a meta func application, handled at `<` using previous token
            add_term_to_current_branch(token);

        } else if(is_kind_of_rule(token.kind) || token.kind == SYNTAX){

            // rules that are within branches, rules before `RULE_START` and `RULE_APPEND` are handled at `RULE_START` and `RULE_APPEND`
            if(!stack.empty()){
                add_term_to_current_branch(token);
                // add_term_to_current_branches(token);
                rule_decl_scope = Scope::GLOB; // reset to GLOB scope as default
            }

        } else if (token.kind == RULE_START) {
            if (stack.empty()){
                stack.push(Current(get_rule_pointer(prev_token, rule_def_scope)));
                stack.top().rule->clear();
            } else {
                std::cout << "Grammar: " << name << std::endl;
                std::cout << "STACK_SIZE: " << stack.size() << std::endl;
                std::cout << "STACK TOP: " << *stack.top().rule << std::endl;
                ERROR("At RULE_START current stack is expected to be empty");
            }

        } else if (token.kind == RULE_APPEND){
            if (stack.empty()){
                stack.push(Current(get_rule_pointer(prev_token, rule_def_scope)));
            } else {
                std::cout << "Grammar: " << name << std::endl;
                std::cout << "STACK_SIZE: " << stack.size() << std::endl;
                std::cout << *stack.top().rule << std::endl;
                ERROR("At RULE_APPEND current stack is expected to be empty");
            }

        } else if (token.kind == RULE_END){
            complete_rule();

        } else if (token.kind == LPAREN){
            Token new_rule_token{.value = "NR_" + std::to_string(new_rule_counter++), .kind = RULE};
            Scope parent_scope = stack.top().rule->get_scope();
            stack.push(Current(get_rule_pointer(new_rule_token, parent_scope)));

        } else if (token.kind == RPAREN){
            std::shared_ptr<Rule> new_rule_ptr = stack.top().rule;

            complete_rule();

            // add new rule term to current branch
            add_term_to_current_branch(new_rule_ptr->get_token());

        } else if (token.kind == SEPARATOR){
            add_branch_to_current_rule();

            // add_current_branches_to_rule();
            // reset_current_branches();

        } else if (token.kind == OPTIONAL){
            Term_constraint constraint(Term_constraint_kind::RANDOM_MAX, 0, 1);
            add_constraint_to_last_term(constraint);


        } else if (token.kind == ONE_OR_MORE){
            Term_constraint constraint(Term_constraint_kind::RANDOM_MAX, 1, QuteFuzz::WILDCARD_MAX);
            add_constraint_to_last_term(constraint);


        } else if (token.kind == ZERO_OR_MORE){
            Term_constraint constraint(Term_constraint_kind::RANDOM_MAX, 0, QuteFuzz::WILDCARD_MAX);
            add_constraint_to_last_term(constraint);

        } else if (token.kind == LBRACE){
            // scope has been set to some other scope
            assert(rule_def_scope != Scope::GLOB);

        } else if (token.kind == RBRACE){
            // reset to GLOB scope as default
            rule_def_scope = Scope::GLOB;

        } else if (token.kind == LANGLE_BRACKET){
            // use previous token to set meta function
            set_meta_func(prev_token.kind);

        } else if (token.kind == RANGLE_BRACKET){
            stack.top().rule_decl_meta_func = Meta_func::NONE; // reset to NONE as default

        } else if (token.kind == EXTERNAL){

            if(next_token.get_ok().kind == SCOPE_RES){
                rule_decl_scope = Scope::EXT;
            } else {
                rule_def_scope = Scope::EXT;
            }

        } else if (token.kind == INTERNAL){

            if(next_token.get_ok().kind == SCOPE_RES){
                rule_decl_scope = Scope::INT;
            } else {
                rule_def_scope = Scope::INT;
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
