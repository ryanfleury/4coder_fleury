
internal b32
F4_CPP_SkipParseBody(F4_Index_ParseCtx *ctx)
{
    b32 body_found = 0;
    int nest = 0;
    
    for(;!ctx->done;)
    {
        Token *token = token_it_read(&ctx->it);
        if(token->sub_kind == TokenCppKind_BraceOp)
        {
            nest += 1;
            body_found = 1;
        }
        else if(token->sub_kind == TokenCppKind_BraceCl)
        {
            nest -= 1;
            if(nest == 0)
            {
                break;
            }
        }
        else
        {
            break;
        }
        
        F4_Index_ParseCtx_Inc(ctx, F4_Index_TokenSkipFlag_SkipWhitespace);
    }
    return body_found;
}

internal F4_LANGUAGE_INDEXFILE(F4_CPP_IndexFile)
{
    int scope_nest = 0;
    
    for(;!ctx->done;)
    {
        Token *name = 0;
        F4_Index_TokenSkipFlags flags = F4_Index_TokenSkipFlag_SkipWhitespace;
        
        // NOTE(rjf): Handle nests.
        {
            Token *token = token_it_read(&ctx->it);
            if(token && !(token->flags & TokenBaseFlag_PreprocessorBody))
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
        
        b32 handled = 0;
        
        //~ NOTE(rjf): Typedef'd things
        if(scope_nest == 0 && (F4_Index_RequireToken(ctx, S8Lit("typedef"), flags) ||
                               (F4_Index_PeekToken(ctx, S8Lit("struct"))) ||
                               (F4_Index_PeekToken(ctx, S8Lit("union"))) ||
                               (F4_Index_PeekToken(ctx, S8Lit("enum")))))
        {
            //~ NOTE(rjf): Structs
            if(scope_nest == 0 &&
               F4_Index_RequireToken(ctx, S8Lit("struct"), flags) &&
               F4_Index_RequireTokenKind(ctx, TokenBaseKind_Identifier, &name, flags))
            {
                handled = 1;
                F4_Index_NoteFlags note_flags = F4_Index_NoteFlag_ProductType;
                if(!F4_CPP_SkipParseBody(ctx))
                {
                    note_flags |= F4_Index_NoteFlag_Prototype;
                }
                F4_Index_MakeNote(ctx->app, ctx->file, 0,
                                  F4_Index_StringFromToken(ctx, name),
                                  F4_Index_NoteKind_Type,
                                  note_flags, Ii64(name));
            }
            
            //~ NOTE(rjf): Unions
            else if(scope_nest == 0 &&
                    F4_Index_RequireToken(ctx, S8Lit("union"), flags) &&
                    F4_Index_RequireTokenKind(ctx, TokenBaseKind_Identifier, &name, flags))
            {
                handled = 1;
                F4_Index_NoteFlags note_flags = F4_Index_NoteFlag_SumType;
                if(!F4_CPP_SkipParseBody(ctx))
                {
                    note_flags |= F4_Index_NoteFlag_Prototype;
                }
                F4_Index_MakeNote(ctx->app, ctx->file, 0,
                                  F4_Index_StringFromToken(ctx, name),
                                  F4_Index_NoteKind_Type,
                                  note_flags, Ii64(name));
            }
            
            //~ NOTE(rjf): Enums
            else if(scope_nest == 0 &&
                    F4_Index_RequireToken(ctx, string_u8_litexpr("enum"), flags))
            {
                handled = 1;
                F4_Index_NoteFlags note_flags = 0;
                F4_Index_RequireTokenKind(ctx, TokenBaseKind_Identifier, &name, flags);
                
                if (F4_Index_PeekToken(ctx, S8Lit(":"))) {
                    F4_Index_RequireToken(ctx, S8Lit(":"), flags);
                    
                    while (!ctx->done) {
                        if (F4_Index_PeekTokenKind(ctx, TokenBaseKind_Identifier, nullptr) ||
                            F4_Index_PeekTokenKind(ctx, TokenBaseKind_Keyword, nullptr))
                        {
                            F4_Index_ParseCtx_Inc(ctx, flags);
                        } else {
                            break;
                        }
                    }
                }
                
                if(F4_Index_RequireToken(ctx, S8Lit("{"), flags))
                {
                    Token *constant = 0;
                    
                    for(;!ctx->done;)
                    {
                        if(F4_Index_RequireTokenKind(ctx, TokenBaseKind_Identifier, &constant, flags))
                        {
                            F4_Index_MakeNote(ctx->app, ctx->file, 0, F4_Index_StringFromToken(ctx, constant), F4_Index_NoteKind_Constant, 0, Ii64(constant));
                            
                            // NOTE(rjf): Need to skip initializer.
                            if(F4_Index_RequireToken(ctx, S8Lit("="), flags))
                            {
                                for(;!ctx->done;)
                                {
                                    if(F4_Index_PeekToken(ctx, S8Lit("}")) ||
                                       F4_Index_PeekToken(ctx, S8Lit(",")))
                                    {
                                        break;
                                    }
                                    F4_Index_ParseCtx_IncWs(ctx);
                                }
                            }
                        }
                        
                        if(F4_Index_RequireToken(ctx, S8Lit(","), flags))
                        {
                        }
                        else if(F4_Index_RequireToken(ctx, S8Lit("}"), flags))
                        {
                            break;
                        }
                        else
                        {
                            F4_Index_ParseCtx_IncWs(ctx);
                        }
                    }
                }
                else
                {
                    note_flags |= F4_Index_NoteFlag_Prototype;
                }
                
                if(name)
                {
                    F4_Index_MakeNote(ctx->app, ctx->file, 0,
                                      F4_Index_StringFromToken(ctx, name),
                                      F4_Index_NoteKind_Type,
                                      note_flags, Ii64(name));
                }
            }
            
            //~ NOTE(rjf): Regular Typedef
            else
            {
                for(;token_it_inc_all(&ctx->it);)
                {
                    Token *token = token_it_read(&ctx->it);
                    if(token)
                    {
                        if(token->kind == TokenBaseKind_Identifier)
                        {
                            name = token;
                        }
                        else if(token->kind == TokenBaseKind_StatementClose ||
                                token->kind == TokenBaseKind_ParentheticalOpen)
                        {
                            break;
                        }
                    }
                }
                F4_Index_SeekToken(ctx, S8Lit(";"));
                if(name)
                {
                    F4_Index_MakeNote(ctx->app, ctx->file, 0,
                                      F4_Index_StringFromToken(ctx, name),
                                      F4_Index_NoteKind_Type,
                                      0, Ii64(name));
                }
            }
            
        }
        
        //~ NOTE(rjf): Macros
        else if(F4_Index_PeekTokenSubKind(ctx, TokenCppKind_PPDefine, 0))
        {
            handled = 1;
            
            F4_Index_ParseCtx_Inc(ctx, F4_Index_TokenSkipFlag_SkipWhitespace);
            if(F4_Index_RequireTokenKind(ctx, TokenBaseKind_Identifier, &name, flags))
            {
                F4_Index_MakeNote(ctx->app, ctx->file, 0, F4_Index_StringFromToken(ctx, name),
                                  F4_Index_NoteKind_Macro, 0, Ii64(name));
            }
            
            for(;!ctx->done;)
            {
                Token *token = token_it_read(&ctx->it);
                if(!(token->flags & TokenBaseFlag_PreprocessorBody) ||
                   token->kind == TokenBaseKind_Preprocessor)
                {
                    break;
                }
                F4_Index_ParseCtx_IncWs(ctx);
            }
        }
        
        //~ NOTE(rjf): Comment Tags
        else if(F4_Index_RequireTokenKind(ctx, TokenBaseKind_Comment, &name, F4_Index_TokenSkipFlag_SkipWhitespace))
        {
            handled = 1;
            F4_Index_ParseComment(ctx, name);
        }
        
        //~ NOTE(rjf): Functions
        else if(scope_nest == 0)
        {
            Token_Iterator_Array it_restore = ctx->it;
            if(F4_Index_RequireTokenKind(ctx, TokenBaseKind_Identifier, 0, flags) ||
               F4_Index_RequireTokenKind(ctx, TokenBaseKind_Keyword, 0, flags))
            {
                b32 semicolon = 0;
                
                int mode = 0;
                enum
                {
                    ReadMode_Name,
                    ReadMode_OpenParen,
                    ReadMode_CloseParen,
                    ReadMode_SemicolonOrBrace,
                    ReadMode_Valid,
                };
                
                int paren_nest = 0;
                for(;;)
                {
                    switch(mode)
                    {
                        case ReadMode_Name:
                        {
                            if(F4_Index_PeekTokenKind(ctx, TokenBaseKind_Identifier, &name))
                            {
                                mode = ReadMode_OpenParen;
                            }
                            else if(F4_Index_PeekTokenSubKind(ctx, TokenCppKind_Star, &name))
                            {
                            }
                            else
                            {
                                goto end;
                            }
                        }break;
                        
                        case ReadMode_OpenParen:
                        {
                            if(F4_Index_PeekToken(ctx, S8Lit("(")))
                            {
                                mode = ReadMode_CloseParen;
                            }
                            else
                            {
                                goto end;
                            }
                        }break;
                        
                        case ReadMode_CloseParen:
                        {
                            if(F4_Index_PeekToken(ctx, S8Lit("(")))
                            {
                                paren_nest += 1;
                            }
                            else if(F4_Index_PeekToken(ctx, S8Lit(")")))
                            {
                                paren_nest -= 1;
                                if(paren_nest < 0)
                                {
                                    mode = ReadMode_SemicolonOrBrace;
                                }
                            }
                        }break;
                        
                        case ReadMode_SemicolonOrBrace:
                        {
                            if(F4_Index_PeekToken(ctx, S8Lit(";")))
                            {
                                mode = ReadMode_Valid;
                                semicolon = 1;
                                token_it_inc_all(&ctx->it);
                                goto end;
                            }
                            else if(F4_Index_PeekToken(ctx, S8Lit("{")))
                            {
                                mode = ReadMode_Valid;
                                F4_CPP_SkipParseBody(ctx);
                                goto end;
                            }
                            else
                            {
                                goto end;
                            }
                        }break;
                        
                        default: break;
                    }
                    
                    if(!token_it_inc_non_whitespace(&ctx->it))
                    {
                        break;
                    }
                }
                end:;
                
                if(mode == ReadMode_Valid)
                {
                    handled = 1;
                    F4_Index_NoteFlags note_flags = 0;
                    if(semicolon)
                    {
                        note_flags |= F4_Index_NoteFlag_Prototype;
                    }
                    F4_Index_MakeNote(ctx->app, ctx->file, 0,
                                      F4_Index_StringFromToken(ctx, name),
                                      F4_Index_NoteKind_Function,
                                      note_flags, Ii64(name));
                }
                else
                {
                    ctx->it = it_restore;
                }
            }
        }
        
        if(handled == 0)
        {
            F4_Index_ParseCtx_Inc(ctx, 0);
        }
    }
}

internal F4_LANGUAGE_POSCONTEXT(F4_CPP_PosContext)
{
    int count = 0;
    F4_Language_PosContextData *first = 0;
    F4_Language_PosContextData *last = 0;
    
    Token_Array tokens = get_token_array_from_buffer(app, buffer);
    Token_Iterator_Array it = token_iterator_pos(0, &tokens, pos);
    
    // NOTE(rjf): Search for left parentheses (function call or macro invocation).
    {
        int paren_nest = 0;
        int arg_idx = 0;
        for(int i = 0; count < 4; i += 1)
        {
            Token *token = token_it_read(&it);
            if(token)
            {
                if(paren_nest == 0 &&
                   token->sub_kind == TokenCppKind_ParenOp &&
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
                else if(token->sub_kind == TokenCppKind_ParenOp)
                {
                    paren_nest -= 1;
                }
                else if(token->sub_kind == TokenCppKind_ParenCl && i > 0)
                {
                    paren_nest += 1;
                }
                else if(token->sub_kind == TokenCppKind_Comma && i > 0 && paren_nest == 0)
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
    
    // NOTE(rjf): Search for *.* pattern, or *->* pattern (accessing a type)
    {
#if 0
        Token *last_query_candidate = 0;
        for(;;)
        {
            Token *token = token_it_read(&it);
            if(token)
            {
                if(token->kind == TokenBaseKind_Identifier)
                {
                    last_query_candidate = token;
                }
                else if((token->sub_kind == TokenCppKind_Dot ||
                         token->sub_kind == TokenCppKind_Arrow) &&
                        token_it_dec_non_whitespace(&it))
                {
                    Token *decl_name = token_it_read(&it);
                    if(decl_name && decl_name->kind == TokenBaseKind_Identifier)
                    {
                        
                    }
                }
            }
            else { break; }
            if(!token_it_dec_non_whitespace(&it))
            {
                break;
            }
        }
#endif
    }
    
    return first;
}

internal F4_LANGUAGE_HIGHLIGHT(F4_CPP_Highlight)
{
}
