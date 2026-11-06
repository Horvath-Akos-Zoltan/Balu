#pragma once
#include <filesystem>

namespace parser {
    ast::Expr* getDefaultValueForType(ast::Type* type) {
        if (auto symbolType = dynamic_cast<ast::SymbolType*>(type)) {
            if (symbolType->name == L"number") {
                return new ast::NumberExpr(0);
            } else if (symbolType->name == L"bool") {
                return new ast::BooleanExpr(false);
            } else if (symbolType->name == L"byte") {
                return new ast::ByteExpr(0);
            } else if (symbolType->name == L"sbyte") {
                return new ast::SByteExpr(0);
            } else if (symbolType->name == L"short") {
                return new ast::ShortExpr(0);
            } else if (symbolType->name == L"ushort") {
                return new ast::UShortExpr(0);
            } else if (symbolType->name == L"int24") {
                return new ast::Int24Expr(0);
            } else if (symbolType->name == L"uint24") {
                return new ast::UInt24Expr(0);
            } else if (symbolType->name == L"int32") {
                return new ast::Int32Expr(0);
            } else if (symbolType->name == L"uint32") {
                return new ast::UInt32Expr(0);
            } else if (symbolType->name == L"int64") {
                return new ast::Int64Expr(0);
            } else if (symbolType->name == L"uint64") {
                return new ast::UInt64Expr(0);
            } else if (symbolType->name == L"string8" || symbolType->name == L"string16" || symbolType->name == L"string32") {
                return new ast::StringExpr(L"", symbolType->name);
            } else if (symbolType->name == L"char8" || symbolType->name == L"char16" || symbolType->name == L"char32") {
                return new ast::CharExpr(L'\0', symbolType->name);
            } else if (symbolType->name == L"null") {
                return new ast::NullExpr(ast::NullExpr::NullState::EXPLICIT);
            }
        }
        // For other types, we can return a NullExpr as a default value.
        return new ast::NullExpr(ast::NullExpr::NullState::EXPLICIT);
    }
    ast::Type* parse_type(Parser* parser, binding_power bp);
    ast::Stmt* parse_block_stmt(Parser* p);
    bool is_assignment_operator(lexer::TokenKind kind) {
        return kind == lexer::ASSIGNMENT ||
               kind == lexer::PLUS_EQUALS ||
               kind == lexer::MINUS_EQUALS ||
               kind == lexer::STAR_EQUALS ||
               kind == lexer::SLASH_EQUALS ||
               kind == lexer::PERCENT_EQUALS;
    }
    ast::Stmt* parse_stmt(Parser* p) {
        if (CompilerOptions.verbose) _wcout << L"Parsing statement at " << p->position() << endl;

        if (p->currentTokenKind() == lexer::OPEN_CURLY) {
            return parse_block_stmt(p);
        }

        const lexer::TokenKind currentKind = p->currentTokenKind();

        // Identifier-led assignments (e.g. x = 2;) are expression statements,
        // not declarations. Handle them before stmt lookup routes IDENTIFIER to var decl.
        if (p->currentTokenKind() == lexer::IDENTIFIER && is_assignment_operator(p->nextTokenKind())) {
            ast::Expr* expr = parse_expr(p, default_bp);
            if (expr == nullptr) {
                p->advanceUntil(lexer::SEMICOLON);
                p->expect(lexer::SEMICOLON);
                return nullptr;
            }
            p->expect(lexer::SEMICOLON);
            return new ast::ExpressionStmt(expr);
        }

        bool exists = stmt_lu[p->currentTokenKind()] != nullptr;
        if (exists) {
            return stmt_lu[p->currentTokenKind()](p);
        }

        ast::Expr* expr = parse_expr(p, default_bp); // This could return nullptr if an error occurred and CompilerOptions.panic is false
        if (expr == nullptr) {
            // If expression parsing failed, we should not proceed to create an ExpressionStmt.
            // The error would have been reported by parse_expr.
            // We should try to recover by advancing past the current token, or until a semicolon.
            p->advanceUntil(lexer::SEMICOLON); // Skip to the end of the problematic statement
            p->expect(lexer::SEMICOLON); // Consume the semicolon
            return nullptr; // Return nullptr to indicate failure to parse a valid statement
        }

        if (currentKind != lexer::VOLATILE) p->expect(lexer::SEMICOLON);  // Required semilcolon

        return new ast::ExpressionStmt( expr );
    }
    ast::Stmt* parse_block_stmt(Parser* p) {
        if (CompilerOptions.verbose) _wcout << L"Parsing block statement at " << p->position() << endl;
        bool isScopeless = p->currentTokenKind() == lexer::LESS;
        if (isScopeless) {
            p->advance();  // eat <
            p->expect(lexer::GREATER);
        }
        p->expect(lexer::OPEN_CURLY);  // consume '{'
        vector<ast::Stmt*> statements;
        while (p->hasTokens() && p->currentTokenKind() != lexer::CLOSE_CURLY) {
            ast::Stmt* stmt = parse_stmt(p);
            if (stmt != nullptr) statements.push_back(stmt);
        }
        p->expect(lexer::CLOSE_CURLY);  // consume '}'
        if (p->currentTokenKind() == lexer::SEMICOLON) p->advance();
        return new ast::BlockStmt(statements, isScopeless);
    };
    ast::Stmt* parse_var_decl_stmt(Parser* p) {
        if (CompilerOptions.verbose) _wcout << L"Parsing variable declaration statement at " << p->position() << endl;
        
        const bool mayAutoDelete = p->currentTokenKind() != lexer::USED;
        if (!mayAutoDelete) p->advanceOver(lexer::USED);

        // Declared using the let or const keyword, otherwise it's either a mut, or a typename declaration
        const bool letConst = p->currentTokenKind() == lexer::LET || p->currentTokenKind() == lexer::CONST;
        
        // Keyword alone, skip
        if (p->currentToken().isTypeName() && p->nextTokenKind() == lexer::SEMICOLON) {
            p->advance();
            p->advance();
            return nullptr;
        }

        bool isConstant;
        ast::Type* explicitType = nullptr;
        ast::Expr* assignedValue = nullptr;
        if (letConst) {
            isConstant = p->currentTokenKind() == lexer::CONST;
            p->advance(); // consume 'let' or 'const'
        } else {  // Either 'mut' + typename or the typename itself
            isConstant = p->currentTokenKind() != lexer::MUT;  // If it's not 'mut', then it's a constant declaration
            if (!isConstant) p->advance(); // consume 'mut' if it exists

        }
        // Check if we are looking at a type or just the variable name.
        // We parse a type if:
        // 1. It's a known keyword type (NUMBER, STRING32, etc.)
        // 2. It's a prefix modifier ([)
        // 3. It's an identifier followed by a modifier or another identifier (the variable name)
        lexer::TokenKind nextKind = p->tokens.size() > p->pos + 1 ? p->tokens[p->pos + 1].kind : lexer::EOF_TOKEN;
        if (p->currentTokenKind() != lexer::IDENTIFIER || 
            (nextKind == lexer::IDENTIFIER || nextKind == lexer::OPEN_BRACKET)) 
            explicitType = parse_type(p, default_bp);

        wstring varName = p->expectError(lexer::IDENTIFIER, Error(L"Expected variable name after declaration with " + (letConst ? (isConstant ? wstring(L"'const'") : wstring(L"'let'")) : (isConstant ? wstring(L"typename") : wstring(L"'mut'"))) + L", but got " + lexer::TokenKindString(p->currentTokenKind()) + L" at " + p->position())).value;

        if (mapContainsKey(lexer::reserved_lu, varName)) {
            wstring message = L"Variable name '" + varName + L"' at " + p->position() + L" cannot be a reserved keyword";
            p->errors.push_back(ParserError(message));
            _wcout << (CompilerOptions.debug ? L"[Parser] " : L"") << message << endl;
            if (CompilerOptions.panic) {
                if (CompilerOptions.debug) _wcout << L"[Parser] Panicing" << endl;
                exit(1);
            }
            varName = L'_' + varName;  // Prepend an underscore to make it a valid identifier
        }

        if (p->currentTokenKind() == lexer::COLON) {
            p->advance();  // consume ':'
            if (explicitType != nullptr) {
                ast::Type* newExplicitType = parse_type(p, default_bp);
                wstring message = L"Variable declaration for '" + varName + L"' at " + p->position() + L" can only contain one type, but multiple were found (" + explicitType->GetName() + L" and " + newExplicitType->GetName() + L")";
                p->errors.push_back(ParserError(message));
                _wcout << (CompilerOptions.debug ? L"[Parser] " : L"") << message << endl;
                if (CompilerOptions.panic) {
                    if (CompilerOptions.debug) _wcout << L"[Parser] Panicing" << endl;
                    exit(1);
                }
            } else explicitType = parse_type(p, default_bp);
        }

        // Default to type: auto (const) | any (let/mut)
        if (explicitType == nullptr) {
            if (isConstant) explicitType = new ast::SymbolType(L"auto");
            else explicitType = new ast::SymbolType(L"any");
        }

        if (p->currentTokenKind() != lexer::SEMICOLON) {
            if (p->currentTokenKind() != lexer::ASSIGNMENT) {
                wstring message = L"Expected token at " + p->position() + L" to be either an assignment operator '=' for variable initialization or a semicolon ';' to end the declaration, but found " + lexer::TokenKindString(p->currentTokenKind());
                p->errors.push_back(ParserError(message));
                _wcout << (CompilerOptions.debug ? L"[Parser] " : L"") << message << endl;
                if (CompilerOptions.panic) {
                    if (CompilerOptions.debug) _wcout << L"[Parser] Panicing" << endl;
                    exit(1);
                }
                p->advanceUntil(lexer::SEMICOLON); // Skip to the end of the problematic statement
                p->expect(lexer::SEMICOLON); // Consume the semicolon
                return nullptr;
            }
            p->expect(lexer::ASSIGNMENT); 
            assignedValue = parse_expr(p, assignment);
        }

        p->expect(lexer::SEMICOLON);
        
        if (isConstant) {
            if (explicitType->GetName() == L"any") {
                wstring message = L"Constant variable declaration for '" + varName + L"' at " + p->position() + L" cannot have type 'any' as it cannot be reassigned, did you mean 'auto'?";
                p->errors.push_back(ParserError(message));
                _wcout << (CompilerOptions.debug ? L"[Parser] " : L"") << message << endl;
                if (CompilerOptions.panic) {
                    if (CompilerOptions.debug) _wcout << L"[Parser] Panicing" << endl;
                    exit(1);
                }
                explicitType = new ast::SymbolType(L"auto");
            }
            if (assignedValue == nullptr) {
                wstring message = L"Constant variable declaration for '" + varName + L"' at " + p->position() + L" must have an initializer";
                p->errors.push_back(ParserError(message));
                _wcout << (CompilerOptions.debug ? L"[Parser] " : L"") << message << endl;
                if (CompilerOptions.panic) {
                    if (CompilerOptions.debug) _wcout << L"[Parser] Panicing" << endl;
                    exit(1);
                }
                assignedValue = new ast::NullExpr(ast::NullExpr::NullState::IMPLICIT);
            }
        }

        if (assignedValue == nullptr) assignedValue = new ast::NullExpr();
        else if (auto assignedValueNullExpr = dynamic_cast<ast::NullExpr*>(assignedValue)) {
            assignedValueNullExpr->setStateIfUnset(ast::NullExpr::NullState::EXPLICIT);
        }

        if (auto assignedValueNullExpr = dynamic_cast<ast::NullExpr*>(assignedValue)) {
            if (!assignedValueNullExpr->isExplicitlyStated()) {
                ast::Expr* defaultValue = getDefaultValueForType(explicitType);
                delete assignedValue;
                assignedValue = defaultValue;
            }
        }

        return new ast::VarDeclStmt{
            varName,
            isConstant,
            assignedValue,
            explicitType,
            mayAutoDelete
        };
    }
    ast::Stmt* parse_unused_stmt(Parser* p) {
        if (CompilerOptions.verbose) _wcout << L"Parsing unused statement at " << p->position() << endl;
        p->expect(lexer::UNUSED);  // consume 'unused'

        wstring variableIdentifier = p->expectError(lexer::IDENTIFIER, Error(L"Expected variable identifier after 'unused' keyword, but got " + lexer::TokenKindString(p->currentTokenKind()) + L" at " + p->position())).value;
        if (p->previousTokenKind() != lexer::IDENTIFIER) {
            p->advanceUntil(lexer::SEMICOLON); // Skip to the end of the problematic statement
            p->expect(lexer::SEMICOLON); // Consume the semicolon
            return nullptr;
        }

        p->expect(lexer::SEMICOLON); // Consume the semicolon

        return new ast::UnusedStmt(variableIdentifier);
    }
    ast::Stmt* parse_alias_decl_stmt(Parser* p) {
        if (CompilerOptions.verbose) _wcout << L"Parsing alias declaration statement at " << p->position() << endl;
        p->expect(lexer::ALIAS);  // consume 'alias'

        const wstring correctSyntax = GetCorrectSyntax(lexer::ALIAS);

        if (p->currentTokenKind() == lexer::COLON) p->advance();  // consume ':'
        const bool currentIsTypename = p->currentToken().mightBeType() 
                                        && p->nextTokenKind() == lexer::IDENTIFIER;
        if (currentIsTypename) {
            wstring probableAliasName = currentIsTypename ? p->nextToken().value : L"?";
            wstring message = L"Alias declaration for '" + probableAliasName + L"' at " + p->position() + L" cannot have an explicit type ("+ lexer::TokenKindString(p->currentTokenKind()) + L")" + correctSyntax;
            p->errors.push_back(ParserError(message));
            _wcout << (CompilerOptions.debug ? L"[Parser] " : L"") << message << endl;
            if (CompilerOptions.panic) {
                if (CompilerOptions.debug) _wcout << L"[Parser] Panicing" << endl;
                exit(1);
            }
            if (p->currentTokenKind() == lexer::COLON) p->advance();
            delete parse_type(p, default_bp);
        }

        wstring aliasName = p->expectError(lexer::IDENTIFIER, Error(L"Expected alias name after 'alias' keyword, but got " + lexer::TokenKindString(p->currentTokenKind()) + L" at " + p->position() + correctSyntax)).value;
        if (p->previousTokenKind() != lexer::IDENTIFIER) {
            p->advanceUntil(lexer::SEMICOLON); // Skip to the end of the problematic statement
            p->expect(lexer::SEMICOLON); // Consume the semicolon
            return nullptr;
        }
        if (p->currentTokenKind() == lexer::COLON) {
            wstring message = L"Alias declaration for '" + aliasName + L"' at " + p->position() + L" cannot have an explicit type";
            p->errors.push_back(ParserError(message));
            _wcout << (CompilerOptions.debug ? L"[Parser] " : L"") << message << endl;
            if (CompilerOptions.panic) {
                if (CompilerOptions.debug) _wcout << L"[Parser] Panicing" << endl;
                exit(1);
            }
            p->advance();  // consume ':'

            if (p->currentToken().mightBeType()) parse_type(p, default_bp);
        }

        if (p->currentTokenKind() != lexer::ASSIGNMENT) {
            wstring message = L"Alias '" + aliasName + L"' must be initialized at " + p->position() + correctSyntax;
            p->errors.push_back(ParserError(message));
            _wcout << (CompilerOptions.debug ? L"[Parser] " : L"") << message << endl;
            if (CompilerOptions.panic) {
                if (CompilerOptions.debug) _wcout << L"[Parser] Panicing" << endl;
                exit(1);
            }
            p->advanceUntil(lexer::SEMICOLON); // Skip to the end of the problematic statement
            p->expect(lexer::SEMICOLON); // Consume the semicolon
            return nullptr;
        }
        p->expect(lexer::ASSIGNMENT);

        ast::Expr* aliasedValue = parse_expr(p, assignment);
        p->expect(lexer::SEMICOLON);

        if (aliasedValue == nullptr) {
            wstring message = L"Alias declaration for '" + aliasName + L"' at " + p->position() + L" must have an initializer" + correctSyntax;
            p->errors.push_back(ParserError(message));
            _wcout << (CompilerOptions.debug ? L"[Parser] " : L"") << message << endl;
            if (CompilerOptions.panic) {
                if (CompilerOptions.debug) _wcout << L"[Parser] Panicing" << endl;
                exit(1);
            }
            aliasedValue = new ast::NullExpr();
        }
        return new ast::AliasDeclStmt{
            aliasName,
            aliasedValue
        };
    }
    // Able to parse complex modifiers
    ast::AccessModifier parse_access_modifier(Parser* p, bool* accessModifierSet = nullptr) {
        if (p->currentToken().isAccessModifier()) {
            ast::AccessModifier accessModifier = ast::TokenToAccessModifier(p->currentTokenKind());
            if (p->nextToken().isAccessModifier()) {
                ast::AccessModifier next = ast::TokenToAccessModifier(p->nextTokenKind());
                if (accessModifier == ast::AMPrivate && next == ast::AMInternal || accessModifier == ast::AMInternal && next == ast::AMPrivate) {
                    p->advance();  // consume the first access modifier
                    p->advance();  // consume the sencond access modifier
                    if (accessModifierSet) *accessModifierSet = true;
                    return ast::AMPrivateInternal;
                }
                if (accessModifier == ast::AMProtected && next == ast::AMInternal || accessModifier == ast::AMInternal && next == ast::AMProtected) {
                    p->advance();  // consume the first access modifier
                    p->advance();  // consume the sencond access modifier
                    if (accessModifierSet) *accessModifierSet = true;
                    return ast::AMProtectedInternal;
                }
                if (accessModifier == ast::AMPublic && next == ast::AMInternal || accessModifier == ast::AMInternal && next == ast::AMPublic) {
                    p->advance();  // consume the first access modifier
                    p->advance();  // consume the sencond access modifier
                    if (accessModifierSet) *accessModifierSet = true;
                    return ast::AMInternal;
                }
                wstring message = L"Conflicting access modifiers for struct member at " + p->position() + L" (" + lexer::TokenKindString(p->currentTokenKind()) + L" and " + lexer::TokenKindString(p->nextTokenKind()) + L")";
                p->errors.push_back(ParserError(message));
                _wcout << (CompilerOptions.debug ? L"[Parser] " : L"") << message << endl;
                if (CompilerOptions.panic) {
                    if (CompilerOptions.debug) _wcout << L"[Parser] Panicing" << endl;
                    exit(1);
                }
                p->advanceUntil(lexer::IDENTIFIER, lexer::STATIC, lexer::FN, lexer::STRUCT, lexer::SEMICOLON);  // Skip to the next relevant token
                return ast::AMPrivate;  // Default to private if there are conflicting access modifiers
            }
            p->advance();  // consume the access modifier
            if (accessModifierSet) *accessModifierSet = true;
            return accessModifier;
        }
        return ast::AMPrivate;  // Default access modifier is private
    }
    ast::Stmt* parse_struct_decl_stmt(Parser* p) {
        if (CompilerOptions.verbose) _wcout << L"Parsing struct declaration statement at " << p->position() << endl;
        p->expect(lexer::STRUCT);  // consume 'struct'

        unordered_map<wstring, ast::StructProperty*> properties;
        unordered_map<wstring, ast::StructMethod*> methods;

        wstring structName = p->expectError(lexer::IDENTIFIER, 
            Error(L"Expected struct name after 'struct' keyword, but got " + lexer::TokenKindString(p->currentTokenKind()) + L" at " + p->position())
        ).value;

        p->expect(lexer::OPEN_CURLY);

        while (p->hasTokens() && p->currentTokenKind() != lexer::CLOSE_CURLY) {
            bool accessModifierSet = false;
            ast::AccessModifier accessModifier = parse_access_modifier(p, &accessModifierSet);  // Default access modifier for struct members is private
            bool isStatic = false;
            if (p->currentTokenKind() == lexer::STATIC) {
                isStatic = true;
                p->advance();  // consume 'static'
            }
            if (p->currentToken().isAccessModifier()) {
                if (accessModifierSet) {
                    wstring message = L"Duplicate access modifier for struct member at " + p->position() + L" (" + lexer::TokenKindString(p->currentTokenKind()) + L" and " + AccessModifierString(accessModifier) + L")";
                    p->errors.push_back(ParserError(message));
                    _wcout << (CompilerOptions.debug ? L"[Parser] " : L"") << message << endl;
                    if (CompilerOptions.panic) {
                        if (CompilerOptions.debug) _wcout << L"[Parser] Panicing" << endl;
                        exit(1);
                    }
                }
                accessModifier = parse_access_modifier(p);
            }

            bool isConstant = p->currentTokenKind() == lexer::CONST;
            if (isConstant) p->advance();  // consume 'const'

            ast::Type* ExpectedType = nullptr;
            bool isMethod = p->currentTokenKind() == lexer::FN || p->nextTokenKind(1) == lexer::OPEN_PAREN || p->nextTokenKind(2) == lexer::OPEN_PAREN;
            if (isMethod && p->nextTokenKind(1) != lexer::OPEN_PAREN) p->advance();  // consume 'fn' / typename

            // fn typename methodname() {} or static fn typename methodname() {}
            if ((p->currentTokenKind() == lexer::IDENTIFIER || p->currentToken().isType()) && p->nextTokenKind() == lexer::IDENTIFIER) {
                ExpectedType = parse_type(p, default_bp);
                if (ExpectedType == nullptr) ExpectedType = new ast::SymbolType(L"auto");
                if (ExpectedType->GetName() == L"any") {
                    wstring message = L"Struct method declaration for '" + p->currentToken().value + L"' at " + p->position() + L" cannot have return type 'any', did you mean 'auto'?";
                    p->errors.push_back(ParserError(message));
                    _wcout << (CompilerOptions.debug ? L"[Parser] " : L"") << message << endl;
                    if (CompilerOptions.panic) {
                        if (CompilerOptions.debug) _wcout << L"[Parser] Panicing" << endl;
                        exit(1);
                    }
                    delete ExpectedType;
                    ExpectedType = new ast::SymbolType(L"auto");
                }
            }
            wstring propertyName = p->advance().value;

            // Name & collision check
            if (p->currentTokenKind() == lexer::IDENTIFIER) {
                if (properties.find(propertyName) != properties.end()) {
                    if (!isMethod) {
                        wstring message = L"Duplicate struct property '" + propertyName + L"' found in struct '" + structName + L"' declaration at " + p->position();
                        p->errors.push_back(ParserError(message));
                        _wcout << (CompilerOptions.debug ? L"[Parser] " : L"") << message << endl;
                        if (CompilerOptions.panic) {
                            if (CompilerOptions.debug) _wcout << L"[Parser] Panicing" << endl;
                            exit(1);
                        }
                        p->advanceUntil(lexer::SEMICOLON);  // Skip until the end of the property declaration
                        continue;
                    }
                    // TODO: Method overloading based on parameter types and count
                }
            }

            // Interpret type
            if (p->currentTokenKind() == lexer::COLON) {
                p->advance();  // consume ':'
                if (ExpectedType != nullptr) {
                    wstring message = L"Struct property declaration for '" + propertyName + L"' at " + p->position() + L" cannot have an explicit type as it was already specified as '" + ExpectedType->GetName() + L"'";
                    p->errors.push_back(ParserError(message));
                    _wcout << (CompilerOptions.debug ? L"[Parser] " : L"") << message << endl;
                    if (CompilerOptions.panic) {
                        if (CompilerOptions.debug) _wcout << L"[Parser] Panicing" << endl;
                        exit(1);
                    }
                    p->advance();  // consume the type
                } else ExpectedType = parse_type(p, default_bp);
            } else if (!isMethod && ExpectedType == nullptr) ExpectedType = new ast::SymbolType(L"any");  // Default to 'any' type for struct properties if no type is specified
            
            ast::Expr* AssignedValue = nullptr;
            ast::Stmt* MethodBody = nullptr;
            unordered_map<wstring, ast::MethodParameter*> parameters;
            
            // Method parameters
            if (isMethod) {  // Parse method
                p->expect(lexer::OPEN_PAREN);
                while (p->hasTokens() && p->currentTokenKind() != lexer::CLOSE_PAREN)
                {
                    wstring paramName = L"UnnamedParam";
                    ast::Type* paramType = nullptr;
                    ast::Expr* defaultValue = nullptr;
                    if (p->currentTokenKind() == lexer::CONST) {
                        isConstant = true;
                        p->advance();  // consume 'const'
                    }
                    bool isAlias = p->currentTokenKind() == lexer::ALIAS;
                    if (isAlias) p->advance();
                    if (p->currentTokenKind() == lexer::IDENTIFIER || p->currentToken().isType()) {
                        if (p->nextTokenKind() == lexer::IDENTIFIER && !isAlias) {
                            paramType = parse_type(p, default_bp);
                            if (paramType == nullptr) paramType = new ast::SymbolType(L"auto");
                        }
                        paramName = p->advance().value;
                    } else {
                        wstring message = L"Expected parameter name in method '" + propertyName + L"' declaration at " + p->position() + L", but got " + lexer::TokenKindString(p->currentTokenKind());
                        p->errors.push_back(ParserError(message));
                        _wcout << (CompilerOptions.debug ? L"[Parser] " : L"") << message << endl;
                        if (CompilerOptions.panic) {
                            if (CompilerOptions.debug) _wcout << L"[Parser] Panicing" << endl;
                            exit(1);
                        }
                        p->advanceUntil(lexer::CLOSE_PAREN, lexer::COMMA);
                        continue;
                    }

                    if (p->currentTokenKind() == lexer::COLON) {
                        p->advance();  // consume ':'
                        if (paramType != nullptr || isAlias) {
                            ast::Type* newParamType = parse_type(p, default_bp);
                            wstring message = L"Method parameter '" + paramName + L"' in method '" + propertyName + L"' declaration at " + p->position() + L" cannot have an explicit type as it was already specified as '" + (paramType == nullptr ? L"alias" : paramType->GetName()) + L"'";
                            p->errors.push_back(ParserError(message));
                            _wcout << (CompilerOptions.debug ? L"[Parser] " : L"") << message << endl;
                            if (CompilerOptions.panic) {
                                if (CompilerOptions.debug) _wcout << L"[Parser] Panicing" << endl;
                                exit(1);
                            }
                            p->advance();  // consume the type
                        } else paramType = parse_type(p, default_bp);
                    }

                    if (p->currentTokenKind() == lexer::ASSIGNMENT) {
                        p->advance();  // consume '='
                        defaultValue = parse_expr(p, assignment);
                    }

                    if (parameters.find(paramName) != parameters.end()) {
                        wstring message = L"Duplicate parameter name '" + paramName + L"' found in method '" + propertyName + L"' declaration at " + p->position();
                        p->errors.push_back(ParserError(message));
                        _wcout << (CompilerOptions.debug ? L"[Parser] " : L"") << message << endl;
                        if (CompilerOptions.panic) {
                            if (CompilerOptions.debug) _wcout << L"[Parser] Panicing" << endl;
                            exit(1);
                        }
                        p->advanceUntil(lexer::COMMA);  // Skip until the end of the parameter declaration
                        continue;
                    }

                    if (defaultValue == nullptr) defaultValue = new ast::NullExpr();
                    if (paramType == nullptr) paramType = new ast::SymbolType(isConstant ? L"auto" : L"any");

                    parameters[paramName] = new ast::MethodParameter(
                        paramName, 
                        paramType, 
                        defaultValue, 
                        isConstant,
                        isAlias
                    );
                    if (p->currentTokenKind() == lexer::COMMA) p->advance();
                }
                p->expect(lexer::CLOSE_PAREN);
                if (p->currentTokenKind() == lexer::COLON) {
                    p->advance();  // consume ':'
                    if (ExpectedType != nullptr) {
                        ast::Type* newExpectedType = parse_type(p, default_bp);
                        wstring message = L"Struct method declaration for '" + propertyName + L"' at " + p->position() + L" cannot have an explicit return type as it was already specified as '" + ExpectedType->GetName() + L"'";
                        p->errors.push_back(ParserError(message));
                        _wcout << (CompilerOptions.debug ? L"[Parser] " : L"") << message << endl;
                        if (CompilerOptions.panic) {
                            if (CompilerOptions.debug) _wcout << L"[Parser] Panicing" << endl;
                            exit(1);
                        }
                        p->advance();  // consume the type
                    } else ExpectedType = parse_type(p, default_bp);
                }
                if (p->currentTokenKind() == lexer::ARROW) {  // fn <type> name() => <expr>;
                    p->advance();  // consume '=>'
                    if (p->currentTokenKind() == lexer::OPEN_CURLY) {
                        MethodBody = parse_block_stmt(p);
                    } else {
                        MethodBody = new ast:: ExpressionStmt(new ast::ReturnExpr(parse_expr(p, assignment)));
                        p->expect(lexer::SEMICOLON);
                    }
                } else MethodBody = parse_block_stmt(p);
            } else {  // Parse property
                if (p->currentTokenKind() == lexer::ASSIGNMENT) {
                    p->advance();  // consume '='
                    AssignedValue = parse_expr(p, assignment);
                } else AssignedValue = new ast::NullExpr();
            }

            if (isMethod) {
                if (ExpectedType == nullptr) ExpectedType = new ast::SymbolType(L"auto");
                methods[propertyName] = new ast::StructMethod(
                    propertyName,
                    ExpectedType,
                    MethodBody,
                    isStatic,
                    accessModifier,
                    parameters,
                    isConstant
                );
                if (p->currentTokenKind() == lexer::SEMICOLON) p->advance();  // Optional semicolon after method declaration
            } else {
                properties[propertyName] = new ast::StructProperty(
                    propertyName,
                    ExpectedType,
                    AssignedValue,
                    isStatic,
                    accessModifier,
                    isConstant
                );
                p->expect(lexer::SEMICOLON);
            }
        }

        p->expect(lexer::CLOSE_CURLY);
        if (p->currentTokenKind() == lexer::SEMICOLON) p->advance();  // Optional semicolon after struct declaration

        return new ast::StructDeclStmt(
            structName,
            reverseUnorderedMap(properties),  // Properties
            reverseUnorderedMap(methods)   // Methods
        );
    }
    vector<ast::MethodParameter*> parse_func_parameters(Parser* p, wstring funcName) {
        p->expect(lexer::OPEN_PAREN);  // consume '('
        vector<ast::MethodParameter*> parameters;
        const wstring defaultParameterType = L"auto";
        while (p->hasTokens() && p->currentTokenKind() != lexer::CLOSE_PAREN)
        {
            wstring paramName = L"UnnamedParam";
            ast::Type* paramType = nullptr;  // may also be 'auto' and 'any'
            ast::Expr* defaultValue = nullptr;
            bool isConstant = false;
            if (p->currentTokenKind() == lexer::CONST) {
                isConstant = true;
                p->advance();  // consume 'const'
            }
            bool isAlias = p->currentTokenKind() == lexer::ALIAS;
            if (isAlias) p->advance();
            if (p->currentToken().isTypeName()) {
                if (p->nextTokenKind() == lexer::IDENTIFIER && !isAlias) {
                    paramType = parse_type(p, default_bp);
                    if (paramType == nullptr) paramType = new ast::SymbolType(defaultParameterType);
                }
                paramName = p->advance().value;
            } else {
                wstring message = L"Expected parameter name in function '" + funcName + L"' declaration at " + p->position() + L", but got " + lexer::TokenKindString(p->currentTokenKind());
                p->errors.push_back(ParserError(message));
                _wcout << (CompilerOptions.debug ? L"[Parser] " : L"") << message << endl;
                if (CompilerOptions.panic) {
                    if (CompilerOptions.debug) _wcout << L"[Parser] Panicing" << endl;
                    exit(1);
                }
                p->advanceUntil(lexer::CLOSE_PAREN, lexer::COMMA, lexer::SEMICOLON);
                continue;
            }

            if (p->currentTokenKind() == lexer::COLON) {
                p->advance();  // consume ':'
                if (paramType != nullptr || isAlias) {
                    ast::Type* newParamType = parse_type(p, default_bp);
                    wstring message = L"Function parameter '" + paramName + L"' in function '" + funcName + L"' declaration at " + p->position() + L" cannot have an explicit type as it was already specified as '" + (paramType == nullptr ? L"alias" : paramType->GetName()) + L"'";
                    p->errors.push_back(ParserError(message));
                    _wcout << (CompilerOptions.debug ? L"[Parser] " : L"") << message << endl;
                    if (CompilerOptions.panic) {
                        if (CompilerOptions.debug) _wcout << L"[Parser] Panicing" << endl;
                        exit(1);
                    }
                    p->advance();  // consume the type
                } else paramType = parse_type(p, default_bp);
                if (paramType == nullptr) paramType = new ast::SymbolType(defaultParameterType);
            }
            if (p->currentTokenKind() == lexer::ASSIGNMENT) {
                p->advance();  // consume '='
                defaultValue = parse_expr(p, assignment);
            }
            parameters.push_back(new ast::MethodParameter(
                paramName, 
                paramType != nullptr ? paramType : new ast::SymbolType(defaultParameterType), 
                defaultValue, 
                isConstant,
                isAlias
            ));
            if (p->currentTokenKind() == lexer::COLON) {
                p->advance();  // consume ':'
                if (paramType != nullptr) {
                    ast::Type* newParamType = parse_type(p, default_bp);
                    wstring message = L"Function parameter '" + paramName + L"' in function '" + funcName + L"' declaration at " + p->position() + L" cannot have an explicit type as it was already specified as '" + paramType->GetName() + L"'";
                    p->errors.push_back(ParserError(message));
                    _wcout << (CompilerOptions.debug ? L"[Parser] " : L"") << message << endl;
                    if (CompilerOptions.panic) {
                        if (CompilerOptions.debug) _wcout << L"[Parser] Panicing" << endl;
                        exit(1);
                    }
                    p->advance();  // consume the type
                } else parameters.back()->ParamType = parse_type(p, default_bp);
            }
            if (p->currentToken().isOneOfMany(lexer::COMMA, lexer::SEMICOLON)) p->advance();
            else if (p->currentTokenKind() != lexer::CLOSE_PAREN) {
                wstring message = L"Expected token after parameter declaration for '" + paramName + L"' in function '" + funcName + L"' at " + p->position() + L" to be either a semicolon ';' or a comma ',' to separate parameters, but got " + lexer::TokenKindString(p->currentTokenKind());
                p->errors.push_back(ParserError(message));
                _wcout << (CompilerOptions.debug ? L"[Parser] " : L"") << message << endl;
                if (CompilerOptions.panic) {
                    if (CompilerOptions.debug) _wcout << L"[Parser] Panicing" << endl;
                    exit(1);
                }
                p->advanceUntil(lexer::COMMA, lexer::SEMICOLON, lexer::CLOSE_PAREN);
                if (p->currentTokenKind() != lexer::CLOSE_PAREN) p->advance();
            }
        }
        p->expect(lexer::CLOSE_PAREN);  // consume ')'
        return parameters;
    }
    ast::Stmt* parse_func_decl_stmt(Parser* p) {
        if (CompilerOptions.verbose) _wcout << L"Parsing function declaration statement at " << p->position() << endl;
        const ast::FunctionLining functionLining = p->currentToken().kind == lexer::INLINE ? ast::FLInline : (p->currentToken().kind == lexer::OUTLINE ? ast::FLOutline : ast::FLAutomatic);
        if (functionLining != ast::FLAutomatic) p->advance();  // consume 'inline' or 'outline'
        if (p->currentToken().isOneOfMany(lexer::INLINE, lexer::OUTLINE)) {
            wstring message = L"Function declaration at " + p->position() + L" has multiple function lining specifiers, only one is allowed";
            p->errors.push_back(ParserError(message));
            _wcout << (CompilerOptions.debug ? L"[Parser] " : L"") << message << endl;
            if (CompilerOptions.panic) {
                if (CompilerOptions.debug) _wcout << L"[Parser] Panicing" << endl;
                exit(1);
            }
            while (p->currentToken().isOneOfMany(lexer::INLINE, lexer::OUTLINE)) p->advance();  // consume any extra 'inline' or 'outline' keywords
        }
        if (p->currentTokenKind() == lexer::FN) p->advance();  // consume 'fn' if it exists, but allow it to be optional for better ergonomics
        else if (functionLining != ast::FLAutomatic) {
            wstring message = L"Expected 'fn' keyword after function lining specifier '" + (functionLining == ast::FLInline ? wstring(L"'inline'") : wstring(L"'outline'")) + L"' at " + p->position() + L", but got " + lexer::TokenKindString(p->currentTokenKind());
            p->errors.push_back(ParserError(message));
            _wcout << (CompilerOptions.debug ? L"[Parser] " : L"") << message << endl;
            if (CompilerOptions.panic) {
                if (CompilerOptions.debug) _wcout << L"[Parser] Panicing" << endl;
                exit(1);
            }
            // If function name, try ignoring 'fn' keyword
            if (p->currentTokenKind() != lexer::IDENTIFIER) {
                p->advanceUntil(lexer::SEMICOLON);
                p->expect(lexer::SEMICOLON);
                return nullptr;
            }
        }
        const wstring funcName = p->advance().value;  // consume function name

        // Expect '('
        if (p->currentTokenKind() != lexer::OPEN_PAREN) {
            wstring message = L"Expected '(' after function name '" + funcName + L"' at " + p->position() + L", but got " + lexer::TokenKindString(p->currentTokenKind());
            p->errors.push_back(ParserError(message));
            _wcout << (CompilerOptions.debug ? L"[Parser] " : L"") << message << endl;
            if (CompilerOptions.panic) {
                if (CompilerOptions.debug) _wcout << L"[Parser] Panicing" << endl;
                exit(1);
            }
            p->advanceUntil(lexer::SEMICOLON);
            p->expect(lexer::SEMICOLON);
            return nullptr;
        }
        vector<ast::MethodParameter*> parameters = parse_func_parameters(p, funcName);

        ast::Type* returnType = nullptr;
        if (p->currentTokenKind() == lexer::COLON) {
            p->advance();  // consume ':'
            returnType = parse_type(p, default_bp);
            if (returnType == nullptr) returnType = new ast::SymbolType(L"auto");  // Default function return type
        } else returnType = new ast::SymbolType(L"auto");

        ast::Stmt* body = nullptr;
        if (p->currentTokenKind() == lexer::ARROW) {  // [inline|outline] fn name(): <type> => <expr>;
            p->advance();  // consume '=>'
            body = new ast::ExpressionStmt(new ast::ReturnExpr(parse_expr(p, assignment)));
            if (p->currentTokenKind() != lexer::SEMICOLON) {
                const wstring message = L"Expected expression after arrow ('=>') function declaration at " + p->position() + L" to be followed by a semicolon ';', but got " + lexer::TokenKindString(p->currentTokenKind());
                p->errors.push_back(ParserError(message));
                _wcout << (CompilerOptions.debug ? L"[Parser] " : L"") << message << endl;
                if (CompilerOptions.panic) {
                    if (CompilerOptions.debug) _wcout << L"[Parser] Panicing" << endl;
                    exit(1);
                }
                p->advanceUntil(lexer::SEMICOLON);
            }
            p->expect(lexer::SEMICOLON);
        } else body = parse_block_stmt(p);

        return new ast::FuncDeclStmt(
            funcName,
            returnType,
            body,
            functionLining,
            parameters
        );
    }
    ast::Stmt* parse_type_change_stmt(Parser* p) {
        if (CompilerOptions.verbose) _wcout << L"Parsing type change statement at " << p->position() << endl;
        p->advance();  // consume 'typeof'
        // Get varname
        wstring varName;
        if (p->currentTokenKind() == lexer::IDENTIFIER) {
            varName = p->advance().value;
        } else {
            wstring message = L"Expected variable name after 'typeof' keyword at " + p->position() + L", but got " + lexer::TokenKindString(p->currentTokenKind());
            p->errors.push_back(ParserError(message));
            _wcout << (CompilerOptions.debug ? L"[Parser] " : L"") << message << endl;
            if (CompilerOptions.panic) {
                if (CompilerOptions.debug) _wcout << L"[Parser] Panicing" << endl;
                exit(1);
            }
            p->advanceUntil(lexer::SEMICOLON);
            p->advance();  // consume ';'
            return nullptr;
        }

        // Assignment
        if (p->currentTokenKind() != lexer::ASSIGNMENT) {
            wstring message = L"Expected assignment operator '=' after variable name '" + varName + L"' in type change statement at " + p->position() + L", but got " + lexer::TokenKindString(p->currentTokenKind());
            p->errors.push_back(ParserError(message));
            _wcout << (CompilerOptions.debug ? L"[Parser] " : L"") << message << endl;
            if (CompilerOptions.panic) {
                if (CompilerOptions.debug) _wcout << L"[Parser] Panicing" << endl;
                exit(1);
            }
            p->advanceUntil(lexer::SEMICOLON);
            p->advance();  // consume ';'
            return nullptr;
        }
        p->advance();  // consume '='
        ast::Expr* newTypeExpr = nullptr;
        if (p->currentTokenKind() == lexer::TYPEOF) {
            p->advance();  // consume 'typeof'
            p->expect(lexer::OPEN_PAREN);
            newTypeExpr = parse_expr(p, default_bp);
            p->expect(lexer::CLOSE_PAREN);
        } else {
            if (p->currentToken().isTypeName()) {
                newTypeExpr = new ast::TypeExpr(parse_type(p, default_bp));
            } else {
                wstring message = L"Expected type expression after '=' in type change statement for variable '" + varName + L"' at " + p->position() + L", but got " + lexer::TokenKindString(p->currentTokenKind());
                p->errors.push_back(ParserError(message));
                _wcout << (CompilerOptions.debug ? L"[Parser] " : L"") << message << endl;
                if (CompilerOptions.panic) {
                    if (CompilerOptions.debug) _wcout << L"[Parser] Panicing" << endl;
                    exit(1);
                }
                p->advanceUntil(lexer::SEMICOLON);
                p->advance();  // consume ';'
                return nullptr;
            }
        }
        p->expect(lexer::SEMICOLON);
        return new ast::TypeChangeStmt(varName, newTypeExpr);
    }
    ast::Stmt* parse_if_stmt(Parser* p) {
        if (CompilerOptions.verbose) _wcout << L"Parsing if statement at " << p->position() << endl;
        p->expect(lexer::IF, lexer::ELIF);
        ast::Expr* condition = nullptr;
        if (p->currentTokenKind() == lexer::OPEN_PAREN) {
            p->expect(lexer::OPEN_PAREN);
            condition = parse_expr(p, default_bp);
            p->expect(lexer::CLOSE_PAREN);
        } else condition = new ast::BooleanExpr(true);
        if (p->currentTokenKind() == lexer::SEMICOLON) {
            wstring message = L"Expected 'if' statement at " + p->position() + L" to have a condition expression in parentheses, but got ';'";
            p->errors.push_back(ParserError(message));
            _wcout << (CompilerOptions.debug ? L"[Parser] " : L"") << message << endl;
            if (CompilerOptions.panic) {
                if (CompilerOptions.debug) _wcout << L"[Parser] Panicing" << endl;
                exit(1);
            }
            p->advance();
            return nullptr;
        }
        ast::Stmt* thenBranch = parse_stmt(p);
        ast::Stmt* elseBranch = nullptr;
        if (p->currentToken().isOneOfMany(lexer::ELSE, lexer::ELIF)) {
            if (p->currentTokenKind() == lexer::ELSE) p->advance();  // consume 'else'
            if (p->currentToken().isOneOfMany(lexer::IF, lexer::ELIF)) elseBranch = parse_if_stmt(p);  // else if
            else {
                elseBranch = parse_stmt(p);
                if (elseBranch == nullptr) {
                    wstring message = L"Expected 'else' branch for if statement at " + p->position() + L" to be a statement, but got " + lexer::TokenKindString(p->currentTokenKind());
                    p->errors.push_back(ParserError(message));
                    _wcout << (CompilerOptions.debug ? L"[Parser] " : L"") << message << endl;
                    if (CompilerOptions.panic) {
                        if (CompilerOptions.debug) _wcout << L"[Parser] Panicing" << endl;
                        exit(1);
                    }
                    p->advanceUntil(lexer::OPEN_CURLY, lexer::SEMICOLON);
                    if (p->currentTokenKind() != lexer::SEMICOLON) p->advanceDepth(lexer::OPEN_CURLY, lexer::CLOSE_CURLY);
                    p->advance();
                }
            }
        }
        return new ast::IfStmt(condition, thenBranch, elseBranch);
    }
    ast::Stmt* parse_while_stmt(Parser* p) {
        if (CompilerOptions.verbose) _wcout << L"Parsing while statement at " << p->position() << endl;
        p->expect(lexer::WHILE);
        ast::Expr* condition = nullptr;
        if (p->currentTokenKind() == lexer::OPEN_PAREN) {
            p->expect(lexer::OPEN_PAREN);
            condition = parse_expr(p, default_bp);
            p->expect(lexer::CLOSE_PAREN);
        }
        if (p->currentTokenKind() == lexer::SEMICOLON) {
            wstring message = L"Expected 'while' statement at " + p->position() + L" to have a condition expression in parentheses, but got ';'";
            p->errors.push_back(ParserError(message));
            _wcout << (CompilerOptions.debug ? L"[Parser] " : L"") << message << endl;
            if (CompilerOptions.panic) {
                if (CompilerOptions.debug) _wcout << L"[Parser] Panicing" << endl;
                exit(1);
            }
            p->advance();
            return nullptr;
        }
        if (condition == nullptr) condition = new ast::BooleanExpr(true);
        ast::Stmt* body = parse_stmt(p);
        ast::Stmt* elseBranch = nullptr;
        if (p->currentTokenKind() == lexer::ELIF) {
            elseBranch = parse_if_stmt(p);
        } else if (p->currentTokenKind() == lexer::ELSE) {
            p->advance();  // consume 'else'
            if (p->currentTokenKind() == lexer::IF) elseBranch = parse_if_stmt(p);  // else if
            else {
                elseBranch = parse_stmt(p);
                if (elseBranch == nullptr) {
                    wstring message = L"Expected 'else' branch for while statement at " + p->position() + L" to be a statement, but got " + lexer::TokenKindString(p->currentTokenKind());
                    p->errors.push_back(ParserError(message));
                    _wcout << (CompilerOptions.debug ? L"[Parser] " : L"") << message << endl;
                    if (CompilerOptions.panic) {
                        if (CompilerOptions.debug) _wcout << L"[Parser] Panicing" << endl;
                        exit(1);
                    }
                    p->advanceUntil(lexer::OPEN_CURLY, lexer::SEMICOLON);
                    if (p->currentTokenKind() != lexer::SEMICOLON) p->advanceDepth(lexer::OPEN_CURLY, lexer::CLOSE_CURLY);
                    p->advance();
                }
            }
        }
        if (p->currentTokenKind() == lexer::SEMICOLON) p->advance();  // Optional semicolon after while statement
        return new ast::WhileStmt(condition, body, elseBranch);
    }
    ast::Stmt* skip_stmt(Parser* p) {
        p->advance();  // consume the token that indicates the start of the statement
        p->advanceUntil(lexer::SEMICOLON);  // Skip until the end of the statement
        if (p->currentTokenKind() == lexer::SEMICOLON) p->advance();  // consume ';'
        return nullptr;
    }
    ast::Stmt* parse_import_stmt(Parser* p) {
        p->advance();  // consume 'import'
        bool isGlobal = p->currentTokenKind() == lexer::LESS;
        if (isGlobal) p->advance();  // consume '<'
        wstring moduleName;
        if (isGlobal) {
            if (p->currentTokenKind() != lexer::IDENTIFIER) {
                wstring message = L"Expected identifier for module name in relative import at " + p->position() + L", but got " + lexer::TokenKindString(p->currentTokenKind());
                p->errors.push_back(ParserError(message));
                _wcout << (CompilerOptions.debug ? L"[Parser] " : L"") << message << endl;
                if (CompilerOptions.panic) {
                    if (CompilerOptions.debug) _wcout << L"[Parser] Panicing" << endl;
                    exit(1);
                }
                return nullptr;
            }
            moduleName = p->currentToken().value;
            p->advance();
            if (p->currentTokenKind() == lexer::GREATER) p->advance();  // consume '>' for relative import
            else {
                wstring message = L"Expected '>' for relative import at " + p->position() + L", but got " + lexer::TokenKindString(p->currentTokenKind());
                p->errors.push_back(ParserError(message));
                _wcout << (CompilerOptions.debug ? L"[Parser] " : L"") << message << endl;
                if (CompilerOptions.panic) {
                    if (CompilerOptions.debug) _wcout << L"[Parser] Panicing" << endl;
                    exit(1);
                }
                return nullptr;
            }
        } else {
            ast::StringExpr* stringExpr = static_cast<ast::StringExpr*>(parse_expr(p, binding_power::default_bp));
            if (stringExpr == nullptr) {
                wstring message = L"Expected module name for import statement at " + p->position() + L", but got " + lexer::TokenKindString(p->currentTokenKind());
                p->errors.push_back(ParserError(message));
                _wcout << (CompilerOptions.debug ? L"[Parser] " : L"") << message << endl;
                if (CompilerOptions.panic) {
                    if (CompilerOptions.debug) _wcout << L"[Parser] Panicing" << endl;
                    exit(1);
                }
                return nullptr;
            }
            moduleName = stringExpr->value;
        }

        wstring namespaceName;
        if (p->currentTokenKind() == lexer::AS) {
            p->advance();  // consume 'as'
            if (p->currentToken().isOneOfMany(lexer::IDENTIFIER)) {
                namespaceName = p->currentToken().value;
                p->advance();
            } else {
                wstring message = L"Expected identifier for namespace alias at " + p->position() + L", but got " + lexer::TokenKindString(p->currentTokenKind());
                p->errors.push_back(ParserError(message));
                _wcout << (CompilerOptions.debug ? L"[Parser] " : L"") << message << endl;
                if (CompilerOptions.panic) {
                    if (CompilerOptions.debug) _wcout << L"[Parser] Panicing" << endl;
                    exit(1);
                }
            }
        }
        if (p->currentTokenKind() == lexer::SEMICOLON) p->advance();  // Optional semicolon after import statement
        
        if (!filesystem::exists(moduleName)) {
            // TODO: add error handling
            throw new runtime_error(string(moduleName.begin(), moduleName.end()) + " does not exist");
        }

        // TODO: Handle error
        string narrowInputFilename(moduleName.begin(), moduleName.end());
        ifstream inputFile(narrowInputFilename, ios::binary);
        if (!inputFile.is_open()) {
            _wcout << L"Error: Unable to open input file." << endl;
            return nullptr;
        }

        stringstream buffer;
        buffer << inputFile.rdbuf();
        string content = buffer.str();
        inputFile.close();

        // Check for and remove UTF-8 BOM (0xEF, 0xBB, 0xBF)
        if (content.size() >= 3 && 
            (unsigned char)content[0] == 0xEF && 
            (unsigned char)content[1] == 0xBB && 
            (unsigned char)content[2] == 0xBF) {
            content.erase(0, 3);
        }
        wstring fileContent;
        if (!content.empty()) {
            int size_needed = MultiByteToWideChar(CP_UTF8, 0, content.c_str(), (int)content.size(), NULL, 0);
            fileContent.resize(size_needed);
            MultiByteToWideChar(CP_UTF8, 0, content.c_str(), (int)content.size(), &fileContent[0], size_needed);
        }

        if (CompilerOptions.debug) {
            _wcout << L"Tokenizing import: " << moduleName << endl;
        }
        vector<lexer::Token> tokens = lexer::Tokenize(fileContent);
        if (CompilerOptions.debug) {
            _wcout << L"AST conversion of: " << moduleName << endl;
        }
        ast::BlockStmt* ast = parser::Parse_(tokens);
        ast->isScopeless = true;
        if (CompilerOptions.debug) {
            _wcout << L"Import done: " << moduleName << endl;
        }
        
        return ast;
    }
}