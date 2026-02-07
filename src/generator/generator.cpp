#include <generator.h>
#include <ast_utils.h>


std::shared_ptr<Ast> Generator::setup_builder(const Control& control){
    std::shared_ptr<Ast> builder = std::make_shared<Ast>(control);

    if(grammar->is_rule(entry_name, entry_scope)){
        builder->set_entry(grammar->get_rule_pointer_if_exists(entry_name, entry_scope));

    } else if(builder->entry_set()){
        WARNING("Rule " + entry_name + STR_SCOPE(entry_scope) + " is not defined for grammar " + grammar->get_name() + ". Will use previous entry instead");

    } else {
        WARNING("Rule " + entry_name + " is not defined for grammar " + grammar->get_name());
    }

    return builder;
}

Node Generator::build_equivalent(Node ast_root){
    // for each COMPOUND_STMTS node, apply mutation rules
    for(auto& compound_stmts : Node_gen(ast_root, COMPOUND_STMTS)){
        RULE->apply(compound_stmts);
    }

    return ast_root;
}

void Generator::ast_to_program(fs::path output_dir, const Control& control, unsigned int seed){
    rng().seed(seed);

    std::ofstream stream;

    stream = get_stream(output_dir, "circuit_seed.txt");
    stream << seed << std::endl;

    stream = get_stream(output_dir, "prog" + control.ext);

    std::shared_ptr<Ast> builder = setup_builder(control);
    Result<Node> maybe_ast_root = builder->build();

    if(maybe_ast_root.is_ok()){
        Node ast_root = maybe_ast_root.get_ok();

        ast_root.print_program(stream);
        stream.close();

        if(control.run_mutate){
            fs::path equi_dir = output_dir / "equi_progs";

            stream = get_stream(equi_dir, "equi_prog0" + control.ext);

            Node ast_root = build_equivalent(ast_root);
            ast_root.print_program(stream);
            stream.close();
        }

        if (control.render) {
            // builder->render_dag(output_dir);
            builder->render_ast(output_dir);
        }

    } else {
        WARNING(maybe_ast_root.get_error());
    }
}
