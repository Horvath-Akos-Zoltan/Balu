#pragma once

namespace ast::precompiler {
    void PreCompileBlockStmt(BlockStmt* blockStmt, VariableRegistry& varReg) {
        uint64_t scope = varReg.AddScope();

        for (auto& stmt : blockStmt->statements) {
            if (stmt != nullptr) PreCompile(stmt, varReg, blockStmt);
        }
        varReg.AdjustDeclarationIndices(blockStmt, 0);
        vector<VariableInfo*> remainingVars = varReg.GetVariablesInScope(scope);

        if (remainingVars.size() > 0) {
            for (const auto& varInfo : remainingVars) {
                bool isUndeletable = !varInfo->canBeDestroyed;
                if (isUndeletable) {
                    if (CompilerOptions.Warnings.undeletableVariableRemains) {
                        _wcout << L"Warning: Undeletable variable found: \"" << varInfo->variableName << L"\" in scope " << scope << endl;
                        isUndeletable = true;  // Only delete at the end of the scope for security reasons
                    }
                }
                if (varInfo->usage.empty()) {
                    size_t declarationIndex = blockStmt->IndexOf(varInfo->declaration);
                    if (declarationIndex == static_cast<size_t>(-1)) {
                        throw logic_error("Variable declaration is missing from its block.");
                    }
                    blockStmt->statements[declarationIndex] = nullptr;
                    delete varInfo->declaration;
                    varInfo->declaration = nullptr;
                    continue;
                }
                UnusedStmt* unusedStmt = new UnusedStmt(varInfo->variableName);
                size_t insertIndex = blockStmt->statements.size();
                if (!isUndeletable && !varInfo->usage.empty()) {
                    size_t lastUsageIndex = blockStmt->IndexOf(varInfo->usage.back());
                    if (lastUsageIndex != static_cast<size_t>(-1)) {
                        insertIndex = lastUsageIndex + 1;
                    }
                }

                blockStmt->statements.insert(blockStmt->statements.begin() + insertIndex, unusedStmt);
            }
        }
        blockStmt->Fix();
        varReg.RemoveScope(scope);
    }
}