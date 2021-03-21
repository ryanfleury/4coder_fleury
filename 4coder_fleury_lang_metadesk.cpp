
struct F4_MD_LexerState
{
    String_Const_u8 string;
    u8 *at;
    u8 *one_past_last;
};

enum F4_MD_TokenSubKind
{
    F4_MD_TokenSubKind_Null,
    F4_MD_TokenSubKind_Tag,
};

internal F4_LANGUAGE_INDEXFILE(F4_MD_IndexFile)
{
    for(;!ctx->done;)
    {
        Token *name = 0;
        if(F4_Index_RequireTokenKind(ctx, TokenBaseKind_Identifier, &name, F4_Index_TokenSkipFlag_SkipWhitespace))
        {
            if(F4_Index_RequireToken(ctx, S8Lit(":"), F4_Index_TokenSkipFlag_SkipWhitespace))
            {
                F4_Index_MakeNote(ctx, Ii64(name), F4_Index_NoteKind_Constant, 0);
            }
        }
        else if(F4_Index_RequireTokenKind(ctx, TokenBaseKind_Comment, &name, F4_Index_TokenSkipFlag_SkipWhitespace))
        {
            F4_Index_ParseComment(ctx, name);
        }
        else
        {
            F4_Index_ParseCtx_Inc(ctx, F4_Index_TokenSkipFlag_SkipWhitespace);
        }
    }
}

internal F4_LANGUAGE_LEXINIT(F4_MD_LexInit)
{
    F4_MD_LexerState *state = (F4_MD_LexerState *)state_ptr;
    state->string = contents;
    state->at = contents.str;
    state->one_past_last = contents.str + contents.size;
}

internal b32
F4_MD_CharIsSymbol(u8 c)
{
    return (c == '~' || c == '!' || c == '@' || c == '#' || c == '$' ||
            c == '%' || c == '^' || c == '&' || c == '*' || c == '(' ||
            c == ')' || c == '-' || c == '=' || c == '+' || c == '[' ||
            c == ']' || c == '{' || c == '}' || c == ':' || c == ';' ||
            c == ',' || c == '<' || c == '.' || c == '>' || c == '/' ||
            c == '?' || c == '|' || c == '\\');
}

internal F4_LANGUAGE_LEXFULLINPUT(F4_MD_LexFullInput)
{
    b32 result = false;
    F4_MD_LexerState state_ = *(F4_MD_LexerState *)state_ptr;
    F4_MD_LexerState *state = &state_;
    u64 emit_counter = 0;
    i64 strmax = (i64)state->string.size;
    for(i64 i = (i64)(state->at - state->string.str);
        i < strmax && state->at + i < state->one_past_last;)
    {
        i64 start_i = i;
        u8 chr = state->string.str[i];
        
        // NOTE(rjf): Comments
        if(i+1 < strmax &&
           state->string.str[i] == '/' &&
           state->string.str[i+1] == '/')
        {
            Token token = { i, 1, TokenBaseKind_Comment, 0 };
            token.size += 1;
            for(i64 j = i+2; j < strmax && state->string.str[j] != '\n'; j += 1, token.size += 1);
            token_list_push(arena, list, &token);
            i += token.size;
        }
        
        // NOTE(rjf): Comments
        else if(i+1 < strmax &&
                state->string.str[i] == '/' &&
                state->string.str[i+1] == '*')
        {
            Token token = { i, 1, TokenBaseKind_Comment, 0 };
            token.size += 1;
            for(i64 j = i+2; j+1 < strmax && !(state->string.str[j] == '*' && state->string.str[j+1] == '/'); j += 1, token.size += 1);
            token.size += 2;
            token_list_push(arena, list, &token);
            i += token.size;
        }
        
        // NOTE(rjf): Identifier
        else if(character_is_alpha(chr))
        {
            Token token = { i, 1, TokenBaseKind_Identifier, 0 };
            for(i64 j = i+1; j < (i64)state->string.size && 
                (character_is_alpha_numeric(state->string.str[j]) ||
                 state->string.str[j] == '_');
                j += 1, token.size += 1);
            token_list_push(arena, list, &token);
            i += token.size;
        }
        
        // NOTE(rjf): Whitespace
        else if(character_is_whitespace(chr))
        {
            Token token = { i, 1, TokenBaseKind_Whitespace, 0 };
            for(i64 j = i+1; j < (i64)state->string.size && 
                character_is_whitespace(state->string.str[j]);
                j += 1, token.size += 1);
            token_list_push(arena, list, &token);
            i += token.size;
        }
        
        // NOTE(rjf): Numeric Literal
        else if(chr >= '0' && chr <= '9')
        {
            Token token = { i, 1, TokenBaseKind_LiteralFloat, 0 };
            for(i64 j = i+1; j < (i64)state->string.size && 
                (character_is_alpha_numeric(state->string.str[j]) ||
                 state->string.str[j] == '_' ||
                 state->string.str[j] == '.');
                j += 1, token.size += 1);
            token_list_push(arena, list, &token);
            i += token.size;
        }
        
        // NOTE(rjf): Single-Line String Literal
        else if(chr == '"')
        {
            Token token = { i, 1, TokenBaseKind_LiteralString, 0 };
            for(i64 j = i+1; j < (i64)state->string.size && state->string.str[j] != '"';
                j += 1, token.size += 1);
            token.size += 1;
            token_list_push(arena, list, &token);
            i += token.size;
        }
        
        // NOTE(rjf): Single-Line String Literal Marker (Bundle-Of-Tokens)
        else if(chr == '`')
        {
            Token token = { i, 1, TokenBaseKind_LiteralString, 0 };
            token_list_push(arena, list, &token);
            i += token.size;
        }
        
        // NOTE(rjf): Single-Line Char Literal
        else if(chr == '\'')
        {
            Token token = { i, 1, TokenBaseKind_LiteralString, 0 };
            for(i64 j = i+1; j < (i64)state->string.size && state->string.str[j] != '\'';
                j += 1, token.size += 1);
            token.size += 1;
            token_list_push(arena, list, &token);
            i += token.size;
        }
        
        // NOTE(rjf): Multi-line String Literal
        else if(i+2 < strmax &&
                state->string.str[i]   == '"' &&
                state->string.str[i+1] == '"' &&
                state->string.str[i+2] == '"')
        {
            Token token = { i, 1, TokenBaseKind_LiteralString, 0 };
            for(i64 j = i+1; j+2 < (i64)state->string.size &&
                !(state->string.str[j]   == '"' &&
                  state->string.str[j+1] == '"' &&
                  state->string.str[j+2] == '"');
                j += 1, token.size += 1);
            token.size += 3;
            token_list_push(arena, list, &token);
            i += token.size;
        }
        
        // NOTE(rjf): Multi-Line String Literal Marker (Bundle-Of-Tokens)
        else if(i+2 < strmax &&
                state->string.str[i]   == '`' &&
                state->string.str[i+1] == '`' &&
                state->string.str[i+2] == '`')
        {
            Token token = { i, 3, TokenBaseKind_LiteralString, 0 };
            token_list_push(arena, list, &token);
            i += token.size;
        }
        
        // NOTE(rjf): Multi-line Char Literal
        else if(i+2 < strmax &&
                state->string.str[i]   == '\'' &&
                state->string.str[i+1] == '\'' &&
                state->string.str[i+2] == '\'')
        {
            Token token = { i, 1, TokenBaseKind_LiteralString, 0 };
            for(i64 j = i+1; j+2 < (i64)state->string.size &&
                !(state->string.str[j]   == '\'' &&
                  state->string.str[j+1] == '\'' &&
                  state->string.str[j+2] == '\'');
                j += 1, token.size += 1);
            token.size += 3;
            token_list_push(arena, list, &token);
            i += token.size;
        }
        
        // NOTE(rjf): Tags
        else if(chr == '@')
        {
            Token token = { i, 1, TokenBaseKind_Identifier, 0 };
            token.sub_kind = F4_MD_TokenSubKind_Tag;
            for(i64 j = i+1; j < (i64)state->string.size && 
                (character_is_alpha_numeric(state->string.str[j]) ||
                 state->string.str[j] == '_');
                j += 1, token.size += 1);
            token_list_push(arena, list, &token);
            i += token.size;
        }
        
        // NOTE(rjf): Scope-Open
        else if(chr == '{')
        {
            Token token = { i, 1, TokenBaseKind_ScopeOpen, 0 };
            token_list_push(arena, list, &token);
            i += token.size;
        }
        
        // NOTE(rjf): Scope-Close
        else if(chr == '}')
        {
            Token token = { i, 1, TokenBaseKind_ScopeClose, 0 };
            token_list_push(arena, list, &token);
            i += token.size;
        }
        
        // NOTE(rjf): Paren-Open
        else if(chr == '(' || chr == '[')
        {
            Token token = { i, 1, TokenBaseKind_ParentheticalOpen, 0 };
            token_list_push(arena, list, &token);
            i += token.size;
        }
        
        // NOTE(rjf): Scope-Close
        else if(chr == ')' || chr == ']')
        {
            Token token = { i, 1, TokenBaseKind_ParentheticalClose, 0 };
            token_list_push(arena, list, &token);
            i += token.size;
        }
        
        // NOTE(rjf): Statement closes
        else if(chr == ',' || chr == ';' || (chr == '-' && i+1 < strmax && state->string.str[i+1] == '>'))
        {
            Token token = { i, 1, TokenBaseKind_StatementClose, 0 };
            token_list_push(arena, list, &token);
            i += token.size;
        }
        
        // NOTE(rjf): Operators
        else if(F4_MD_CharIsSymbol(chr))
        {
            Token token = { i, 1, TokenBaseKind_Operator, 0 };
            token_list_push(arena, list, &token);
            i += token.size;
        }
        
        // NOTE(rjf): Catch-All
        else
        {
            Token token = {i, 1, TokenBaseKind_LexError, 0 };
            token_list_push(arena, list, &token);
            i += token.size;
        }
        
        if(state->at >= state->one_past_last)
        {
            goto eof;
        }
        else if(start_i == i)
        {
            i += 1;
            state->at = state->string.str + i;
        }
        else
        {
            state->at = state->string.str + i;
            emit_counter += 1;
            if(emit_counter >= max)
            {
                goto end;
            }
        }
    }
    
    // NOTE(rjf): Add EOF
    eof:;
    {
        result = true;
        Token token = { (i64)state->string.size, 1, TokenBaseKind_EOF, 0 };
        token_list_push(arena, list, &token);
    }
    
    end:;
    *(F4_MD_LexerState *)state_ptr = *state;
    return result;
}

internal F4_LANGUAGE_POSCONTEXT(F4_MD_PosContext)
{
    return 0;
}

internal F4_LANGUAGE_HIGHLIGHT(F4_MD_Highlight)
{
    Range_i64 visible_range = text_layout_get_visible_range(app, text_layout_id);
    i64 first_index = token_index_from_pos(array, visible_range.first);
    Token_Iterator_Array it = token_iterator_index(0, array, first_index);
    
    for(;;)
    {
        Token *token = token_it_read(&it);
        if(!token || token->pos >= visible_range.one_past_last)
        {
            break;
        }
        if(token->sub_kind == F4_MD_TokenSubKind_Tag)
        {
            paint_text_color(app, text_layout_id, Ii64(token), F4_ARGBFromID(color_table, fleury_color_index_comment_tag, 0));
        }
        if(!token_it_inc_all(&it))
        {
            break;
        }
    }
}