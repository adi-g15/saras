#pragma once

#include "ast.hpp"
#include "utf8.hpp"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>

template <typename ExprPtrType> bool is_same_ptr(ExprAST *e) {
    return dynamic_cast<ExprPtrType>(e);
}

static void recursive_ast(ExprAST *e, int &max_idx, std::ofstream &fout) {
    ++max_idx;

    if (is_same_ptr<BinaryExprAST *>(e)) {
        auto b = dynamic_cast<BinaryExprAST *>(e);

        fout << "idx" + std::to_string(max_idx) << ";\n";
        fout << "idx" + std::to_string(max_idx) << "[label=\""
             << utf8::to_string(b->opr) << "\"] ;\n";
        fout << "idx" + std::to_string(max_idx) << " -- "
             << "idx" + std::to_string(max_idx + 1) << ";\n";

        auto curr_id = max_idx;
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

    } else if (is_same_ptr<BlockAST *>(e)) {
        auto b = dynamic_cast<BlockAST *>(e);

        if (b->expressions.size() == 1) {
            max_idx--; // to negate effect of ++ in next call, since i want it
                       // to have current max_id
            return recursive_ast(b->expressions.back().get(), max_idx, fout);
        }

        fout << "idx" + std::to_string(max_idx) << ";\n";
        fout << "idx" + std::to_string(max_idx) << "[label=\""
             << "Block"
             << "\"] ;\n";

        fout << "idx" + std::to_string(max_idx) << " -- idx"
             << std::to_string(max_idx + 1) << ";\n";
        for (auto i = 0; i < b->expressions.size(); ++i) {
            ++max_idx;

            auto &expr = b->expressions[i];

            fout << "idx" + std::to_string(max_idx) << ";\n";
            fout << "idx" + std::to_string(max_idx) << "[label=\"Expr["
                 << std::to_string(i) << "]\"] ;\n";

            auto parent = max_idx;
            fout << "idx" + std::to_string(max_idx) << " -- idx"
                 << std::to_string(max_idx + 1) << ";\n";
            recursive_ast(expr.get(), max_idx, fout);

            // Dont create a connection for last node, since it doesn't have any
            // next to connect to
            if (i < b->expressions.size() - 1)
                fout << "idx" + std::to_string(parent) << " -- idx"
                     << std::to_string(max_idx + 1) << ";\n";
        }

        ++max_idx;
    } else if (is_same_ptr<FunctionCallAST *>(e)) {
        auto f = dynamic_cast<FunctionCallAST *>(e);
        fout << "idx" + std::to_string(max_idx) << ";\n";
        fout << "idx" + std::to_string(max_idx) << "[label=\""
             << "FunctionCall: " << f->callee << "\"] ;\n";

        auto parent_node = max_idx;
        for (auto &arg : f->args) {
            recursive_ast(arg.get(), max_idx, fout);
            fout << "idx" + std::to_string(max_idx) << " -- idx"
                 << std::to_string(parent_node) << ";\n";
        }
    } else if (is_same_ptr<FunctionPrototypeAST *>(e)) {
        auto f = dynamic_cast<FunctionPrototypeAST *>(e);
        fout << "idx" + std::to_string(max_idx) << ";\n";
        fout << "idx" + std::to_string(max_idx) << "[label=\""
             << "Prototype: " << f->function_name << "\"] ;\n";

        auto parent_node = max_idx;
        for (auto &arg : f->parameter_names) {
            ++max_idx;
            fout << "idx" + std::to_string(max_idx) << ";\n";
            fout << "idx" + std::to_string(max_idx) << "[label=\"" << arg
                 << "\"] ;\n";
            fout << "idx" + std::to_string(max_idx) << " -- idx"
                 << std::to_string(parent_node) << ";\n";
        }

    } else if (is_same_ptr<FunctionAST *>(e)) {
        auto f = dynamic_cast<FunctionAST *>(e);

        fout << "idx" + std::to_string(max_idx) << ";\n";
        fout << "idx" + std::to_string(max_idx) << "[label=\""
             << f->prototype->function_name << "\"] ;\n";
        fout << "idx" + std::to_string(max_idx) << " -- "
             << "idx" + std::to_string(max_idx + 1) << ";\n";

        auto parent_node = max_idx;
        recursive_ast(f->prototype.get(), max_idx, fout);
        fout << "idx" + std::to_string(parent_node) << " -- "
             << "idx" + std::to_string(max_idx + 1) << ";\n";
        recursive_ast(f->block.get(), max_idx, fout);
    } else if (is_same_ptr<IfExprAST *>(e)) {
        auto f = dynamic_cast<IfExprAST *>(e);

        fout << "idx" + std::to_string(max_idx) << ";\n";
        fout << "idx" + std::to_string(max_idx) << "[label=\""
             << "If"
             << "\"] ;\n";
        fout << "idx" + std::to_string(max_idx) << " -- "
             << "idx" + std::to_string(max_idx + 1) << ";\n";

        auto parent_node = max_idx;

        recursive_ast(f->condition.get(), max_idx, fout);

        fout << "idx" + std::to_string(parent_node) << " -- "
             << "idx" + std::to_string(max_idx + 1) << ";\n";
        recursive_ast(f->then_.get(), max_idx, fout);

        fout << "idx" + std::to_string(parent_node) << " -- "
             << "idx" + std::to_string(max_idx + 1) << ";\n";
        recursive_ast(f->else_.get(), max_idx, fout);
    } else {
        fout << "idx" + std::to_string(max_idx) << ";\n";
        fout << "idx" + std::to_string(max_idx) << "[label=\"ExprAST\"] ;\n";
    }
}

static void visualise_ast(ExprAST *root) {
    static int i = 0;
    if (!root)
        return;

    std::string fname_1 = "graph" + std::to_string(i++);
    std::ofstream fout(fname_1 + ".dot");
    int max_idx = 0;

    fout << "graph \"\" {\n"
         << "label=\"" << i - 1 << ": Abstract Syntax Tree\"";

    recursive_ast(root, max_idx, fout);

    fout << "}";

    fout.close();

    system(std::string("dot -Tpng " + fname_1 + ".dot > " + fname_1 + ".png")
               .c_str());
}
