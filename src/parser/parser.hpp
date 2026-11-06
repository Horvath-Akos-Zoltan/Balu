#pragma once

namespace parser {
    ast::Stmt* parse_stmt(Parser* p);
    
    // Returns a new parser instance with the given tokens (free manually)
    Parser* createParser(const vector<lexer::Token>& tokens) {
        createTokenLookups();
        createTokenTypeLookups();
        Parser* p = new Parser{ {}, tokens, 0 };
        lexer::Token current = p->currentToken();
        p->line = current.line;
        p->column = current.column;
        return p;
    }

    ast::BlockStmt Parse(const vector<lexer::Token>& tokens) {
        vector<ast::Stmt*> Body;
        
        unique_ptr<Parser> p(createParser(tokens));

        while (p->hasTokens())
        {
            ast::Stmt* stmt = parse_stmt(p.get());
            if (stmt) { // Only add valid statements
                Body.push_back(stmt);
            }
        }

        ast::BlockStmt block = ast::BlockStmt(Body);
        if (CompilerOptions.allowOptimization) {
            ast::Stmt* root = &block;
            ast::optimizer::Optimize(root);
        }
        return block;
    }
    ast::BlockStmt* Parse_(const vector<lexer::Token>& tokens) {
        vector<ast::Stmt*> Body;
        
        unique_ptr<Parser> p(createParser(tokens));

        while (p->hasTokens())
        {
            ast::Stmt* stmt = parse_stmt(p.get());
            if (stmt) { // Only add valid statements
                Body.push_back(stmt);
            }
        }

        ast::BlockStmt* block = new ast::BlockStmt(Body);
        if (CompilerOptions.allowOptimization) {
            ast::Stmt* root = block;
            ast::optimizer::Optimize(root);
        }
        return block;
    }
}