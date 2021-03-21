
internal void
F4_Jai_ParseDeclSet(F4_Index_ParseCtx *ctx, F4_Index_Note *parent)
{
    F4_Index_Note *last_parent = F4_Index_PushParent(ctx, parent);
    for(;!ctx->done;)
    {
        Token *name = 0;
        if(F4_Index_RequireTokenKind(ctx, TokenBaseKind_Identifier, &name, F4_Index_TokenSkipFlag_SkipWhitespace) &&
           F4_Index_RequireToken(ctx, S8Lit(":"), F4_Index_TokenSkipFlag_SkipWhitespace))
        {
            F4_Index_MakeNote(ctx, Ii64(name), F4_Index_NoteKind_Decl, 0);
            
            for(;!ctx->done;)
            {
                Token *token = token_it_read(&ctx->it);
                if(token->sub_kind == TokenJaiKind_Comma ||
                   token->sub_kind == TokenJaiKind_Semicolon)
                {
                    F4_Index_ParseCtx_Inc(ctx, F4_Index_TokenSkipFlag_SkipWhitespace);
                    break;
                }
                else if(token->kind == TokenBaseKind_ScopeClose ||
                        token->sub_kind == TokenJaiKind_ParenCl)
                {
                    goto end;
                }
                F4_Index_ParseCtx_Inc(ctx, F4_Index_TokenSkipFlag_SkipWhitespace);
            }
            
        }
        else
        {
            break;
        }
    }
    
    end:;
    F4_Index_PopParent(ctx, last_parent);
}

internal void
F4_Jai_ParseDeclSet_Braces(F4_Index_ParseCtx *ctx, F4_Index_Note *parent)
{
    if(F4_Index_RequireToken(ctx, S8Lit("{"), F4_Index_TokenSkipFlag_SkipWhitespace))
    {
        F4_Jai_ParseDeclSet(ctx, parent);
        F4_Index_RequireToken(ctx, S8Lit("}"), F4_Index_TokenSkipFlag_SkipWhitespace);
    }
}

internal void
F4_Jai_ParseDeclSet_Parens(F4_Index_ParseCtx *ctx, F4_Index_Note *parent)
{
    if(F4_Index_RequireToken(ctx, S8Lit("("), F4_Index_TokenSkipFlag_SkipWhitespace))
    {
        F4_Jai_ParseDeclSet(ctx, parent);
        F4_Index_RequireToken(ctx, S8Lit(")"), F4_Index_TokenSkipFlag_SkipWhitespace);
    }
}

internal F4_LANGUAGE_INDEXFILE(F4_Jai_IndexFile)
{
    int scope_nest = 0;
    for(;!ctx->done;)
    {
        Token *name = 0;
        F4_Index_TokenSkipFlags flags = F4_Index_TokenSkipFlag_SkipWhitespace;
        
        // NOTE(rjf): Handle nests.
        {
            Token *token = token_it_read(&ctx->it);
            if(token)
            {
                if(token->kind == TokenBaseKind_ScopeOpen)
                {
                    scope_nest += 1;
                }
                else if(token->kind == TokenBaseKind_ScopeClose)
                {
                    scope_nest -= 1;
                }
                if(scope_nest < 0)
                {
                    scope_nest = 0;
                }
            }
        }
        
        if(F4_Index_RequireTokenKind(ctx, TokenBaseKind_Identifier, &name, flags))
        {
            
            // NOTE(rjf): Definitions
            if(F4_Index_RequireToken(ctx, S8Lit("::"), flags))
            {
                // NOTE(rjf): Procedures
                if(F4_Index_PeekToken(ctx, S8Lit("(")) ||
                   (F4_Index_RequireToken(ctx, S8Lit("inline"), flags) &&
                    F4_Index_PeekToken(ctx, S8Lit("("))))
                {
                    F4_Index_Note *parent = F4_Index_MakeNote(ctx, Ii64(name), F4_Index_NoteKind_Function, 0);
                    F4_Jai_ParseDeclSet_Parens(ctx, parent);
                }
                // NOTE(rjf): Structs
                else if(F4_Index_RequireToken(ctx, S8Lit("struct"), flags))
                {
                    F4_Index_Note *parent = F4_Index_MakeNote(ctx, Ii64(name), F4_Index_NoteKind_Type, F4_Index_NoteFlag_ProductType);
                    F4_Jai_ParseDeclSet_Braces(ctx, parent);
                }
                // NOTE(rjf): Unions
                else if(F4_Index_RequireToken(ctx, S8Lit("union"), flags))
                {
                    F4_Index_Note *parent = F4_Index_MakeNote(ctx, Ii64(name), F4_Index_NoteKind_Type, F4_Index_NoteFlag_SumType);
                    F4_Jai_ParseDeclSet_Braces(ctx, parent);
                }
                // NOTE(rjf): Enums
                else if(F4_Index_RequireToken(ctx, S8Lit("enum"), flags))
                {
                    F4_Index_MakeNote(ctx, Ii64(name), F4_Index_NoteKind_Type, 0);
                }
                // NOTE(rjf): Constants
                else if(F4_Index_RequireTokenKind(ctx, TokenBaseKind_Identifier, 0, flags) ||
                        F4_Index_RequireTokenKind(ctx, TokenBaseKind_LiteralInteger, 0, flags) ||
                        F4_Index_RequireTokenKind(ctx, TokenBaseKind_LiteralFloat, 0, flags) ||
                        F4_Index_RequireTokenKind(ctx, TokenBaseKind_LiteralString, 0, flags))
                {
                    F4_Index_MakeNote(ctx, Ii64(name), F4_Index_NoteKind_Constant, 0);
                }
            }
        }
        
        //~ NOTE(rjf): Comment Tags
        else if(F4_Index_RequireTokenKind(ctx, TokenBaseKind_Comment, &name, F4_Index_TokenSkipFlag_SkipWhitespace))
        {
            F4_Index_ParseComment(ctx, name);
        }
        
        else
        {
            F4_Index_ParseCtx_Inc(ctx, flags);
        }
    }
}

internal Token *
_F4_Jai_FindDecl(Application_Links *app, Buffer_ID buffer, i64 pos, Token *decl_name)
{
    Token *result = 0;
    Scratch_Block scratch(app);
    
    int scope_nest = 0;
    String_Const_u8 decl_name_str = push_buffer_range(app, scratch, buffer, Ii64(decl_name));
    Token_Array tokens = get_token_array_from_buffer(app, buffer);
    Token_Iterator_Array it = token_iterator_pos(0, &tokens, pos);
    for(;;)
    {
        Token *token = token_it_read(&it);
        if(token)
        {
            if(scope_nest == 0 &&
               token->sub_kind == TokenJaiKind_Colon &&
               token_it_dec_non_whitespace(&it))
            {
                Token *name_candidate = token_it_read(&it);
                if(name_candidate && name_candidate->kind == TokenBaseKind_Identifier)
                {
                    String_Const_u8 name_candidate_string = push_buffer_range(app, scratch, buffer, Ii64(name_candidate));
                    if(string_match(name_candidate_string, decl_name_str))
                    {
                        result = name_candidate;
                        break;
                    }
                }
            }
            else if(token->sub_kind == TokenJaiKind_BraceCl)
            {
                scope_nest += 1;
            }
            else if(token->sub_kind == TokenJaiKind_BraceOp)
            {
                scope_nest -= 1;
            }
        }
        else { break; }
        if(!token_it_dec_non_whitespace(&it))
        {
            break;
        }
    }
    return result;
}

internal F4_LANGUAGE_POSCONTEXT(F4_Jai_PosContext)
{
    int count = 0;
    F4_Language_PosContextData *first = 0;
    F4_Language_PosContextData *last = 0;
    
    Token_Array tokens = get_token_array_from_buffer(app, buffer);
    
    // NOTE(rjf): Search for left parentheses (function call or macro invocation).
    {
        Token_Iterator_Array it = token_iterator_pos(0, &tokens, pos);
        
        int paren_nest = 0;
        int arg_idx = 0;
        for(int i = 0; count < 4; i += 1)
        {
            Token *token = token_it_read(&it);
            if(token)
            {
                if(paren_nest == 0 &&
                   token->sub_kind == TokenJaiKind_ParenOp &&
                   token_it_dec_non_whitespace(&it))
                {
                    Token *name = token_it_read(&it);
                    if(name && name->kind == TokenBaseKind_Identifier)
                    {
                        F4_Language_PosContext_PushData_Call(arena, &first, &last, push_buffer_range(app, arena, buffer, Ii64(name)), arg_idx);
						count += 1;
                        arg_idx = 0;
                    }
                }
                else if(token->sub_kind == TokenJaiKind_ParenOp)
                {
                    paren_nest -= 1;
                }
                else if(token->sub_kind == TokenJaiKind_ParenCl && i > 0)
                {
                    paren_nest += 1;
                }
                else if(token->sub_kind == TokenJaiKind_Comma && i > 0 && paren_nest == 0)
                {
                    arg_idx += 1;
                }
            }
            else { break; }
            if(!token_it_dec_non_whitespace(&it))
            {
                break;
            }
        }
    }
    
    // NOTE(rjf): Search for *.* pattern (accessing a type)
    {
        Token_Iterator_Array it = token_iterator_pos(0, &tokens, pos);
        
        Token *last_query_candidate = 0;
        for(int i = 0; i < 3; i += 1)
        {
            Token *token = token_it_read(&it);
            if(token)
            {
                if(i == 0 && token->kind == TokenBaseKind_Identifier)
                {
                    last_query_candidate = token;
                }
                else if((i == 0 || i == 1) &&
					    token->sub_kind == TokenJaiKind_Dot &&
                        token_it_dec_non_whitespace(&it))
                {
                    Token *decl_name = token_it_read(&it);
                    if(decl_name && decl_name->kind == TokenBaseKind_Identifier)
                    {
                        Token *decl_start = _F4_Jai_FindDecl(app, buffer, decl_name->pos, decl_name);
                        if(decl_start)
                        {
                            Token_Iterator_Array it2 = token_iterator_pos(0, &tokens, decl_start->pos);
                            b32 found_colon = 0;
                            Token *base_type = 0;
                            for(;;)
                            {
                                Token *token2 = token_it_read(&it2);
                                if(token2)
                                {
                                    if(token2->sub_kind == TokenJaiKind_Colon)
                                    {
                                        found_colon = 1;
                                    }
                                    else if(found_colon && token2->kind == TokenBaseKind_Identifier)
                                    {
                                        base_type = token2;
                                    }
                                    else if(found_colon && token2->kind == TokenBaseKind_StatementClose)
                                    {
                                        break;
                                    }
                                }
                                else { break; }
                                if(!token_it_inc_non_whitespace(&it2)) { break; }
                            }
                            
                            if(base_type != 0)
                            {
                                F4_Language_PosContext_PushData_Dot(arena, &first, &last,
                                                                    push_buffer_range(app, arena, buffer, Ii64(base_type)), last_query_candidate);
                            }
                        }
                    }
                }
            }
            else { break; }
            if(!token_it_dec_non_whitespace(&it))
            {
                break;
            }
        }
    }
    
    return first;
}

internal F4_LANGUAGE_HIGHLIGHT(F4_Jai_Highlight)
{
}