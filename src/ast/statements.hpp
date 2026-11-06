#pragma once

namespace ast {
    enum FunctionLining {
        FLAutomatic = 0,
        FLInline,
        FLOutline
    };
    wstring FunctionLiningString(FunctionLining fl) {
        switch (fl) {
            case FLAutomatic: return L"automatic";
            case FLInline: return L"inline";
            case FLOutline: return L"outline";
            default: return L"unknown";
        }
    }
    enum AccessModifier {
        AMPrivate = 0,  // Only accessible within the struct/class it is declared in
        AMProtected,    // Accessible within the struct/class it is declared in and its subclasses (if it's a class)
        AMPublic,       // Accessible from anywhere
        AMInternal,     // Accessible within the same module (.exe, .dll, etc.), but not from other modules (like dllimport -> cannot access it)
        // Complex modifiers
        AMPrivateInternal, // Accessible within the same module and only within the struct/class it is declared in (like private but also not accessible from other modules)
        AMProtectedInternal, // Accessible within the same module and within the struct/class it is declared
    };
    wstring AccessModifierString(AccessModifier am) {
        switch (am) {
            case AMPrivate: return L"private";
            case AMProtected: return L"protected";
            case AMPublic: return L"public";
            case AMInternal: return L"internal";
            case AMPrivateInternal: return L"private_internal";
            case AMProtectedInternal: return L"protected_internal";
            default: return L"unknown";
        }
    }
    AccessModifier TokenToAccessModifier(lexer::TokenKind kind) {
        switch (kind) {
            case lexer::PRIVATE: return AMPrivate;
            case lexer::PROTECTED: return AMProtected;
            case lexer::PUBLIC: return AMPublic;
            case lexer::INTERNAL: return AMInternal;
            default: return AMPrivate;  // Default to private if not an access modifier
        }
    }
    // {...Stmt[]}
    struct BlockStmt : public Stmt {
        vector<Stmt*> statements;
        bool isScopeless = false;
        void stmt() override {}
        BlockStmt(vector<Stmt*> statements, bool isScopeless = false) : statements(statements), isScopeless(isScopeless) {}
        BlockStmt() : statements({}) {}
        BlockStmt(BlockStmt&& other) noexcept : statements(std::move(other.statements)) {
            other.statements.clear();
        }
        BlockStmt& operator=(BlockStmt&& other) noexcept {
            if (this != &other) {
                for (auto s : statements) delete s;
                statements = std::move(other.statements);
                other.statements.clear();
            }
            return *this;
        }
        size_t IndexOf(Stmt* stmt) {
            for (size_t i = 0; i < statements.size(); ++i) {
                if (statements[i] == stmt) return i;
            }
            return static_cast<size_t>(-1); // Not found
        }
        void Fix() {
            vector<Stmt*> nonNullStatements;
            for (auto s : statements) {
                if (s != nullptr) nonNullStatements.push_back(s);
            }
            statements = std::move(nonNullStatements);
        }
        BlockStmt(const BlockStmt&) = delete;
        BlockStmt& operator=(const BlockStmt&) = delete;
        ~BlockStmt() {
            for (auto s : statements) delete s;
        }
        void Dump(int indent = 0, wostream& wcout_ = _wcout) const override {
            wcout_ << wstring(indent * 2, L' ') << L"BlockStmt" << (isScopeless ? L" <>" : L"") << endl;
            for (auto s : statements) {
                if (s) s->Dump(indent + 1, wcout_);
            }
        }
        static BlockStmt Null() { return BlockStmt(); }
        static BlockStmt* NullPtr() { return new BlockStmt(); }
        wstring kind() const override { return L"BlockStmt"; }
        Stmt* Clone() const override {
            vector<Stmt*> clonedStatements;
            for (Stmt* stmt : statements) clonedStatements.push_back(stmt ? stmt->Clone() : nullptr);
            return new BlockStmt(clonedStatements);
        }
    };
    struct ExpressionStmt : public Stmt {
        Expr* expression;
        void stmt() override {}
        ExpressionStmt(Expr* e) : expression(e) {}
        ~ExpressionStmt() {
            delete expression;
        }
        void Dump(int indent = 0, wostream& wcout_ = _wcout) const override {
            wcout_ << wstring(indent * 2, L' ') << L"ExpressionStmt" << endl;
            if (expression) {
                expression->Dump(indent + 1, wcout_);
            }
        }
        wstring kind() const override { return L"ExpressionStmt"; }
        Stmt* Clone() const override {
            return new ExpressionStmt(expression ? expression->Clone() : nullptr);
        }
    };
    struct VarDeclStmt : public Stmt {
        wstring VariableName;
        bool isConstant;
        bool mayAutoDelete;
        Expr* AssignedValue;
        Type* ExplicitType;
        void stmt() override {}
        VarDeclStmt(const wstring& n, bool c, Expr* init, Type* type, bool mayAutoDel = true) : VariableName(n), isConstant(c), AssignedValue(init), ExplicitType(type), mayAutoDelete(mayAutoDel) {}
        ~VarDeclStmt() {
            delete AssignedValue;
            delete ExplicitType;
        }
        void Dump(int indent = 0, wostream& wcout_ = _wcout) const override {
            wcout_ << wstring(indent * 2, L' ') << L"VarDeclStmt: " << VariableName << (isConstant ? L" (const)" : L"") << (!mayAutoDelete ? L" (manually managed)" : L"") << endl;
            if (ExplicitType) {
                ExplicitType->Dump(indent + 1, wcout_);
            }
            if (AssignedValue) {
                AssignedValue->Dump(indent + 1, wcout_);
            }
        }
        wstring kind() const override { return L"VarDeclStmt"; }
        Stmt* Clone() const override {
            return new VarDeclStmt(VariableName, isConstant, AssignedValue ? AssignedValue->Clone() : nullptr, ExplicitType ? ExplicitType->Clone() : nullptr, mayAutoDelete);
        }
    };
    // typeof varName = typeName;
    struct TypeChangeStmt : public Stmt {
        wstring VariableName;
        Expr* NewExpr;
        void stmt() override {}
        TypeChangeStmt(const wstring& n, Expr* e) : VariableName(n), NewExpr(e) {}
        ~TypeChangeStmt() {
            delete NewExpr;
        }
        void Dump(int indent = 0, wostream& wcout_ = _wcout) const override {
            wcout_ << wstring(indent * 2, L' ') << L"TypeChangeStmt: " << VariableName << endl;
            if (NewExpr) {
                NewExpr->Dump(indent + 1, wcout_);
            }
        }
        wstring kind() const override { return L"TypeChangeStmt"; }
        Stmt* Clone() const override {
            return new TypeChangeStmt(VariableName, NewExpr ? NewExpr->Clone() : nullptr);
        }
    };
    struct MethodParameter {
        wstring Name;
        Type* ParamType;
        ast::Expr* DefaultValue;
        bool isConstant;
        bool isAlias;
        MethodParameter(const wstring& n, Type* t, ast::Expr* d, bool c, bool a) : Name(n), ParamType(t), DefaultValue(d), isConstant(c), isAlias(a) {}
        ~MethodParameter() {
            delete ParamType;
            delete DefaultValue;
        }
        void Dump(int indent = 0, wostream& wcout_ = _wcout) const {
            wcout_ << wstring(indent * 2, L' ') << L"MethodParameter: " << Name << (isConstant ? L" (const)" : L"") << (isAlias ? L" (alias)" : L"") << endl;
            if (ParamType) {
                ParamType->Dump(indent + 1, wcout_);
            }
            if (DefaultValue) {
                DefaultValue->Dump(indent + 1, wcout_);
            }
        }
        MethodParameter* Clone() const {
            return new MethodParameter(Name, ParamType ? ParamType->Clone() : nullptr, DefaultValue ? DefaultValue->Clone() : nullptr, isConstant, isAlias);
        }
    };
    struct StructProperty {
        wstring Property;
        ast::Type* Type;
        ast::Expr* AssignedValue;
        bool isStatic;
        AccessModifier Access;
        bool isConstant;
        StructProperty(const wstring& p, ast::Type* t, ast::Expr* v, bool s, AccessModifier a, bool c) : Property(p), Type(t), AssignedValue(v), isStatic(s), Access(a), isConstant(c) {}
        ~StructProperty() {
            delete Type;
            delete AssignedValue;
        }
        void Dump(int indent = 0, wostream& wcout_ = _wcout) const {
            wcout_ << wstring(indent * 2, L' ') << L"StructProperty: " << Property << (isStatic ? L" (static)" : L"") << (isConstant ? L" (const)" : L"") << L" (" << AccessModifierString(Access) << L")" << endl;
            if (Type) {
                Type->Dump(indent + 1, wcout_);
            }
            if (AssignedValue) {
                AssignedValue->Dump(indent + 1, wcout_);
            }
        }
        StructProperty* Clone() const {
            return new StructProperty(Property, Type ? Type->Clone() : nullptr, AssignedValue ? AssignedValue->Clone() : nullptr, isStatic, Access, isConstant);
        }
    };
    struct StructMethod {  // TODO: implement (add fn type) ; Replace with FuncDeclStmt
        bool isStatic;
        AccessModifier Access;
        wstring MethodName;
        Type* ReturnType;
        Stmt* Body;
        unordered_map<wstring, MethodParameter*> Parameters;
        bool isConstant;
        StructMethod(const wstring& n, Type* t, Stmt* b, bool s, AccessModifier a, unordered_map<wstring, MethodParameter*> params, bool c) : MethodName(n), ReturnType(t), Body(b), isStatic(s), Access(a), Parameters(params), isConstant(c) {}
        ~StructMethod() {
            delete ReturnType;
            delete Body;
            for (auto& p : Parameters) delete p.second;
        }
        void Dump(int indent = 0, wostream& wcout_ = _wcout) const {
            wcout_ << wstring(indent * 2, L' ') << L"StructMethod: " << MethodName << (isStatic ? L" (static)" : L"") << (isConstant ? L" (const)" : L"") << L" (" << AccessModifierString(Access) << L")" << endl;
            if (ReturnType) {
                ReturnType->Dump(indent + 1, wcout_);
            }
            for (const auto& param : Parameters) {
                wcout_ << wstring((indent + 1) * 2, L' ') << L"Parameter: " << param.first << endl;
                if (param.second) param.second->Dump(indent + 2, wcout_);
            }
            if (Body) {
                Body->Dump(indent + 1, wcout_);
            }
        }
        StructMethod* Clone() const {
            unordered_map<wstring, MethodParameter*> clonedParams;
            for (const auto& p : Parameters) clonedParams[p.first] = p.second ? p.second->Clone() : nullptr;
            return new StructMethod(MethodName, ReturnType ? ReturnType->Clone() : nullptr, Body ? Body->Clone() : nullptr, isStatic, Access, clonedParams, isConstant);
        }
    };
    struct StructDeclStmt : public Stmt {
        wstring StructName;
        unordered_map<wstring, StructProperty*> Properties;
        unordered_map<wstring, StructMethod*> Methods;
        StructDeclStmt(const wstring& n, const unordered_map<wstring, StructProperty*>& p, const unordered_map<wstring, StructMethod*>& m) : StructName(n), Properties(p), Methods(m) {}
        ~StructDeclStmt() {
            for (auto& p : Properties) delete p.second;
            for (auto& m : Methods) delete m.second;
        }
        void stmt() override {}
        void Dump(int indent = 0, wostream& wcout_ = _wcout) const override {
            wcout_ << wstring(indent * 2, L' ') << L"StructDeclStmt: " << StructName << endl;
            for (const auto& p : Properties) {
                p.second->Dump(indent + 1, wcout_);
            }
            for (const auto& m : Methods) {
                m.second->Dump(indent + 1, wcout_);
            }
        }
        wstring kind() const override { return L"StructDeclStmt"; }
        Stmt* Clone() const override {
            unordered_map<wstring, StructProperty*> clonedProperties;
            for (const auto& p : Properties) clonedProperties[p.first] = p.second->Clone();
            unordered_map<wstring, StructMethod*> clonedMethods;
            for (const auto& m : Methods) clonedMethods[m.first] = m.second->Clone();
            return new StructDeclStmt(StructName, clonedProperties, clonedMethods);
        }
    };
    struct AliasDeclStmt : public Stmt {
        wstring AliasName;
        ast::Expr* AliasedValue;
        void stmt() override {}
        AliasDeclStmt(const wstring& n, ast::Expr* v) : AliasName(n), AliasedValue(v) {}
        ~AliasDeclStmt() {
            delete AliasedValue;
        }
        void Dump(int indent = 0, wostream& wcout_ = _wcout) const override {
            wcout_ << wstring(indent * 2, L' ') << L"AliasDeclStmt: " << AliasName << endl;
            if (AliasedValue) {
                AliasedValue->Dump(indent + 1, wcout_);
            }
        }
        wstring kind() const override { return L"AliasDeclStmt"; }
        Stmt* Clone() const override {
            return new AliasDeclStmt(AliasName, AliasedValue ? AliasedValue->Clone() : nullptr);
        }
    };
    struct IfStmt : public Stmt {
        ast::Expr* Condition;
        ast::Stmt* ThenBranch;
        ast::Stmt* ElseBranch;  // can be nullptr
        void stmt() override {}
        IfStmt(ast::Expr* condition, ast::Stmt* thenBranch, ast::Stmt* elseBranch) : Condition(condition), ThenBranch(thenBranch), ElseBranch(elseBranch) {}
        ~IfStmt() {
            delete Condition;
            delete ThenBranch;
            delete ElseBranch;
        }
        void Dump(int indent = 0, wostream& wcout_ = _wcout) const override {
            wcout_ << wstring(indent * 2, L' ') << L"IfStmt" << endl;
            if (Condition) {
                wcout_ << wstring((indent + 1) * 2, L' ') << L"Condition:" << endl;
                Condition->Dump(indent + 2, wcout_);
            }
            if (ThenBranch) {
                wcout_ << wstring((indent + 1) * 2, L' ') << L"ThenBranch:" << endl;
                ThenBranch->Dump(indent + 2, wcout_);
            }
            if (ElseBranch) {
                wcout_ << wstring((indent + 1) * 2, L' ') << L"ElseBranch:" << endl;
                ElseBranch->Dump(indent + 2, wcout_);
            }
        }
        wstring kind() const override { return L"IfStmt"; }
        Stmt* Clone() const override {
            return new IfStmt(Condition ? Condition->Clone() : nullptr, ThenBranch ? ThenBranch->Clone() : nullptr, ElseBranch ? ElseBranch->Clone() : nullptr);
        }
    };
    struct WhileStmt : public Stmt {
        ast::Expr* Condition;
        ast::Stmt* Body;
        ast::Stmt* ElseBranch;  // can be nullptr
        void stmt() override {}
        WhileStmt(ast::Expr* condition, ast::Stmt* body, ast::Stmt* elseBranch = nullptr) : Condition(condition), Body(body), ElseBranch(elseBranch) {}
        ~WhileStmt() {
            delete Condition;
            delete Body;
            delete ElseBranch;
        }
        void Dump(int indent = 0, wostream& wcout_ = _wcout) const override {
            wcout_ << wstring(indent * 2, L' ') << L"WhileStmt" << endl;
            if (Condition) {
                wcout_ << wstring((indent + 1) * 2, L' ') << L"Condition:" << endl;
                Condition->Dump(indent + 2, wcout_);
            }
            if (Body) {
                wcout_ << wstring((indent + 1) * 2, L' ') << L"Body:" << endl;
                Body->Dump(indent + 2, wcout_);
            }
            if (ElseBranch) {
                wcout_ << wstring((indent + 1) * 2, L' ') << L"ElseBranch:" << endl;
                ElseBranch->Dump(indent + 2, wcout_);
            }
        }
        wstring kind() const override { return L"WhileStmt"; }
        Stmt* Clone() const override {
            return new WhileStmt(Condition ? Condition->Clone() : nullptr, Body ? Body->Clone() : nullptr, ElseBranch ? ElseBranch->Clone() : nullptr);
        }
    };
    struct FuncDeclStmt : public Stmt {
        wstring FunctionName;
        Type* ReturnType;
        Stmt* Body;
        vector<MethodParameter*> Parameters;
        FunctionLining Lining;
        void stmt() override {}
        FuncDeclStmt(const wstring& n, Type* t, Stmt* b, FunctionLining l, vector<MethodParameter*> params) : FunctionName(n), ReturnType(t), Body(b), Lining(l), Parameters(params) {}
        ~FuncDeclStmt() {
            delete ReturnType;
            delete Body;
            for (auto& p : Parameters) delete p;
        }
        void Dump(int indent = 0, wostream& wcout_ = _wcout) const override {
            wcout_ << wstring(indent * 2, L' ') << L"FuncDeclStmtcls: " << FunctionName << (Lining == FLInline ? L" (inline)" : (Lining == FLOutline ? L" (outline)" : L"")) << endl;
            if (ReturnType) {
                ReturnType->Dump(indent + 1, wcout_);
            }
            for (const auto& param : Parameters) {
                wcout_ << wstring((indent + 1) * 2, L' ') << L"Parameter: " << param << endl;
                if (param) param->Dump(indent + 2, wcout_);
            }
            if (Body) {
                Body->Dump(indent + 1, wcout_);
            }
        }
        wstring kind() const override { return L"FunctionStmt"; }
        Stmt* Clone() const override {
            vector<MethodParameter*> clonedParams;
            for (auto& p : Parameters) clonedParams.push_back(p ? p->Clone() : nullptr);
            return new FuncDeclStmt(FunctionName, ReturnType ? ReturnType->Clone() : nullptr, Body ? Body->Clone() : nullptr, Lining, clonedParams);
        }
    };
    struct UnusedStmt : public Stmt {
        wstring variableIdentifier;
        UnusedStmt(const wstring& varId) : variableIdentifier(varId) {}
        UnusedStmt(VarDeclStmt* varDecl) : variableIdentifier(varDecl ? varDecl->VariableName : L"") {}
        void stmt() override {}
        void Dump(int indent = 0, wostream& wcout_ = _wcout) const override {
            wcout_ << GetName(indent) << endl;
        }
        wstring kind() const override { return L"UnusedStmt"; }
        wstring GetName(int indent = 0) const {
            return wstring(indent * 2, L' ') + L"UnusedStmt: " + variableIdentifier;
        }
        Stmt* Clone() const override {
            return new UnusedStmt(variableIdentifier);
        }
    };
    struct ImportStmt : public Stmt {
        wstring ModuleName;
        wstring namespaceName;
        void stmt() override {}
        ImportStmt(const wstring& moduleName, const wstring& nsName = L"") : ModuleName(moduleName), namespaceName(nsName) {}
        void Dump(int indent = 0, wostream& wcout_ = _wcout) const override {
            wcout_ << wstring(indent * 2, L' ') << L"ImportStmt: " << ModuleName;
            if (!namespaceName.empty()) {
                wcout_ << L" (namespace: " << namespaceName << L")";
            }
            wcout_ << endl;
        }
        wstring kind() const override { return L"ImportStmt"; }
        Stmt* Clone() const override {
            return new ImportStmt(ModuleName, namespaceName);
        }
    };
}