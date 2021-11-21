#pragma once

#include "ast.hpp"
#include "utf8.hpp"
#include <cstdlib>
#include <fstream>
#include <iostream>

template <typename ExprPtrType> bool is_same_ptr(ExprAST *e) {
    return dynamic_cast<ExprPtrType>(e);
}

void recursive_ast(ExprAST *e, int &max_idx, std::ofstream &fout) {
    ++max_idx;

    if (is_same_ptr<BinaryExprAST *>(e)) {
        auto b = dynamic_cast<BinaryExprAST *>(e);

        auto curr_id = max_idx;
        fout << "idx" + std::to_string(max_idx) << ";\n";
        fout << "idx" + std::to_string(max_idx) << "[label=\""
             << utf8::to_string(b->opr) << "\"] ;\n";
        fout << "idx" + std::to_string(curr_id) << " -- "
             << "idx" + std::to_string(max_idx + 1) << ";\n";

        recursive_ast(b->lhs.get(), max_idx, fout);
        fout << "idx" + std::to_string(curr_id) << " -- "
             << "idx" + std::to_string(max_idx + 1) << ";\n";
        recursive_ast(b->rhs.get(), max_idx, fout);
    } else if (is_same_ptr<NumberAST *>(e)) {
        auto n = dynamic_cast<NumberAST *>(e);

        fout << "idx" + std::to_string(max_idx) << ";\n";
        fout << "idx" + std::to_string(max_idx) << "[label=\""
             << std::to_string(n->value) << "\"] ;\n";

    } else if (is_same_ptr<VariableAST *>(e)) {
        auto v = dynamic_cast<VariableAST *>(e);

        fout << "idx" + std::to_string(max_idx) << ";\n";
        fout << "idx" + std::to_string(max_idx) << "[label=\"" << v->var_name
             << "\"] ;\n";

    } else if (is_same_ptr<FunctionCallAST *>(e)) {
        auto f = dynamic_cast<FunctionCallAST *>(e);

        fout << "idx" + std::to_string(max_idx) << ";\n";
        fout << "idx" + std::to_string(max_idx) << "[label=\"" << f->callee
             << "\"] ;\n";

    } else if (is_same_ptr<FunctionPrototypeAST *>(e)) {
        auto f = dynamic_cast<FunctionPrototypeAST *>(e);
        fout << "idx" + std::to_string(max_idx) << ";\n";
        fout << "idx" + std::to_string(max_idx) << "[label=\""
             << "Proto: " << f->function_name << "\"] ;\n";

    } else if (is_same_ptr<FunctionAST *>(e)) {
        auto f = dynamic_cast<FunctionAST *>(e);

        fout << "idx" + std::to_string(max_idx) << ";\n";
        fout << "idx" + std::to_string(max_idx) << "[label=\""
             << f->prototype->function_name << "\"] ;\n";

    } else {
        fout << "idx" + std::to_string(max_idx) << ";\n";
        fout << "idx" + std::to_string(max_idx) << "[label=\"ExprAST\"] ;\n";
    }
}

void visualise_ast(ExprAST *root) {
    std::ofstream fout("graph.dot");
    int max_idx = 0;

    fout << "graph \"\" {\n"
         << "label=\"Abstract Syntax Tree\"";

    recursive_ast(root, max_idx, fout);

    fout << "}";

    fout.close();

    std::system("dot -Tpng graph.dot > graph.png");
    std::system("xdg-open graph.png");
}
