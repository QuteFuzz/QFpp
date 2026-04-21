#include <grammar.h>
#include <qf_term.h>
#include <params.h>

Grammar::Grammar(const fs::path& filename, std::vector<Token>& meta_grammar_tokens) : 
    lexer(filename.string()), 
    name(filename.stem()), 
    path(filename)
{
    tokens = append_vectors(meta_grammar_tokens, lexer.get_tokens());

    num_tokens = tokens.size();

    consume(0); // prepare current token
    peek(); // prepare next token
}

[[noreturn]]
void Grammar::grammar_error(const std::string& msg, std::source_location loc){
    std::cout << "===================================" << std::endl;
    std::cout << "Grammar: " << name << std::endl;
    std::cout << "STACK_SIZE: " << stack.size() << std::endl;

    std::cout << "STACK TOP: ";
    if (stack.top().rule == nullptr) {
        std::cout << "null ";
    } else {
        std::cout << *stack.top().rule;
    }
    std::cout << std::endl;

    std::cout << "Curr token: " << curr_token << std::endl;
    ERROR(msg, loc);
}

bool Grammar::check(const std::string& expected, std::source_location loc){
    if (curr_token.value == expected){
        return true;
    } else {
        grammar_error("Expected syntax not matched. Expected " +  expected + " got " + curr_token.value, loc);
    }
}

/// Consume n tokens, set previous token to last consumed token if possible (`token_pointer > 0`)
void Grammar::consume(int n){
    token_pointer += n;

    if(token_pointer == num_tokens){
        grammar_error("Out of tokens! Consumed too much");
    } else {
        curr_token = tokens[token_pointer];
    }
}

void Grammar::consume(const std::string& expected, std::source_location loc){
    check(expected, loc);
    consume();
}

void Grammar::peek(){
    if((token_pointer + 1) < num_tokens){
        next_token = tokens[token_pointer+1];
    } else if (curr_token.kind != _EOF){
        grammar_error("Cannot peek!");
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

    if(token.kind == STRING || token.kind == NUMBER){
        current_branch.add(Term(token.value, token.kind));

    } else if (is_kind_of_rule(token.kind)){
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
        ERROR("add_term_to_branch should only be called on syntax or rule tokens!");
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

void Grammar::add_constraint_to_last_term(){
    Branch& current_branch = stack.top().branch;

    size_t branch_size = current_branch.size();

    if (branch_size == 0){
        ERROR("Current branch should have at least one term to add constraint to");
    } else {
        current_branch.at(branch_size - 1).add_term_constraint(curr_expr);
    }
}

std::unique_ptr<Expr> Grammar::build_factor() {
    if (curr_token.value == "(") {
        consume(); // advance to the first token inside parens
        auto expr = build_expr(); 
        consume(); // advance to ')'
        check(")"); // ensure it is ')', check does not consume so ) remains as curr_token
        expr->paren = true;
        return expr;

    } else if (curr_token.value == "UNIFORM"){
        consume(); // consume UNIFORM
        consume("("); // checks and consumes '('
        
        auto left = build_expr(); // curr_token lands on last token of left expr
        
        consume(); // advance to ','
        consume(","); // checks and consumes ','
        
        auto right = build_expr(); // curr_token lands on last token of right expr
        
        consume(); // advance to ')'
        check(")"); // ensure it is ')'
        return std::make_unique<BinExpr>("UNIFORM", std::move(left), std::move(right));

    } else if (curr_token.kind == NUMBER){
        return std::make_unique<IntExpr>(safe_stoi(curr_token.value, 0));

    } else {
        return std::make_unique<VarExpr>(curr_token.kind); 
    }
}

std::unique_ptr<Expr> Grammar::build_term() {
    auto left = build_factor();
    peek(); 
    
    while (next_token.value == "*" || next_token.value == "/" || 
           next_token.value == ">" || next_token.value == "<" || 
           next_token.value == "<=" || next_token.value == ">=") 
    {
        consume(); // advance curr_token to the operator itself
        std::string op = curr_token.value;
        
        consume(); // advance curr_token past the operator to the next factor
        auto right = build_factor();
        
        // fold the tree downward and leftward
        left = std::make_unique<BinExpr>(op, std::move(left), std::move(right));
        
        peek(); // peek ahead again to see if the chain continues
    }
    
    return left;
}


std::unique_ptr<Expr> Grammar::build_expr() {
    auto left = build_term();
    peek();
    
    while (next_token.value == "+" || next_token.value == "-" || 
           next_token.value == "==" || next_token.value == "!=") 
    {
        consume(); // advance curr_token to the operator itself
        std::string op = curr_token.value;
        
        consume(); // advance curr_token past the operator to the next term
        auto right = build_term();
        
        // fold the tree downward and leftward
        left = std::make_unique<BinExpr>(op, std::move(left), std::move(right));
        
        peek(); // peek ahead again to see if the chain continues
    }
    
    return left;
}

/// @brief Builds the grammar. Note the `consume` at the bottom, which advances to the next token. This means that any computations that deal with a series
/// of tokens must always exit with `curr_token` pointing at the LAST token in the series. For example in an expression [3 + 5], after building the `Expr` AST,
/// `curr_token` must point to `5`.
void Grammar::build_grammar(){

    if (curr_token.kind == _EOF) {
        // must not peek if at EOF
        return;

    } else if (curr_token.kind == LBRACK){
        consume();
        curr_expr = std::move(build_expr());

    } else if (curr_token.kind == RBRACK){
        add_constraint_to_last_term();

    } else if (is_meta(curr_token.kind) && (next_token.kind != LANGLE_BRACKET)){
        // if next token is `<`, this is a meta func application, handled at `<` using previous curr_token
        add_term_to_current_branch(curr_token);

    } else if(is_kind_of_rule(curr_token.kind) || curr_token.kind == STRING || curr_token.kind == NUMBER){

        // rules that are within branches, rules before `RULE_START` and `RULE_APPEND` are handled at `RULE_START` and `RULE_APPEND`
        if(!stack.empty()){
            add_term_to_current_branch(curr_token);
            rule_decl_scope = Scope::GLOB; // reset to GLOB scope as default
        }

    } else if (curr_token.kind == RULE_START) {
        if (stack.empty()){
            stack.push(Current(get_rule_pointer(prev_token, rule_def_scope)));
            stack.top().rule->clear();
        } else {
            grammar_error("At RULE_START current stack is expected to be empty");
        }

    } else if (curr_token.kind == RULE_APPEND){
        if (stack.empty()){
            stack.push(Current(get_rule_pointer(prev_token, rule_def_scope)));
        } else {
            grammar_error("At RULE_APPEND current stack is expected to be empty");
        }

    } else if (curr_token.kind == RULE_END){
        complete_rule();

    } else if (curr_token.kind == LPAREN){
        Token new_rule_token{.value = "NR_" + std::to_string(new_rule_counter++), .kind = RULE};
        Scope parent_scope = stack.top().rule->get_scope();
        stack.push(Current(get_rule_pointer(new_rule_token, parent_scope)));

    } else if (curr_token.kind == RPAREN){
        std::shared_ptr<Rule> new_rule_ptr = stack.top().rule;
        complete_rule();

        // add new rule term to current branch
        add_term_to_current_branch(new_rule_ptr->get_token());

    } else if (curr_token.kind == SEPARATOR){
        add_branch_to_current_rule();

    } else if (curr_token.kind == OPTIONAL){
        curr_expr = std::make_shared<IntExpr>(random_uint(1, 0));
        add_constraint_to_last_term();

    } else if (curr_token.kind == ONE_OR_MORE){
        curr_expr = std::make_shared<IntExpr>(random_uint(QuteFuzz::WILDCARD_MAX, 1));
        add_constraint_to_last_term();

    } else if (curr_token.kind == ZERO_OR_MORE){
        curr_expr = std::make_shared<IntExpr>(random_uint(QuteFuzz::WILDCARD_MAX, 0));
        add_constraint_to_last_term();

    } else if (curr_token.kind == LBRACE){
        // scope has been set to some other scope
        assert(rule_def_scope != Scope::GLOB);

    } else if (curr_token.kind == RBRACE){
        // reset to GLOB scope as default
        rule_def_scope = Scope::GLOB;

    } else if (curr_token.kind == LANGLE_BRACKET){
        // use previous curr_token to set meta function
        set_meta_func(prev_token.kind);

    } else if (curr_token.kind == RANGLE_BRACKET){
        stack.top().rule_decl_meta_func = Meta_func::NONE; // reset to NONE as default

    } else if (curr_token.kind == EXTERNAL){
        if(next_token.kind == SCOPE_RES){
            rule_decl_scope = Scope::EXT;
        } else {
            rule_def_scope = Scope::EXT;
        }

    } else if (curr_token.kind == INTERNAL){
        if(next_token.kind == SCOPE_RES){
            rule_decl_scope = Scope::INT;
        } else {
            rule_def_scope = Scope::INT;
        }

    } else if (is_quiet(curr_token.kind)){

    } else {
        ERROR("Unknown curr_token: " + curr_token.value);
    }

    prev_token = curr_token;
    consume();
    peek(); // always peek to prepare for next token

    build_grammar();
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
