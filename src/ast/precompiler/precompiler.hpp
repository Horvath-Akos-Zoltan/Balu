#pragma once

namespace ast::precompiler {

    void PreCompile(Stmt* stmt) {
        VariableRegistry varReg;
        PreCompile(stmt, varReg, nullptr);
    }
    void PreCompile(Stmt* stmt, VariableRegistry& varReg, Stmt* parentStmt) {
        if (CompilerOptions.verbose) _wcout << L"[PreCompiler] Pre-compiling statement..." << endl;
        if (stmt == nullptr) return;
        if (auto blockStmt = dynamic_cast<BlockStmt*>(stmt)) {
            PreCompileBlockStmt(blockStmt, varReg);
        } else if (auto varDecl = dynamic_cast<VarDeclStmt*>(stmt)) {
            bool varExists = varReg.GetVariable(varDecl->VariableName) != nullptr;
            if (varExists) {
                if (!CompilerOptions.Language.allowRedeclaration) {
                    wstring errorMessage = L"Redeclaration of variable: \"" + varDecl->VariableName + L"\" is not allowed";
                    _wcout << L"Error: " << errorMessage << endl;
                    if (CompilerOptions.panic) {
                        throw runtime_error(string(errorMessage.begin(), errorMessage.end()));
                    }
                }
                UnusedStmt* unusedStmt = new UnusedStmt(varDecl->VariableName);
                PreCompile(unusedStmt, varReg, parentStmt);
                if (auto blockStmt = dynamic_cast<BlockStmt*>(parentStmt)) {
                    size_t ix = blockStmt->IndexOf(varDecl);
                    if (ix != static_cast<size_t>(-1)) {
                        blockStmt->statements.insert(blockStmt->statements.begin() + ix + 1, unusedStmt);  // TODO: Check if this fucks up precompilation
                    }
                }
            }
            varReg.AddVariable(varDecl);
            PreCompile(varDecl->AssignedValue, varReg, varDecl);
        } else if (auto unusedStmt = dynamic_cast<UnusedStmt*>(stmt)) {
            bool doesExist = varReg.GetVariable(unusedStmt->variableIdentifier) != nullptr;
            if (!doesExist) {
                wstring errorMessage = L"Unknown identifier: \"" + unusedStmt->variableIdentifier + L"\"";
                _wcout << L"Error: " << errorMessage << endl;
                if (CompilerOptions.panic) {
                    throw runtime_error(string(errorMessage.begin(), errorMessage.end()));
                }
                delete unusedStmt;
            }
            varReg.RemoveVariable(unusedStmt->variableIdentifier);
        } else if (auto aliasStmt = dynamic_cast<AliasDeclStmt*>(stmt)) {
            bool doesExist = varReg.HasVariable(aliasStmt->AliasName);
            if (doesExist) {
                wstring errorMessage = L"Identifier \"" + aliasStmt->AliasName + L"\" already defined";
                _wcout << L"Error: " << errorMessage << endl;
                if (CompilerOptions.panic) {
                    throw runtime_error(string(errorMessage.begin(), errorMessage.end()));
                }
                delete aliasStmt;
            }
            varReg.AddAlias(aliasStmt);
        }
        else if (auto exprStmt = dynamic_cast<ExpressionStmt*>(stmt)) {
            PreCompile(exprStmt->expression, varReg, stmt);
        } else if (auto typeChangeStmt = dynamic_cast<TypeChangeStmt*>(stmt)) {
            PreCompile(typeChangeStmt->NewExpr, varReg, stmt);
        } else if (auto aliasStmt = dynamic_cast<AliasDeclStmt*>(stmt)) {
            PreCompile(aliasStmt->AliasedValue, varReg, stmt);
        } else if (auto ifStmt = dynamic_cast<IfStmt*>(stmt)) {
            PreCompile(ifStmt->Condition, varReg, stmt);
            PreCompile(ifStmt->ThenBranch, varReg, ifStmt);
            PreCompile(ifStmt->ElseBranch, varReg, ifStmt);
        } else if (auto whileStmt = dynamic_cast<WhileStmt*>(stmt)) {
            PreCompile(whileStmt->Condition, varReg, stmt);
            PreCompile(whileStmt->Body, varReg, whileStmt);
            PreCompile(whileStmt->ElseBranch, varReg, whileStmt);
        } else if (auto funcDeclStmt = dynamic_cast<FuncDeclStmt*>(stmt)) {
            PreCompile(funcDeclStmt->Body, varReg, funcDeclStmt);
        }
    }
    void PreCompile(Expr* expr, VariableRegistry& varReg, Stmt* stmt) {
        if (expr == nullptr) return;

        if (auto assignmentExpr = dynamic_cast<AssignmentExpr*>(expr)) {
            PreCompile(assignmentExpr->Assignee, varReg, stmt);
            PreCompile(assignmentExpr->Value, varReg, stmt);
        } else if (auto identifierExpr = dynamic_cast<IdentifierExpr*>(expr)) {
            Expr* aliasedExpr = varReg.GetAliasFor(identifierExpr->value);
            if (aliasedExpr != nullptr) {
                expr = aliasedExpr;  // TODO: check for security stuff
            }
            varReg.UsedVariable(identifierExpr->value, stmt);
        } else if (auto binaryExpr = dynamic_cast<BinaryExpr*>(expr)) {
            PreCompile(binaryExpr->left, varReg, stmt);
            PreCompile(binaryExpr->right, varReg, stmt);
        } else if (auto unaryExpr = dynamic_cast<UnaryExpr*>(expr)) {
            PreCompile(unaryExpr->RightExpr, varReg, stmt);
        } else if (auto returnExpr = dynamic_cast<ReturnExpr*>(expr)) {
            PreCompile(returnExpr->Value, varReg, stmt);
        }
    }
}