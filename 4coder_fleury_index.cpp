global F4_Index_State f4_index = {};

internal void
F4_Index_Initialize(void)
{
    f4_index.mutex = system_mutex_make();
    f4_index.arena = make_arena_system(KB(16));
}

internal void
F4_Index_Lock(void)
{
    system_mutex_acquire(f4_index.mutex);
}

internal void
F4_Index_Unlock(void)
{
    system_mutex_release(f4_index.mutex);
}

internal u64
_F4_Index_FileHash(Application_Links *app, Buffer_ID id)
{
    Scratch_Block scratch(app);
    String_Const_u8 unique_name = push_buffer_unique_name(app, scratch, id);
    return table_hash_u8(unique_name.str, unique_name.size);
}

internal F4_Index_File *
_F4_Index_LookupFile(Application_Links *app, u64 hash, Buffer_ID buffer)
{
    F4_Index_File *result = 0;
    u64 slot = hash % ArrayCount(f4_index.file_table);
    for(F4_Index_File *file = f4_index.file_table[slot]; file; file = file->hash_next)
    {
        if(file->buffer == buffer)
        {
            result = file;
            break;
        }
    }
    return result;
}

internal F4_Index_File *
F4_Index_LookupFile(Application_Links *app, Buffer_ID buffer)
{
    return _F4_Index_LookupFile(app, _F4_Index_FileHash(app, buffer), buffer);
}

internal F4_Index_File *
F4_Index_LookupOrMakeFile(Application_Links *app, Buffer_ID buffer)
{
    F4_Index_File *result = 0;
    u64 hash = _F4_Index_FileHash(app, buffer);
    u64 slot = hash % ArrayCount(f4_index.file_table);
    
    // NOTE(rjf): Lookup case.
    {
        result = _F4_Index_LookupFile(app, hash, buffer);
        if(result)
        {
            goto end;
        }
    }
    
    // NOTE(rjf): Make case.
    {
        if(f4_index.free_file)
        {
            result = f4_index.free_file;
            f4_index.free_file = f4_index.free_file->hash_next;
            memset(result, 0, sizeof(*result));
        }
        else
        {
            result = push_array_zero(&f4_index.arena, F4_Index_File, 1);
        }
        
        if(result != 0)
        {
            result->hash_next = f4_index.file_table[slot];
            f4_index.file_table[slot] = result;
            result->buffer = buffer;
            result->arena = make_arena_system(KB(16));
        }
    }
    
    end:;
    return result;
}

internal void
F4_Index_EraseFile(Application_Links *app, Buffer_ID id)
{
    u64 hash = _F4_Index_FileHash(app, id);
    F4_Index_File *file = _F4_Index_LookupFile(app, hash, id);
    if(file)
    {
        u64 slot = hash % ArrayCount(f4_index.file_table);
        {
            F4_Index_File *prev = 0;
            for(F4_Index_File *hash_file = f4_index.file_table[slot]; hash_file; prev = hash_file, hash_file = hash_file->hash_next)
            {
                if(file == hash_file)
                {
                    if(prev)
                    {
                        prev->hash_next = file->hash_next;
                    }
                    else
                    {
                        f4_index.file_table[slot] = file->hash_next;
                    }
                    break;
                }
            }
        }
        file->hash_next = f4_index.free_file;
        f4_index.free_file = file;
    }
}

internal void
_F4_Index_FreeNoteTree(F4_Index_Note *note)
{
    for(F4_Index_Note *child = note->first_child; child; child = child->next_sibling)
    {
        _F4_Index_FreeNoteTree(child);
    }
    
    F4_Index_Note *prev = note->prev;
    F4_Index_Note *next = note->next;
    F4_Index_Note *hash_prev = note->hash_prev;
    F4_Index_Note *hash_next = note->hash_next;
    
    u64 hash = note->hash;
    u64 slot = hash % ArrayCount(f4_index.note_table);
    
    if(prev)
    {
        prev->next = next;
    }
    if(next)
    {
        next->prev = prev;
    }
    
    if(prev == 0)
    {
        if(next)
        {
            next->hash_prev = hash_prev;
            next->hash_next = hash_next;
            if(hash_prev)
            {
                hash_prev->hash_next = next;
            }
            if(hash_next)
            {
                hash_next->hash_prev = next;
            }
        }
        else
        {
            if(hash_prev)
            {
                hash_prev->hash_next = hash_next;
            }
            if(hash_next)
            {
                hash_next->hash_prev = hash_prev;
            }
        }
        
        if(hash_prev == 0)
        {
            f4_index.note_table[slot] = next ? next : hash_next;
        }
    }
}

internal void
F4_Index_ClearFile(F4_Index_File *file)
{
    if(file)
    {
        file->generation += 1;
        for(F4_Index_Note *note = file->first_note;
            note; note = note->next_sibling)
        {
            _F4_Index_FreeNoteTree(note);
        }
        linalloc_clear(&file->arena);
        file->first_note = file->last_note = 0;
    }
}

internal F4_Index_Note *
F4_Index_LookupNote(String_Const_u8 string, F4_Index_Note *parent)
{
    F4_Index_Note *result = 0;
    u64 hash = table_hash_u8(string.str, string.size);
    u64 slot = hash % ArrayCount(f4_index.note_table);
    for(F4_Index_Note *note = f4_index.note_table[slot]; note; note = note->hash_next)
    {
        if(note->hash == hash && note->parent == parent)
        {
            if(string_match(string, note->string))
            {
                result = note;
                break;
            }
        }
    }
    return result;
}

internal F4_Index_Note *
F4_Index_LookupNote(String_Const_u8 string)
{
    return F4_Index_LookupNote(string, 0);
}

internal F4_Index_Note *
F4_Index_MakeNote(Application_Links *app,
                  F4_Index_File *file,
                  F4_Index_Note *parent,
                  String_Const_u8 string,
                  F4_Index_NoteKind note_kind,
                  F4_Index_NoteFlags note_flags,
                  Range_i64 range)
{
    ProfileScope(app, "[f] MakeNote");
    
    F4_Index_Note *result = 0;
    if(file)
    {
        u64 hash = table_hash_u8(string.str, string.size);
        u64 slot = hash % ArrayCount(f4_index.note_table);
        
        if(f4_index.free_note)
        {
            result = f4_index.free_note;
            f4_index.free_note = f4_index.free_note->hash_next;
            memset(result, 0, sizeof(*result));
        }
        else
        {
            result = push_array_zero(&f4_index.arena, F4_Index_Note, 1);
        }
        
        // NOTE(rjf): Push to duplicate chain.
        {
            F4_Index_Note *list_head = F4_Index_LookupNote(string);
            F4_Index_Note *list_tail = list_head;
            for(F4_Index_Note *note = list_tail; note; list_tail = note, note = note->next);
            if(list_tail != 0)
            {
                list_tail->next = result;
                result->prev = list_tail;
                result->hash_next = 0;
                result->hash_prev = 0;
            }
            else
            {
                result->hash_next = f4_index.note_table[slot];
                if(f4_index.note_table[slot])
                {
                    f4_index.note_table[slot]->hash_prev = result;
                }
                f4_index.note_table[slot] = result;
                result->hash_prev = 0;
                result->prev = 0;
            }
        }
        result->next = 0;
        
        // NOTE(rjf): Push to tree.
        {
            result->parent = parent;
            if(parent)
            {
                result->prev_sibling = parent->last_child;
                result->next_sibling = 0;
                if(parent->last_child == 0)
                {
                    parent->first_child = parent->last_child = result;
                }
                else
                {
                    parent->last_child->next_sibling = result;
                    parent->last_child = parent->last_child->next_sibling;
                }
            }
            else
            {
                result->prev_sibling = file->last_note;
                result->next_sibling = 0;
                if(file->last_note == 0)
                {
                    file->first_note = file->last_note = result;
                }
                else
                {
                    file->last_note->next_sibling = result;
                    file->last_note = file->last_note->next_sibling;
                }
            }
        }
        
        // NOTE(rjf): Fill out data.
        {
            result->hash = hash;
            result->string = push_string_copy(&file->arena, string);
            result->kind = note_kind;
            result->flags = note_flags;
            result->range = range;
            result->file = file;
            result->file_generation = file->generation;
        }
    }
    return result;
}

internal void
_F4_Index_Parse(Application_Links *app, F4_Index_File *file, String_Const_u8 string, Token_Array tokens, F4_Language *language)
{
    F4_Index_ParseCtx ctx =
    {
        false,
        app,
        file,
        string,
        tokens,
        token_iterator_pos(0, &ctx.tokens, 0),
    };
    if(language != 0)
    {
        language->IndexFile(&ctx);
    }
}

internal void
F4_Index_ParseFile(Application_Links *app, F4_Index_File *file, String_Const_u8 string, Token_Array tokens)
{
    F4_Index_Lock();
    F4_Language *lang = F4_LanguageFromBuffer(app, file->buffer);
    _F4_Index_Parse(app, file, string, tokens, lang);
    F4_Index_Unlock();
}

internal String_Const_u8
F4_Index_StringFromToken(F4_Index_ParseCtx *ctx, Token *token)
{
    Range_i64 token_range = Ii64(token->pos, token->pos+token->size);
    String_Const_u8 string = string_substring(ctx->string, token_range);
    return string;
}

internal b32
F4_Index_ParseCtx_Inc(F4_Index_ParseCtx *ctx, F4_Index_TokenSkipFlags flags)
{
    if(flags & F4_Index_TokenSkipFlag_SkipWhitespace)
    {
        ctx->done = !token_it_inc_non_whitespace(&ctx->it);
    }
    else
    {
        ctx->done = !token_it_inc_all(&ctx->it);
    }
    return ctx->done;
}

internal b32
F4_Index_RequireToken(F4_Index_ParseCtx *ctx, String_Const_u8 string, F4_Index_TokenSkipFlags flags)
{
    b32 result = 0;
    Token *token = token_it_read(&ctx->it);
    if(token)
    {
        String_Const_u8 token_string =
            string_substring(ctx->string, Ii64(token->pos, token->pos+token->size));
        if(string_match(string, token_string))
        {
            result = 1;
        }
    }
    else
    {
        ctx->done = 1;
    }
    if(result)
    {
        F4_Index_ParseCtx_Inc(ctx, flags);
    }
    return result;
}

internal b32
F4_Index_RequireTokenKind(F4_Index_ParseCtx *ctx, Token_Base_Kind kind, Token **token_out, F4_Index_TokenSkipFlags flags)
{
    b32 result = 0;
    Token *token = token_it_read(&ctx->it);
    if(token)
    {
        if(token->kind == kind)
        {
            result = 1;
            if(token_out)
            {
                *token_out = token;
            }
        }
    }
    else
    {
        ctx->done = 1;
    }if(result)
    {
        F4_Index_ParseCtx_Inc(ctx, flags);
    }
    return result;
}

internal b32
F4_Index_RequireTokenSubKind(F4_Index_ParseCtx *ctx, int sub_kind, Token **token_out, F4_Index_TokenSkipFlags flags)
{
    b32 result = 0;
    Token *token = token_it_read(&ctx->it);
    if(token)
    {
        if(token->sub_kind == sub_kind)
        {
            result = 1;
            if(token_out)
            {
                *token_out = token;
            }
        }
    }
    else
    {
        ctx->done = 1;
    }if(result)
    {
        F4_Index_ParseCtx_Inc(ctx, flags);
    }
    return result;
}

internal b32
F4_Index_PeekToken(F4_Index_ParseCtx *ctx, String_Const_u8 string)
{
    b32 result = 0;
    Token *token = token_it_read(&ctx->it);
    if(token)
    {
        String_Const_u8 token_string =
            string_substring(ctx->string, Ii64(token->pos, token->pos+token->size));
        if(string_match(string, token_string))
        {
            result = 1;
        }
    }
    else
    {
        ctx->done = 1;
    }
    return result;
}

internal b32
F4_Index_PeekTokenKind(F4_Index_ParseCtx *ctx, Token_Base_Kind kind, Token **token_out)
{
    b32 result = 0;
    Token *token = token_it_read(&ctx->it);
    if(token)
    {
        if(token->kind == kind)
        {
            result = 1;
            if(token_out)
            {
                *token_out = token;
            }
        }
    }
    else
    {
        ctx->done = 1;
    }
    return result;
}

internal b32
F4_Index_PeekTokenSubKind(F4_Index_ParseCtx *ctx, int sub_kind, Token **token_out)
{
    b32 result = 0;
    Token *token = token_it_read(&ctx->it);
    if(token)
    {
        if(token->sub_kind == sub_kind)
        {
            result = 1;
            if(token_out)
            {
                *token_out = token;
            }
        }
    }
    else
    {
        ctx->done = 1;
    }
    return result;
}

internal void
F4_Index_SeekToken(F4_Index_ParseCtx *ctx, String_Const_u8 string)
{
    Token_Iterator_Array it_restore = ctx->it;
    for(;;)
    {
        Token *token = token_it_read(&ctx->it);
        if(token)
        {
            String_Const_u8 token_string = F4_Index_StringFromToken(ctx, token);
            if(string_match(token_string, string))
            {
                break;
            }
        }
        else
        {
            break;
        }
        if(!token_it_inc_all(&ctx->it))
        {
            break;
        }
    }
    ctx->it = it_restore;
}

internal void
F4_Index_ParseComment(F4_Index_ParseCtx *ctx, Token *token)
{
    String_Const_u8 string = F4_Index_StringFromToken(ctx, token);
    
    for(u64 i = 0; i < string.size; i += 1)
    {
        if(string.str[i] == '@')
        {
            F4_Index_MakeNote(ctx->app, ctx->file, 0, string_substring(string, Ii64(i, string.size)), F4_Index_NoteKind_CommentTag, 0, Ii64(token));
            break;
        }
        else if(i+4 < string.size && string_match(S8Lit("TODO"), string_substring(string, Ii64(i, i + 4))))
        {
            F4_Index_MakeNote(ctx->app, ctx->file, 0, string_substring(string, Ii64(i, string.size)), F4_Index_NoteKind_CommentToDo, 0, Ii64(token));
        }
    }
}

internal void
F4_Index_SkipSoftTokens(F4_Index_ParseCtx *ctx, b32 preproc)
{
    for(;!ctx->done;)
    {
        Token *token = token_it_read(&ctx->it);
        if(preproc)
        {
            if(!(token->flags & TokenBaseFlag_PreprocessorBody))
            {
                break;
            }
        }
        else
        {
            if(token->kind == TokenBaseKind_StatementClose ||
               token->kind == TokenBaseKind_ScopeOpen ||
               token->kind == TokenBaseKind_ParentheticalOpen)
            {
                break;
            }
        }
        if(!token_it_inc_non_whitespace(&ctx->it))
        {
            break;
        }
    }
}

internal void
F4_Index_Tick(Application_Links *app)
{
    Scratch_Block scratch(app);
    for (Buffer_Modified_Node *node = global_buffer_modified_set.first; node != 0;node = node->next)
    {
        Temp_Memory_Block temp(scratch);
        Buffer_ID buffer_id = node->buffer;
        
        String_Const_u8 contents = push_whole_buffer(app, scratch, buffer_id);
        Token_Array tokens = get_token_array_from_buffer(app, buffer_id);
        if(tokens.count == 0) { continue; }
        
        F4_Index_Lock();
        F4_Index_File *file = F4_Index_LookupOrMakeFile(app, buffer_id);
        if(file)
        {
            ProfileScope(app, "[f] reparse");
            F4_Index_ClearFile(file);
            F4_Index_ParseFile(app, file, contents, tokens);
        }
        F4_Index_Unlock();
        buffer_clear_layout_cache(app, buffer_id);
    }
}
