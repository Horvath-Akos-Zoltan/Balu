#pragma once

namespace parser {
    void createTokenLookups() {
        // Assignment
        led(lexer::ASSIGNMENT, assignment, parse_assignment_expr);
        led(lexer::PLUS_EQUALS, assignment, parse_assignment_expr);
        led(lexer::MINUS_EQUALS, assignment, parse_assignment_expr);
        led(lexer::STAR_EQUALS, assignment, parse_assignment_expr);
        led(lexer::SLASH_EQUALS, assignment, parse_assignment_expr);
        led(lexer::PERCENT_EQUALS, assignment, parse_assignment_expr);

        // Logical
        led(lexer::AND, logical, parse_binary_expr);
        led(lexer::OR, logical, parse_binary_expr);
        led(lexer::DOT_DOT, logical, parse_binary_expr);

        // Bitwise
        led(lexer::BITWISE_AND, logical, parse_binary_expr);
        led(lexer::BITWISE_OR, logical, parse_binary_expr);
        led(lexer::BITWISE_XOR, logical, parse_binary_expr);
        nud(lexer::BITWISE_NOT, parse_prefix_expr);
        led(lexer::BITWISE_LEFT, logical, parse_binary_expr);
        led(lexer::BITWISE_RIGHT, logical, parse_binary_expr);

        // Relational
        led(lexer::LESS, relational, parse_binary_expr);
        led(lexer::LESS_EQUALS, relational, parse_binary_expr);
        led(lexer::GREATER, relational, parse_binary_expr);
        led(lexer::GREATER_EQUALS, relational, parse_binary_expr);
        led(lexer::EQUALS, relational, parse_binary_expr);
        led(lexer::NOT_EQUALS, relational, parse_binary_expr);

        // Additive & Multiplicative
        led(lexer::PLUS, additive, parse_binary_expr);
        led(lexer::DASH, additive, parse_binary_expr);
        
        led(lexer::STAR, multiplicative, parse_binary_expr);
        led(lexer::SLASH, multiplicative, parse_binary_expr);
        led(lexer::PERCENT, multiplicative, parse_binary_expr);

        // Call/Member/Array expressions
        led(lexer::OPEN_CURLY, call, parse_struct_instantiation_expr);
        led(lexer::OPEN_PAREN, call, parse_function_call_expr);
        led(lexer::DOT, member, parse_binary_expr);
        nud(lexer::OPEN_BRACKET, parse_array_instantiation_expr);

        // Literals and Symbols
        nud(lexer::NUMBER, parse_primary_expr);
        nud(lexer::PNUMBER, parse_primary_expr);
        nud(lexer::BYTE, parse_primary_expr);
        nud(lexer::SBYTE, parse_primary_expr);
        nud(lexer::SHORT, parse_primary_expr);
        nud(lexer::USHORT, parse_primary_expr);
        nud(lexer::INT24, parse_primary_expr);
        nud(lexer::UINT24, parse_primary_expr);
        nud(lexer::INT32, parse_primary_expr);
        nud(lexer::UINT32, parse_primary_expr);
        nud(lexer::INT64, parse_primary_expr);
        nud(lexer::UINT64, parse_primary_expr);
        nud(lexer::STRING8, parse_primary_expr);
        nud(lexer::STRING16, parse_primary_expr);
        nud(lexer::STRING32, parse_primary_expr);
        nud(lexer::FSTRING, parse_primary_expr);
        nud(lexer::CHAR8, parse_primary_expr);
        nud(lexer::CHAR16, parse_primary_expr);
        nud(lexer::CHAR32, parse_primary_expr);
        nud(lexer::BOOL, parse_primary_expr);
        nud(lexer::AUTO, parse_primary_expr);
        nud(lexer::ANY, parse_primary_expr);
        nud(lexer::VOID, parse_primary_expr);
        nud(lexer::NULL_, parse_primary_expr);
        nud(lexer::IDENTIFIER, parse_primary_expr);
        nud(lexer::OPEN_PAREN, parse_grouping_expr);
        nud(lexer::DASH, parse_prefix_expr);
        nud(lexer::PLUS, parse_prefix_expr);
        nud(lexer::NOT, parse_prefix_expr);

        nud(lexer::RULE, parse_primary_expr);
        nud(lexer::RETURN, parse_return_expr);

        nud(lexer::BREAK, parse_break_expr);
        
        nud(lexer::TYPEOF, parse_typeof_expr);

        // Volatile expression
        nud(lexer::VOLATILE, parse_volatile_expr);

        // Statements
        stmt(lexer::LET, parse_var_decl_stmt);
        stmt(lexer::CONST, parse_var_decl_stmt);
        stmt(lexer::MUT, parse_var_decl_stmt);
        stmt(lexer::ALIAS, parse_alias_decl_stmt);
        stmt(lexer::OPEN_BRACKET, parse_var_decl_stmt);
        stmt(lexer::STRUCT, parse_struct_decl_stmt);
        stmt(lexer::IF, parse_if_stmt);
        stmt(lexer::WHILE, parse_while_stmt);
        stmt(lexer::INLINE, parse_func_decl_stmt);
        stmt(lexer::OUTLINE, parse_func_decl_stmt);
        stmt(lexer::FN, parse_func_decl_stmt);
        stmt(lexer::OPEN_CURLY, parse_block_stmt);
        stmt(lexer::LESS, parse_block_stmt);  // <> {}

        stmt(lexer::USED, parse_var_decl_stmt);
        stmt(lexer::UNUSED, parse_unused_stmt);

        stmt(lexer::TYPEOF, parse_type_change_stmt);

        stmt(lexer::IMPORT, parse_import_stmt);

        // Types
        stmt(lexer::NUMBER_KW, parse_var_decl_stmt);
        stmt(lexer::BYTE_KW, parse_var_decl_stmt);
        stmt(lexer::SBYTE_KW, parse_var_decl_stmt);
        stmt(lexer::BOOL_KW, parse_var_decl_stmt);
        stmt(lexer::AUTO, parse_var_decl_stmt);
        stmt(lexer::ANY, parse_var_decl_stmt);
        stmt(lexer::VOID, parse_var_decl_stmt);
        stmt(lexer::IDENTIFIER, parse_var_decl_stmt);

        stmt(lexer::CHAR8_KW, parse_var_decl_stmt);
        stmt(lexer::CHAR16_KW, parse_var_decl_stmt);
        stmt(lexer::CHAR32_KW, parse_var_decl_stmt);
        stmt(lexer::STRING8_KW, parse_var_decl_stmt);
        stmt(lexer::STRING16_KW, parse_var_decl_stmt);
        stmt(lexer::STRING32_KW, parse_var_decl_stmt);
        stmt(lexer::SHORT_KW, parse_var_decl_stmt);
        stmt(lexer::USHORT_KW, parse_var_decl_stmt);
        stmt(lexer::SBYTE_KW, parse_var_decl_stmt);
        stmt(lexer::INT24_KW, parse_var_decl_stmt);
        stmt(lexer::UINT24_KW, parse_var_decl_stmt);
        stmt(lexer::INT32_KW, parse_var_decl_stmt);
        stmt(lexer::UINT32_KW, parse_var_decl_stmt);
        stmt(lexer::INT64_KW, parse_var_decl_stmt);
        stmt(lexer::UINT64_KW, parse_var_decl_stmt);

        // stmt(lexer::NUMBER, skip_stmt);
        // stmt(lexer::BYTE, skip_stmt);
    }
}