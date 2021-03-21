
internal void
F4_CPP_ParseMacroDefinition(F4_Index_ParseCtx *ctx)
{
    Token *name = 0;
    if(F4_Index_ParsePattern(ctx, "%k", TokenBaseKind_Identifier, &name))
    {
        F4_Index_Note *last_parent = F4_Index_PushParent(ctx, 0);
        F4_Index_MakeNote(ctx, Ii64(name), F4_Index_NoteKind_Macro, 0);
        F4_Index_PopParent(ctx, last_parent);
        F4_Index_SkipSoftTokens(ctx, 1);
    }
}

internal b32
F4_CPP_SkipParseBody(F4_Index_ParseCtx *ctx)
{
    b32 body_found = 0;
    int nest = 0;
    
    for(;!ctx->done;)
    {
        Token *name = 0;
        
        if(F4_Index_ParsePattern(ctx, "%k", TokenBaseKind_Comment, &name))
        {
            F4_Index_ParseComment(ctx, name);
        }
        else if(F4_Index_ParsePattern(ctx, "%b", TokenCppKind_PPDefine, &name))
        {
            F4_CPP_ParseMacroDefinition(ctx);
        }
        else if(F4_Index_ParsePattern(ctx, "%t", "{"))
        {
            nest += 1;
            body_found = 1;
        }
        else if(F4_Index_ParsePattern(ctx, "%t", "}"))
        {
            nest -= 1;
            if(nest == 0)
            {
                break;
            }
        }
        else if(body_found == 0)
        {
            break;
        }
        else
        {
            F4_Index_ParseCtx_Inc(ctx, F4_Index_TokenSkipFlag_SkipWhitespace);
        }
    }
    return body_found;
}

function b32
F4_CPP_ParseDecl(F4_Index_ParseCtx *ctx, Token **name)
{
    Token *base_type = 0;
    return (F4_Index_ParsePattern(ctx, "%k%o%k%o%t",
                                  TokenBaseKind_Identifier, &base_type,
                                  TokenBaseKind_Identifier, name,
                                  ";") ||
            F4_Index_ParsePattern(ctx, "%k%o%k%o%t",
                                  TokenBaseKind_Keyword, &base_type,
                                  TokenBaseKind_Identifier, name,
                                  ";") ||
            F4_Index_ParsePattern(ctx, "%k%o%k%t",
                                  TokenBaseKind_Identifier, &base_type,
                                  TokenBaseKind_Identifier, name,
                                  "=") ||
            F4_Index_ParsePattern(ctx, "%k%o%k%t",
                                  TokenBaseKind_Keyword, &base_type,
                                  TokenBaseKind_Identifier, name,
                                  "="));
}

function void
F4_CPP_ParseStructOrUnionBodyIFuckingHateCPlusPlus(F4_Index_ParseCtx *ctx, F4_Index_NoteFlags note_flags)
{
    Token *name = 0;
    b32 valid = 0;
    b32 need_end_name = 0;
    
    if(F4_Index_ParsePattern(ctx, "%k", TokenBaseKind_Identifier, &name))
    {
        valid = 1;
    }
    else
    {
        need_end_name = 1;
    }
    
    if(F4_CPP_SkipParseBody(ctx))
    {
    }
    else
    {
        note_flags |= F4_Index_NoteFlag_Prototype;
    }
    
    if(need_end_name)
    {
        if(F4_Index_ParsePattern(ctx, "%k", TokenBaseKind_Identifier, &name))
        {
            valid = 1;
        }
    }
    
    if(valid)
    {
        F4_Index_MakeNote(ctx, Ii64(name), F4_Index_NoteKind_Type, note_flags);
    }
}

function b32
F4_CPP_ParseFunctionBodyIFuckingHateCPlusPlus(F4_Index_ParseCtx *ctx, b32 *prototype_ptr)
{
    b32 valid = 0;
    b32 prototype = 0;
    
    for(;!ctx->done;)
    {
        Token *token = token_it_read(&ctx->it);
        if(token == 0) { break; }
        if(token->sub_kind == TokenCppKind_Semicolon)
        {
            valid = 1;
            prototype = 1;
            break;
        }
        else if(token->sub_kind == TokenCppKind_ParenCl)
        {
        }
        else if(token->kind == TokenBaseKind_ScopeOpen)
        {
            valid = 1;
            break;
        }
        F4_Index_ParseCtx_Inc(ctx, 0);
    }
    
    if(valid)
    {
        if(prototype == 0)
        {
            F4_CPP_SkipParseBody(ctx);
        }
    }
    
    *prototype_ptr = prototype;
    
    return valid;
}

function void
F4_CPP_ParseEnumBodyIFuckingHateCPlusPlus(F4_Index_ParseCtx *ctx)
{
    if(F4_Index_ParsePattern(ctx, "%t", "{"))
    {
        for(;!ctx->done;)
        {
            Token *constant = 0;
            if(F4_Index_ParsePattern(ctx, "%k%t", TokenBaseKind_Identifier, &constant, ","))
            {
                F4_Index_MakeNote(ctx, Ii64(constant), F4_Index_NoteKind_Constant, 0);
            }
            else if(F4_Index_ParsePattern(ctx, "%k%t", TokenBaseKind_Identifier, &constant, "="))
            {
                F4_Index_MakeNote(ctx, Ii64(constant), F4_Index_NoteKind_Constant, 0);
                
                for(;!ctx->done;)
                {
                    Token *token = token_it_read(&ctx->it);
                    if(token->kind == TokenBaseKind_StatementClose)
                    {
                        F4_Index_ParseCtx_Inc(ctx, 0);
                        break;
                    }
                    else if(token->kind == TokenBaseKind_ScopeClose ||
                            token->kind == TokenBaseKind_ScopeOpen)
                    {
                        break;
                    }
                    F4_Index_ParseCtx_Inc(ctx, 0);
                }
            }
            else if(F4_Index_ParsePattern(ctx, "%k", TokenBaseKind_Identifier, &constant))
            {
                F4_Index_MakeNote(ctx, Ii64(constant), F4_Index_NoteKind_Constant, 0);
            }
            else if(F4_Index_ParsePattern(ctx, "%t", "}"))
            {
                break;
            }
            else
            {
                F4_Index_ParseCtx_Inc(ctx, 0);
            }
        }
    }
}

internal F4_LANGUAGE_INDEXFILE(F4_CPP_IndexFile)
{
    int scope_nest = 0;
    for(b32 handled = 0; !ctx->done;)
    {
        handled = 0;
        
        Token *name = 0;
        Token *base_type = 0;
        F4_Index_Note *containing_struct = 0;
        F4_Index_Note *note = 0;
        
        if(0){}
        
        //~ NOTE(rjf): Extern "C" scope changes (ignore) ((dude C++ syntax is so fucked up))
        // NOTE(rjf): CORRECTION: Text files in general are so fucked up, fuck all of this
        // parsing bullshit
        else if(F4_Index_ParsePattern(ctx, "%t%t%t", "extern", "C", "{"))
        {
            handled = 1;
        }
        
        //~ NOTE(rjf): Scope Nest Changes
        else if(F4_Index_ParsePattern(ctx, "%t", "{"))
        {
            handled = 1;
            scope_nest += 1;
        }
        else if(F4_Index_ParsePattern(ctx, "%t", "}"))
        {
            handled = 1;
            scope_nest -= 1;
            if(scope_nest < 0)
            {
                scope_nest = 0;
            }
        }
        
        //~ NOTE(rjf): Structs
        else if(F4_Index_ParsePattern(ctx, "%t", "struct"))
        {
            handled = 1;
            F4_CPP_ParseStructOrUnionBodyIFuckingHateCPlusPlus(ctx, F4_Index_NoteFlag_ProductType);
        }
        else if(F4_Index_ParsePattern(ctx, "%t%t", "typedef", "struct"))
        {
            handled = 1;
            F4_CPP_ParseStructOrUnionBodyIFuckingHateCPlusPlus(ctx, 0);
            if (F4_Index_ParsePattern(ctx, "%k", TokenBaseKind_Identifier, &name))
            {
                F4_Index_MakeNote(ctx, Ii64(name), F4_Index_NoteKind_Type, F4_Index_NoteFlag_ProductType);
            }
        }
        
        //~ NOTE(rjf): Unions
        else if(F4_Index_ParsePattern(ctx, "%t", "union"))
        {
            handled = 1;
            F4_CPP_ParseStructOrUnionBodyIFuckingHateCPlusPlus(ctx, F4_Index_NoteFlag_SumType);
        }
        else if (F4_Index_ParsePattern(ctx, "%t%t", "typedef", "union"))
        {
            handled = 1;
            F4_CPP_ParseStructOrUnionBodyIFuckingHateCPlusPlus(ctx, F4_Index_NoteFlag_SumType);
            if (F4_Index_ParsePattern(ctx, "%k", TokenBaseKind_Identifier, &name))
            {
                F4_Index_MakeNote(ctx, Ii64(name), F4_Index_NoteKind_Type, F4_Index_NoteFlag_SumType);
            }
        }
        
        //~ NOTE(rjf): Typedef'd Enums
        else if(F4_Index_ParsePattern(ctx, "%t%t%k", "typedef", "enum", TokenBaseKind_Identifier, &name) ||
                F4_Index_ParsePattern(ctx, "%t%t", "typedef", "enum"))
        {
            handled = 1;
            b32 prototype = 0;
            b32 possible_name_at_end = name == 0;
            if(F4_Index_ParsePattern(ctx, "%t", ";"))
            {
                prototype = 1;
            }
            if(prototype == 0)
            {
                F4_CPP_ParseEnumBodyIFuckingHateCPlusPlus(ctx);
            }
            if(possible_name_at_end)
            {
                if(F4_Index_ParsePattern(ctx, "%k", TokenBaseKind_Identifier, &name))
                {}
            }
            if(name != 0)
            {
                F4_Index_MakeNote(ctx, Ii64(name), F4_Index_NoteKind_Type, prototype ? F4_Index_NoteFlag_Prototype : 0);
            }
        }
        
        //~ NOTE(rjf): Enums
        else if(F4_Index_ParsePattern(ctx, "%t%k", "enum", TokenBaseKind_Identifier, &name) ||
                F4_Index_ParsePattern(ctx, "%t", "enum"))
        {
            handled = 1;
            b32 prototype = 0;
            if(F4_Index_ParsePattern(ctx, "%t", ";"))
            {
                prototype = 1;
            }
            if(prototype == 0)
            {
                F4_CPP_ParseEnumBodyIFuckingHateCPlusPlus(ctx);
            }
            if(name != 0)
            {
                F4_Index_MakeNote(ctx, Ii64(name), F4_Index_NoteKind_Type, prototype ? F4_Index_NoteFlag_Prototype : 0);
            }
        }
        
        //~ NOTE(rjf): Pure Typedefs
        else if(F4_Index_ParsePattern(ctx, "%t", "typedef"))
        {
            handled = 1;
            int nest = 0;
            b32 sum_type = 0;
            for(;!ctx->done;)
            {
                if(F4_Index_ParsePattern(ctx, "%t", "("))
                {
                    nest += 1;
                }
                else if(F4_Index_ParsePattern(ctx, "%t", "("))
                {
                    nest -= 1;
                }
                else if(nest == 0 && F4_Index_ParsePattern(ctx, "%k", TokenBaseKind_Identifier, &name))
                {
                    String8 namestr = F4_Index_StringFromToken(ctx, name);
                    F4_Index_Note *namenote = F4_Index_LookupNote(namestr, 0);
                    if(namenote != 0 && namenote->kind == F4_Index_NoteKind_Type &&
                       namenote->flags & F4_Index_NoteFlag_SumType)
                    {
                        sum_type = 1;
                    }
                }
                else if(F4_Index_ParsePattern(ctx, "%t", ";"))
                {
                    break;
                }
                else 
                {
                    F4_Index_ParseCtx_Inc(ctx, 0);
                }
            }
            if(name != 0)
            {
                F4_Index_NoteFlags note_flags = 0;
                if(sum_type)
                {
                    note_flags |= F4_Index_NoteFlag_SumType;
                }
                F4_Index_MakeNote(ctx, Ii64(name), F4_Index_NoteKind_Type, note_flags);
            }
        }
        
        //~ NOTE(rjf): Functions
        else if(scope_nest == 0 &&
                (F4_Index_ParsePattern(ctx, "%k%o%k%t",
                                       TokenBaseKind_Identifier, &base_type,
                                       TokenBaseKind_Identifier, &name,
                                       "(") ||
                 F4_Index_ParsePattern(ctx, "%k%o%k%t",
                                       TokenBaseKind_Keyword, &base_type,
                                       TokenBaseKind_Identifier, &name,
                                       "(")))
        {
            handled = 1;
            b32 prototype = 0;
            if(F4_CPP_ParseFunctionBodyIFuckingHateCPlusPlus(ctx, &prototype))
            {
                F4_Index_MakeNote(ctx, Ii64(name), F4_Index_NoteKind_Function, prototype ? F4_Index_NoteFlag_Prototype : 0);
            }
        }
        
        //~ NOTE(rjf): Member Functions
        else if(scope_nest == 0 &&
                (F4_Index_ParsePattern(ctx, "%k%o%n%t%k%t",
                                       TokenBaseKind_Identifier, &base_type,
                                       F4_Index_NoteKind_Type, &containing_struct,
                                       "::",
                                       TokenBaseKind_Identifier, &name,
                                       "(") ||
                 F4_Index_ParsePattern(ctx, "%k%o%n%t%k%t",
                                       TokenBaseKind_Keyword, &base_type,
                                       F4_Index_NoteKind_Type, &containing_struct,
                                       "::",
                                       TokenBaseKind_Identifier, &name,
                                       "(")))
        {
            handled = 1;
            b32 prototype = 0;
            if(F4_CPP_ParseFunctionBodyIFuckingHateCPlusPlus(ctx, &prototype))
            {
                F4_Index_MakeNote(ctx, Ii64(name), F4_Index_NoteKind_Function, prototype ? F4_Index_NoteFlag_ProductType : 0);
            }
        }
        
        //~ NOTE(rjf): Declarations
        else if(scope_nest == 0 && F4_CPP_ParseDecl(ctx, &name))
        {
            handled = 1;
            F4_Index_MakeNote(ctx, Ii64(name), F4_Index_NoteKind_Decl, 0);
        }
        
        //~ NOTE(rjf): Macro Functions
        else if(0 && F4_Index_ParsePattern(ctx, "%n%t%k",
                                           F4_Index_NoteKind_Macro, &note,
                                           "(",
                                           TokenBaseKind_Identifier, &name))
        {
            b32 valid = 0;
            b32 prototype = 0;
            
            for(;!ctx->done;)
            {
                Token *token = token_it_read(&ctx->it);
                if(token == 0) { break; }
                if(token->sub_kind == TokenCppKind_Semicolon)
                {
                    prototype = 1;
                    valid = 1;
                    break;
                }
                else if(token->sub_kind == TokenCppKind_ParenCl)
                {
                }
                else if(token->kind == TokenBaseKind_ScopeOpen)
                {
                    valid = 1;
                    break;
                }
                F4_Index_ParseCtx_Inc(ctx, 0);
            }
            
            if(valid)
            {
                handled = 1;
                F4_Index_MakeNote(ctx, Ii64(name), F4_Index_NoteKind_Function, prototype ? F4_Index_NoteFlag_ProductType : 0);
                F4_CPP_SkipParseBody(ctx);
            }
        }
        
        //~ NOTE(rjf): Comment Tags
        else if(F4_Index_ParsePattern(ctx, "%k", TokenBaseKind_Comment, &name))
        {
            handled = 1;
            F4_Index_ParseComment(ctx, name);
        }
        
        //~ NOTE(rjf): Macros
        else if(F4_Index_ParsePattern(ctx, "%b", TokenCppKind_PPDefine, &name))
        {
            handled = 1;
            F4_CPP_ParseMacroDefinition(ctx);
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
