#pragma once

#include <string>
#include <sstream>
#include <iostream>

namespace parser {
    ast::BlockStmt* Parse_(const vector<lexer::Token>& tokens);
    struct ParserError : public Error {
        ParserError(const wstring& message, unsigned long long line, unsigned long long column) : Error(message, line, column) {}
        ParserError(const wstring& message) : Error(message) {}
        ParserError() : Error() {}
    };
    struct Parser {
        vector<ParserError> errors;
        vector<lexer::Token> tokens;
        int pos;
        unsigned long long line, column;
        lexer::Token currentToken() const {
            if (pos >= (int)tokens.size()) return lexer::Token{ lexer::EOF_TOKEN, L"EOF" };
            return tokens[pos];
        }
        lexer::TokenKind currentTokenKind() const {
            return currentToken().kind;
        }
        lexer::Token nextToken(int offset = 1) const {
            if (pos + offset >= (int)tokens.size()) return lexer::Token{ lexer::EOF_TOKEN, L"EOF" };
            return tokens[pos + offset];
        }
        lexer::TokenKind nextTokenKind(int offset = 1) const {
            return nextToken(offset).kind;
        }
        lexer::Token previousToken() const {
            if (pos <= 0) return NewToken(lexer::EOF_TOKEN, L"EOF");
            return tokens[pos - 1];
        }
        lexer::TokenKind previousTokenKind() const {
            return previousToken().kind;
        }
        lexer::Token advance() {
            lexer::Token token = currentToken();
            pos++;
            lexer::Token next = currentToken();
            line = next.line;
            column = next.column;
            return token;
        }
        lexer::Token advanceBack() {
            pos--;
            lexer::Token current = currentToken();
            line = current.line;
            column = current.column;
            return current;
        }
        template<typename... TokenKinds>
        lexer::Token advanceUntil(TokenKinds... kinds) {
            lexer::Token token = currentToken();
            while (!token.isOneOfMany(kinds...) && token.kind != lexer::EOF_TOKEN) {
                advance();
                token = currentToken();
            }
            return token;
        }
        lexer::Token advanceDepth(lexer::TokenKind positive, lexer::TokenKind negative) {
            int depth = 0;
            lexer::Token token = currentToken();
            while (token.kind != lexer::EOF_TOKEN) {
                if (token.kind == positive) depth++;
                else if (token.kind == negative) depth--;
                if (depth <= 0) break;
                advance();
                token = currentToken();
            }
            return token;
        }
        bool hasTokens() const {
            return pos < tokens.size() && currentToken().kind != lexer::EOF_TOKEN;
        }
        lexer::Token expectError(lexer::TokenKind expectedKind, Error error) {
            lexer::Token token = currentToken();
            lexer::TokenKind kind = token.kind;
            if (kind != expectedKind) {
                if (error.isNull()) {
                    wstring message = L"Expected token at " + position() + L" to be " + lexer::TokenKindString(expectedKind) + L" but found " + lexer::TokenKindString(kind);
                    error = Error(message);
                    _wcout << (CompilerOptions.debug ? L"[Parser] " : L"") << message << endl;
                } else {
                    _wcout << (CompilerOptions.debug ? L"[Parser] " : L"") << error.message << endl;
                }
                errors.push_back(ParserError(error.message, token.line, token.column));
                if (CompilerOptions.panic) {
                    if (CompilerOptions.debug) _wcout << L"[Parser] Panicing" << endl;
                    exit(1);
                }
            }
            return advance();
        }
        template<typename... TokenKinds>
        lexer::Token expect(TokenKinds... expectedKinds) {
            lexer::Token token = currentToken();
            lexer::TokenKind kind = token.kind;
            for (lexer::TokenKind expectedKind : {expectedKinds...}) {
                if (kind == expectedKind) {
                    return advance();
                }
            }
            wstring expectedKindsStr;
            for (lexer::TokenKind expectedKind : {expectedKinds...}) {
                expectedKindsStr += lexer::TokenKindString(expectedKind) + L", ";
            }
            expectedKindsStr = expectedKindsStr.substr(0, expectedKindsStr.length() - 2); // Remove trailing comma and space
            wstring message = L"Expected one of [" + expectedKindsStr + L"] but found " + lexer::TokenKindString(kind) + L" at " + position();
            _wcout << (CompilerOptions.debug ? L"[Parser] " : L"") << message << endl;
            errors.push_back(ParserError(message, token.line, token.column));
            if (CompilerOptions.panic) {
                if (CompilerOptions.debug) _wcout << L"[Parser] Panicing" << endl;
                exit(1);
            }
            return advance();
        }
        lexer::Token expect(lexer::TokenKind expectedKind) {
            return expectError(expectedKind, Error());
        }
        wstring position() const {
            wstringstream ss;
            ss << L'[' << line << L':' << column << L']';
            return ss.str();
        }
        // Exact same as `expect`, but the token is guaranteed to match
        // Should be used when certain about the token
        lexer::Token advanceOver(lexer::TokenKind kind) {
#if UNNECESSARY_PANIC_CHECK
            lexer::Token token = currentToken();
            if (token.kind != kind) {
                wstring message = L"Expected token at " + position() + L" to be " + lexer::TokenKindString(kind) + L" but found " + lexer::TokenKindString(token.kind);
                _wcout << (CompilerOptions.debug ? L"[Parser] " : L"") << message << endl;
                errors.push_back(ParserError(message, token.line, token.column));
                if (CompilerOptions.panic) {
                    if (CompilerOptions.debug) _wcout << L"[Parser] Panicing" << endl;
                    exit(1);
                }
            }
#endif
            return advance();
        }
    };
}

#include "syntax.hpp"
#include "lookup_pre.hpp"
#include "expr.hpp"
#include "stmt.hpp"
#include "types.hpp"
#include "lookup.hpp"
#include "parser.hpp"