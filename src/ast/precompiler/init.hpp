#pragma once

namespace ast::precompiler {
    struct VariableInfo {
        wstring variableName = L"";
        Stmt* declaration = nullptr;
        size_t declarationIndex = 0;
        vector<Stmt*> usage = {};
        size_t scope = 0;
        Type* type = nullptr;
        bool canInline = false;
        bool canBeDestroyed = false;

        bool isAlias = false;
        Expr* aliasedExpr = nullptr;
    };
    struct VariableRegistry : vector<VariableInfo> {
        uint64_t currentScope = 0;
        constexpr static size_t DEFAULT_DECL_INDEX = static_cast<size_t>(-1);
        VariableInfo& AddVariable(VarDeclStmt* varDecl, size_t declarationIndex = DEFAULT_DECL_INDEX) {
            VariableInfo varInfo;
            if (declarationIndex == DEFAULT_DECL_INDEX) {
                declarationIndex = size();
            }
            varInfo.variableName = varDecl->VariableName;
            varInfo.declaration = varDecl;
            varInfo.declarationIndex = declarationIndex;
            varInfo.type = varDecl->ExplicitType;
            varInfo.canBeDestroyed = varDecl->mayAutoDelete;
            varInfo.scope = currentScope;
            push_back(varInfo);
            return back();
        }
        VariableInfo& AddAlias(AliasDeclStmt* aliasDecl, size_t declarationIndex = DEFAULT_DECL_INDEX) {
            VariableInfo varInfo;
            if (declarationIndex == DEFAULT_DECL_INDEX) {
                declarationIndex = size();
            }
            varInfo.variableName = aliasDecl->AliasName;
            varInfo.declaration = aliasDecl;
            varInfo.declarationIndex = declarationIndex;
            varInfo.scope = currentScope;
            varInfo.aliasedExpr = aliasDecl->AliasedValue;
            push_back(varInfo);
            return back();
        }
        bool HasVariable(VarDeclStmt* varDecl) {
            for (const auto& varInfo : *this) {
                if (varInfo.declaration == varDecl) {
                    return true;
                }
            }
            return false;
        }
        bool HasVariable(const wstring& variableName) {
            for (const VariableInfo& varInfo : *this) {
                if (varInfo.declaration && varInfo.variableName == variableName) {
                    return true;
                }
            }
            return false;
        }
        VariableInfo* GetVariable(const wstring& variableName) {
            for (auto& varInfo : *this) {
                if (varInfo.declaration && varInfo.variableName == variableName) {
                    return &varInfo;
                }
            }
            return nullptr;
        }
        void UsedVariable(const wstring& variableName, Stmt* usageStmt) {
            for (auto& varInfo : *this) {
                if (varInfo.declaration && varInfo.variableName == variableName) {
                    varInfo.usage.push_back(usageStmt);
                    break;
                }
            }
        }
        void RemoveVariable(VarDeclStmt* varDecl) {
            this->erase(std::remove_if(this->begin(), this->end(),
                [varDecl](const VariableInfo& varInfo) { return varInfo.declaration == varDecl; }),
                this->end());
        }
        void RemoveVariable(const wstring& variableName) {
            this->erase(std::remove_if(this->begin(), this->end(),
                [variableName](const VariableInfo& varInfo) { return varInfo.variableName == variableName; }),
                this->end());
        }
        void fix() {
            this->erase(std::remove_if(this->begin(), this->end(),
                [](const VariableInfo& varInfo) { return varInfo.declaration == nullptr; }),
                this->end());
        }
        uint64_t AddScope() {
            return ++currentScope;
        }
        vector<VariableInfo*> GetVariablesInScope(size_t scope) {
            vector<VariableInfo*> varsInScope;
            for (auto& varInfo : *this) {
                if (varInfo.scope == scope) {
                    varsInScope.push_back(&varInfo);
                }
            }
            return varsInScope;
        }
        void RemoveScope(size_t scope, bool updateCurrentScope = true) {
            if (updateCurrentScope && currentScope == scope) {
                currentScope--;
            }
            this->erase(std::remove_if(this->begin(), this->end(),
                [scope](const VariableInfo& varInfo) { return varInfo.scope == scope; }),
                this->end());
        }
        void AdjustDeclarationIndices(BlockStmt* blockStmt, size_t startIndex) {
            for (size_t i = startIndex; i < blockStmt->statements.size(); i++) {
                if (auto varDecl = dynamic_cast<VarDeclStmt*>(blockStmt->statements[i])) {
                    if (VariableInfo* varInfo = GetVariable(varDecl->VariableName)) {
                        varInfo->declarationIndex = i;
                    }
                }
            }
        }
        Expr* GetAliasFor(const wstring& varName) {
            for (auto varInfo : *this) {
                if (varInfo.isAlias && varInfo.variableName == varName) return varInfo.aliasedExpr;
            }
            return nullptr;
        }
    };
    void PreCompile(Stmt* stmt, VariableRegistry& varReg, Stmt* parentStmt);
    void PreCompile(Expr* expr, VariableRegistry& varReg, Stmt* stmt);
}

#include "blockstmt.hpp"
#include "precompiler.hpp"