#pragma once

namespace ast::decompiler {
    wstring DecompileStatement(ast::Stmt* stmt, int indent = 0);
    wstring DecompileExpression(ast::Expr* expr, int indent = 0);
    wstring vectorToWstring(const vector<wstring>& vec, wchar_t delimiter=L',') {
        wstring result;
        for (size_t i = 0; i < vec.size(); ++i) {
            result += vec[i];
            if (i < vec.size() - 1) {
                result += delimiter;
            }
        }
        return result;
    }
    wstring DecompileBlockStmt(ast::BlockStmt* block, int indent = 0) {
        wstring result = wstring(indent * 4, L' ') + (block->isScopeless ? L"<> " : L"") + L"{\n";
        for (ast::Stmt* stmt : block->statements) {
            result += DecompileStatement(stmt, indent + 1) + L";\n";
        }
        result += wstring(indent * 4, L' ') + L"}";
        return result;
    }
    wstring DecompileExpressionStmt(ast::ExpressionStmt* stmt, int indent = 0) {
        return wstring(indent * 4, L' ') + DecompileExpression(stmt->expression, indent);
    }
    wstring DecompileVarDeclStmt(ast::VarDeclStmt* stmt, int indent = 0) {
        wstring result = wstring(indent * 4, L' ') + L"used ";
        if (!stmt->isConstant) {
            result += L"let ";
        } else {
            result += L"const ";
        }
        result += stmt->VariableName + (stmt->ExplicitType ? L": " + stmt->ExplicitType->GetName() : L"");
        if (stmt->AssignedValue) {
            result += L" = " + DecompileExpression(stmt->AssignedValue, indent);
        }
        return result;
    }
    wstring DecompileTypeChangeStmt(ast::TypeChangeStmt* stmt, int indent = 0) {
        return wstring(indent * 4, L' ') + L"typeof " + stmt->VariableName + L" = " + DecompileExpression(stmt->NewExpr, indent);
    }
    wstring DecompileIfStmt(ast::IfStmt* stmt, int indent = 0) {
        wstring result = wstring(indent * 4, L' ') + L"if (" + DecompileExpression(stmt->Condition, 0) + L") " + strip_left(DecompileStatement(stmt->ThenBranch, indent), L' ');
        if (stmt->ElseBranch) {
            result += L" else " + strip_left(DecompileStatement(stmt->ElseBranch, indent), L' ');
        }
        return result;
    }
    wstring DecompileExpression(ast::Expr* expr, int indent) {
        if (auto s = dynamic_cast<ast::BinaryExpr*>(expr)) {
            wstring left = DecompileExpression(s->left, indent);
            wstring right = DecompileExpression(s->right, indent);
            const bool isMember = s->op.kind == lexer::DOT;
            if (isMember) return left + L"." + right;
            return L'(' + left + L" " + s->op.value + L" " + right + L')';
        } else if (auto s = dynamic_cast<ast::NumberExpr*>(expr)) {
            if (s->isPrecise()) {
                return s->preciseValue + L"@";
            }
            return to_wstring(s->value);
        } else if (auto s = dynamic_cast<ast::RuleExpr*>(expr)) {
            return L"#" + vectorToWstring(s->value, L' ');
        } else if (auto s = dynamic_cast<ast::IdentifierExpr*>(expr)) {
            return s->value;
        } else if (auto s = dynamic_cast<ast::StringExpr*>(expr)) {
            return to_wstring(s->byteSize * 8) + L"\"" + s->value + L"\"";
        } else if (auto s = dynamic_cast<ast::CharExpr*>(expr)) {
            return to_wstring(s->byteSize * 8) + L"'" + wstring(1, s->value) + L"'";
        } else if (auto s = dynamic_cast<ast::ByteExpr*>(expr)) {
            return to_wstring(s->value) + L"u8";
        } else if (auto s = dynamic_cast<ast::SByteExpr*>(expr)) {
            return to_wstring(s->value) + L"i8";
        } else if (auto s = dynamic_cast<ast::ShortExpr*>(expr)) {
            return to_wstring(s->value) + L"i16";
        } else if (auto s = dynamic_cast<ast::UShortExpr*>(expr)) {
            return to_wstring(s->value) + L"u16";
        } else if (auto s = dynamic_cast<ast::Int24Expr*>(expr)) {
            return to_wstring(s->value) + L"i24";
        } else if (auto s = dynamic_cast<ast::UInt24Expr*>(expr)) {
            return to_wstring(s->value) + L"u24";
        } else if (auto s = dynamic_cast<ast::Int32Expr*>(expr)) {
            return to_wstring(s->value) + L"i32";
        } else if (auto s = dynamic_cast<ast::UInt32Expr*>(expr)) {
            return to_wstring(s->value) + L"u32";
        } else if (auto s = dynamic_cast<ast::Int64Expr*>(expr)) {
            return to_wstring(s->value) + L"i64";
        } else if (auto s = dynamic_cast<ast::UInt64Expr*>(expr)) {
            return to_wstring(s->value) + L"u64";
        } else if (auto s = dynamic_cast<ast::BooleanExpr*>(expr)) {
            return s->value ? L"true" : L"false";
        } else if (auto s = dynamic_cast<ast::NullExpr*>(expr)) {
            return L"null";
        } else if (auto s = dynamic_cast<ast::PrefixExpr*>(expr)) {
            return s->Operator.value + DecompileExpression(s->RightExpr, indent);
        } else if (auto s = dynamic_cast<ast::TypeExpr*>(expr)) {
            return s->value->GetName();
        } else if (auto s = dynamic_cast<ast::SymbolExpr*>(expr)) {
            return s->value;
        } else if (auto s = dynamic_cast<ast::ReturnExpr*>(expr)) {
            return L"return " + DecompileExpression(s->Value, indent);
        } else if (auto s = dynamic_cast<ast::FunctionCallExpr*>(expr)) {
            wstring result = s->FunctionName + L"(";
            for (size_t i = 0; i < s->Arguments.size(); ++i) {
                const auto& arg = s->Arguments[i];
                result += DecompileExpression(arg, indent);
                if (i < s->Arguments.size() - 1) {
                    result += L", ";
                }
            }
            result += L")";
            return result;
        } else if (auto s = dynamic_cast<ast::AssignmentExpr*>(expr)) {
            wstring result = DecompileExpression(s->Assignee, indent) + L" " + s->Operator.value + L" " + DecompileExpression(s->Value, indent);
            return result;
        } else if (auto s = dynamic_cast<ast::BreakExpr*>(expr)) {
            wstring result = L"break";
            if (s->Power == ast::BreakPower::Function) {
                result += L" fn";
            } else if (s->Power == ast::BreakPower::All) {
                result += L" *";
            } else if (s->Power == ast::BreakPower::Block) {
                result += L" .";
            }
            if (s->levels > 1) {
                result += L" " + to_wstring(s->levels);
            }
            return result;
        } else if (auto s = dynamic_cast<ast::VolatileExpr*>(expr)) {
            return L"volatile " + DecompileStatement(s->InnerStmt, indent);
        }
        else {
            return L"<unknown expr>";
        }
    }
    wstring DecompileStatement(ast::Stmt* stmt, int indent) {
        if (auto s = dynamic_cast<ast::BlockStmt*>(stmt)) {
            return DecompileBlockStmt(s, indent);
        } else if (auto s = dynamic_cast<ast::ExpressionStmt*>(stmt)) {
            return DecompileExpressionStmt(s, indent);
        } else if (auto s = dynamic_cast<ast::VarDeclStmt*>(stmt)) {
            return DecompileVarDeclStmt(s, indent);
        } else if (auto s = dynamic_cast<ast::TypeChangeStmt*>(stmt)) {
            return DecompileTypeChangeStmt(s, indent);
        } else if (auto s = dynamic_cast<ast::IfStmt*>(stmt)) {
            return DecompileIfStmt(s, indent);
        } else if (auto s = dynamic_cast<ast::WhileStmt*>(stmt)) {
            wstring result = wstring(indent * 4, L' ') + L"while (" + DecompileExpression(s->Condition, 0) + L") " + strip_left(DecompileStatement(s->Body, indent), L' ');
            if (s->ElseBranch) {
                result += L" else " + DecompileStatement(s->ElseBranch, indent);
            }
            return result;
        } else if (auto s = dynamic_cast<ast::FuncDeclStmt*>(stmt)) {
            wstring result = wstring(indent * 4, L' ') + (s->Lining == FLInline ? L"inline " : (s->Lining == FLOutline ? L"outline " : L"")) + L"fn " + s->FunctionName + L"(";
            for (size_t i = 0; i < s->Parameters.size(); ++i) {
                const auto& param = s->Parameters[i];
                if (param->isConstant) {
                    result += L"const ";
                }
                if (param->isAlias) {
                    result += L"alias ";
                }
                result += param->Name;
                if (!param->isAlias) {
                    result += L": " + param->ParamType->GetName();
                }
                if (param->DefaultValue) {
                    result += L" = " + DecompileExpression(param->DefaultValue, 0);
                }
                if (i < s->Parameters.size() - 1) {
                    result += L", ";
                }
            }
            result += L")";
            if (s->ReturnType) {
                result += L": " + s->ReturnType->GetName();
            }
            result += L" " + DecompileStatement(s->Body, indent);
            return result;
        } else if (auto s = dynamic_cast<ast::AliasDeclStmt*>(stmt)) {
            return wstring(indent * 4, L' ') + L"alias " + s->AliasName + L" = " + DecompileExpression(s->AliasedValue, 0);
        } else if (auto s = dynamic_cast<ast::UnusedStmt*>(stmt)) {
            return wstring(indent * 4, L' ') + L"unused " + s->variableIdentifier;
        }
        else {
            return L"<unknown stmt>";
        }
    }
}