
//~ NOTE(rjf): @f4_base_commands

// TODO(rjf): Remove once Allen adds official version.
CUSTOM_COMMAND_SIG(f4_leave_event_unhandled)
CUSTOM_DOC("when bound to keystroke, ensures the event falls through to text insertion")
{
    leave_current_input_unhandled(app);
}

internal void
F4_Search(Application_Links *app, Scan_Direction dir)
{
    Scratch_Block scratch(app);
    View_ID view = get_active_view(app, Access_Read);
    Buffer_ID buffer = view_get_buffer(app, view, Access_Read);
    if(view && buffer)
    {
        i64 cursor = view_get_cursor_pos(app, view);
        i64 mark = view_get_mark_pos(app, view);
        i64 cursor_line = get_line_number_from_pos(app, buffer, cursor);
        i64 mark_line = get_line_number_from_pos(app, buffer, mark);
        String_Const_u8 query_init = (fcoder_mode != FCoderMode_NotepadLike || cursor == mark || cursor_line != mark_line) ? SCu8() : push_buffer_range(app, scratch, buffer, Ii64(cursor, mark));
        isearch(app, dir, cursor, query_init);
    }
}

CUSTOM_COMMAND_SIG(f4_search)
CUSTOM_DOC("Searches the current buffer forward. If something is highlighted, will fill search query with it.")
{
    F4_Search(app, Scan_Forward);
}

CUSTOM_COMMAND_SIG(f4_reverse_search)
CUSTOM_DOC("Searches the current buffer backwards. If something is highlighted, will fill search query with it.")
{
    F4_Search(app, Scan_Backward);
}

CUSTOM_COMMAND_SIG(f4_write_text_input)
CUSTOM_DOC("Inserts whatever text was used to trigger this command.")
{
    write_text_input(app);
    F4_PowerMode_CharacterPressed();
    User_Input in = get_current_input(app);
    String_Const_u8 insert = to_writable(&in);
    F4_PowerMode_Spawn(app, get_active_view(app, Access_ReadWriteVisible), insert.str ? insert.str[0] : 0);
}

CUSTOM_COMMAND_SIG(f4_write_text_and_auto_indent)
CUSTOM_DOC("Inserts text and auto-indents the line on which the cursor sits if any of the text contains 'layout punctuation' such as ;:{}()[]# and new lines.")
{
    write_text_and_auto_indent(app);
    F4_PowerMode_CharacterPressed();
    User_Input in = get_current_input(app);
    String_Const_u8 insert = to_writable(&in);
    F4_PowerMode_Spawn(app, get_active_view(app, Access_ReadWriteVisible), insert.str ? insert.str[0] : 0);
}

CUSTOM_COMMAND_SIG(f4_write_zero_struct)
CUSTOM_DOC("At the cursor, insert a ' = {0};'.")
{
    write_string(app, string_u8_litexpr(" = {0};"));
    F4_PowerMode_CharacterPressed();
    F4_PowerMode_Spawn(app, get_active_view(app, Access_ReadWriteVisible), 0);
}

CUSTOM_COMMAND_SIG(f4_home)
CUSTOM_DOC("Goes to the beginning of the line.")
{
    seek_pos_of_visual_line(app, Side_Min);
    View_ID view = get_active_view(app, Access_ReadWriteVisible);
    Buffer_Scroll scroll = view_get_buffer_scroll(app, view);
    scroll.target.pixel_shift.x = 0;
    view_set_buffer_scroll(app, view, scroll, SetBufferScroll_NoCursorChange);
}

CUSTOM_COMMAND_SIG(f4_toggle_battery_saver)
CUSTOM_DOC("Toggles battery saving mode.")
{
    global_battery_saver = !global_battery_saver;
}

CUSTOM_COMMAND_SIG(f4_toggle_compilation_expand)
CUSTOM_DOC("Expand the compilation window.")
{
    Buffer_ID buffer = view_get_buffer(app, global_compilation_view, Access_Always);
    Face_ID face_id = get_face_id(app, buffer);
    Face_Metrics metrics = get_face_metrics(app, face_id);
    if(global_compilation_view_expanded ^= 1)
    {
        view_set_split_pixel_size(app, global_compilation_view, (i32)(metrics.line_height*32.f));
    }
    else
    {
        view_set_split_pixel_size(app, global_compilation_view, (i32)(metrics.line_height*4.f));
    }
}

internal void
F4_GoToDefinition(Application_Links *app, F4_Index_Note *note, b32 same_panel)
{
    if(note != 0 && note->file != 0)
    {
        View_ID view = get_active_view(app, Access_Always);
        Rect_f32 region = view_get_buffer_region(app, view);
        f32 view_height = rect_height(region);
        Buffer_ID buffer = note->file->buffer;
        if(!same_panel)
        {
            view = get_next_view_looped_primary_panels(app, view, Access_Always);
        }
        point_stack_push_view_cursor(app, view);
        view_set_buffer(app, view, buffer, 0);
        i64 line_number = get_line_number_from_pos(app, buffer, note->range.min);
        Buffer_Scroll scroll = view_get_buffer_scroll(app, view);
        scroll.position.line_number = line_number;
        scroll.target.line_number = line_number;
        scroll.position.pixel_shift.y = scroll.target.pixel_shift.y = -view_height*0.5f;
        view_set_buffer_scroll(app, view, scroll, SetBufferScroll_SnapCursorIntoView);
        view_set_cursor(app, view, seek_pos(note->range.min));
        view_set_mark(app, view, seek_pos(note->range.min));
    }
}

internal F4_Index_Note *
F4_FindMostIntuitiveNoteInDuplicateChain(F4_Index_Note *note, Buffer_ID cursor_buffer, i64 cursor_pos)
{
    F4_Index_Note *result = note;
    if(note != 0)
    {
        F4_Index_Note *best_note_based_on_cursor = 0;
        for(F4_Index_Note *candidate = note; candidate; candidate = candidate->next)
        {
            F4_Index_File *file = candidate->file;
            if(file != 0)
            {
                if(cursor_buffer == file->buffer &&
                   candidate->range.min <= cursor_pos && cursor_pos <= candidate->range.max)
                {
                    if(candidate->next)
                    {
                        best_note_based_on_cursor = candidate->next;
                        break;
                    }
                    else
                    {
                        best_note_based_on_cursor = note;
                        break;
                    }
                }
            }
        }
        
        if(best_note_based_on_cursor)
        {
            result = best_note_based_on_cursor;
        }
        else if(note->flags & F4_Index_NoteFlag_Prototype)
        {
            for(F4_Index_Note *candidate = note; candidate; candidate = candidate->next)
            {
                if(!(candidate->flags & F4_Index_NoteFlag_Prototype))
                {
                    result = candidate;
                    break;
                }
            }
        }
    }
    return result;
}

CUSTOM_COMMAND_SIG(f4_go_to_definition)
CUSTOM_DOC("Goes to the definition of the identifier under the cursor.")
{
    View_ID view = get_active_view(app, Access_Always);
    Buffer_ID buffer = view_get_buffer(app, view, Access_Always);
    Scratch_Block scratch(app);
    String_Const_u8 string = push_token_or_word_under_active_cursor(app, scratch);
    F4_Index_Note *note = F4_Index_LookupNote(string);
    note = F4_FindMostIntuitiveNoteInDuplicateChain(note, buffer, view_get_cursor_pos(app, view));
    F4_GoToDefinition(app, note, 0);
}

CUSTOM_COMMAND_SIG(f4_go_to_definition_same_panel)
CUSTOM_DOC("Goes to the definition of the identifier under the cursor in the same panel.")
{
    View_ID view = get_active_view(app, Access_Always);
    Buffer_ID buffer = view_get_buffer(app, view, Access_Always);
    Scratch_Block scratch(app);
    String_Const_u8 string = push_token_or_word_under_active_cursor(app, scratch);
    F4_Index_Note *note = F4_Index_LookupNote(string);
    note = F4_FindMostIntuitiveNoteInDuplicateChain(note, buffer, view_get_cursor_pos(app, view));
    F4_GoToDefinition(app, note, 1);
}

internal void
_F4_PushListerOptionForNote(Application_Links *app, Arena *arena, Lister *lister, F4_Index_Note *note)
{
    if(note && note->file)
    {
        F4_Index_File *file = note->file;
        Buffer_ID buffer = file->buffer;
        
        Tiny_Jump *jump = push_array(arena, Tiny_Jump, 1);
        jump->buffer = buffer;
        jump->pos = note->range.first;
        
        String_Const_u8 buffer_name = push_buffer_unique_name(app, arena, buffer);
        String_Const_u8 name = push_stringf(arena, "[%.*s] %.*s", string_expand(buffer_name), string_expand(note->string));
        String_Const_u8 sort = S8Lit("");
        switch(note->kind)
        {
            case F4_Index_NoteKind_Type:
            {
                sort = push_stringf(arena, "type [%s] [%s]",
                                    note->flags & F4_Index_NoteFlag_Prototype ? "prototype" : "def",
                                    note->flags & F4_Index_NoteFlag_SumType ? "sum" : "product");
            }break;
            
            case F4_Index_NoteKind_Function:
            {
                sort = push_stringf(arena, "function [%s]", note->flags & F4_Index_NoteFlag_Prototype ? "prototype" : "def");
            }break;
            
            case F4_Index_NoteKind_Macro:
            {
                sort = S8Lit("macro");
            }break;
            
            case F4_Index_NoteKind_Constant:
            {
                sort = S8Lit("constant");
            }break;
            
            case F4_Index_NoteKind_CommentTag:
            {
                sort = S8Lit("comment tag");
            }break;
            
            case F4_Index_NoteKind_CommentToDo:
            {
                sort = S8Lit("TODO");
            }break;
            
            default: break;
        }
        lister_add_item(lister, name, sort, jump, 0);
    }
}

internal void
F4_JumpToLocation(Application_Links *app, View_ID view, Buffer_ID buffer, i64 pos)
{
    // NOTE(rjf): This function was ripped from 4coder's jump_to_location. It was copied
    // and modified so that jumping to a location didn't cause a selection in notepad-like
    // mode.
    
    view_set_active(app, view);
    Buffer_Seek seek = seek_pos(pos);
    set_view_to_location(app, view, buffer, seek);
    
    if (auto_center_after_jumps)
    {
        center_view(app);
    }
    view_set_cursor(app, view, seek);
    view_set_mark(app, view, seek);
}

CUSTOM_UI_COMMAND_SIG(f4_search_for_definition__project_wide)
CUSTOM_DOC("List all definitions in the index and jump to the one selected by the user.")
{
    char *query = "Index (Project):";
    
    Scratch_Block scratch(app);
    Lister_Block lister(app, scratch);
    lister_set_query(lister, query);
    lister_set_default_handlers(lister);
    
    F4_Index_Lock();
    {
        for (Buffer_ID buffer = get_buffer_next(app, 0, Access_Always);
             buffer != 0; buffer = get_buffer_next(app, buffer, Access_Always))
        {
            F4_Index_File *file = F4_Index_LookupFile(app, buffer);
            if(file != 0)
            {
                for(F4_Index_Note *note = file->first_note; note; note = note->next_sibling)
                {
                    _F4_PushListerOptionForNote(app, scratch, lister, note);
                }
            }
        }
    }
    F4_Index_Unlock();
    
    Lister_Result l_result = run_lister(app, lister);
    Tiny_Jump result = {};
    if (!l_result.canceled && l_result.user_data != 0){
        block_copy_struct(&result, (Tiny_Jump*)l_result.user_data);
    }
    
    if (result.buffer != 0)
    {
        View_ID view = get_this_ctx_view(app, Access_Always);
        point_stack_push_view_cursor(app, view);
        F4_JumpToLocation(app, view, result.buffer, result.pos);
    }
}

CUSTOM_UI_COMMAND_SIG(f4_search_for_definition__current_file)
CUSTOM_DOC("List all definitions in the current file and jump to the one selected by the user.")
{
    char *query = "Index (File):";
    
    View_ID view = get_active_view(app, Access_Always);
    Buffer_ID buffer = view_get_buffer(app, view, Access_Always);
    
    Scratch_Block scratch(app);
    Lister_Block lister(app, scratch);
    lister_set_query(lister, query);
    lister_set_default_handlers(lister);
    
    F4_Index_Lock();
    {
        F4_Index_File *file = F4_Index_LookupFile(app, buffer);
        if(file != 0)
        {
            for(F4_Index_Note *note = file->first_note; note; note = note->next_sibling)
            {
                _F4_PushListerOptionForNote(app, scratch, lister, note);
            }
        }
    }
    F4_Index_Unlock();
    
    Lister_Result l_result = run_lister(app, lister);
    Tiny_Jump result = {};
    if (!l_result.canceled && l_result.user_data != 0){
        block_copy_struct(&result, (Tiny_Jump*)l_result.user_data);
    }
    
    if (result.buffer != 0)
    {
        View_ID view_id = get_this_ctx_view(app, Access_Always);
        point_stack_push_view_cursor(app, view_id);
        F4_JumpToLocation(app, view_id, result.buffer, result.pos);
    }
}

CUSTOM_COMMAND_SIG(f4_toggle_enclosure_side)
CUSTOM_DOC("Moves the cursor between the open/close brace/paren/bracket of the closest enclosure.")
{
    View_ID view = get_active_view(app, Access_Always);
    Buffer_ID buffer = view_get_buffer(app, view, Access_Always);
    i64 pos = view_get_cursor_pos(app, view);
    
    // NOTE(rjf): Adjust position if it's on the start or end of an enclosure.
    {
        Token_Array tokens = get_token_array_from_buffer(app, buffer);
        
        Token_Iterator_Array it = token_iterator_pos(0, &tokens, pos);
        Token *token = token_it_read(&it);
        if(token)
        {
            if(token->kind == TokenBaseKind_ScopeOpen ||
               token->kind == TokenBaseKind_ParentheticalOpen)
            {
                pos += 1;
                goto end;
            }
        }
        
        token_it_dec_all(&it);
        token = token_it_read(&it);
        if(token)
        {
            if(token->kind == TokenBaseKind_ScopeClose ||
               token->kind == TokenBaseKind_ParentheticalClose)
            {
                pos -= 1;
                goto end;
            }
        }
        
        end:;
    }
    
    Scratch_Block scratch(app);
    Range_i64_Array ranges = get_enclosure_ranges(app, scratch, buffer, pos,
                                                  FindNest_Scope | FindNest_Paren);
    if(ranges.count > 0)
    {
        Range_i64 nearest_range = ranges.ranges[0];
        if(pos == nearest_range.min+1)
        {
            pos = nearest_range.max;
        }
        else
        {
            pos = nearest_range.min;
        }
        view_set_cursor(app, view, seek_pos(pos));
        no_mark_snap_to_cursor_if_shift(app, view);
    }
}

CUSTOM_UI_COMMAND_SIG(f4_open_project)
CUSTOM_DOC("Open a project by navigating to the project file.")
{
    for(;;)
    {
        Scratch_Block scratch(app);
        View_ID view = get_this_ctx_view(app, Access_Always);
        File_Name_Result result = get_file_name_from_user(app, scratch, "Open Project:", view);
        if (result.canceled) break;
        
        String_Const_u8 file_name = result.file_name_activated;
        if (file_name.size == 0)
        {
            file_name = result.file_name_in_text_field;
        }
        if (file_name.size == 0) break;
        
        String_Const_u8 path = result.path_in_text_field;
        String_Const_u8 full_file_name = push_u8_stringf(scratch, "%.*s/%.*s",
                                                         string_expand(path), string_expand(file_name));
        
        if (result.is_folder)
        {
            set_hot_directory(app, full_file_name);
            continue;
        }
        
        if(character_is_slash(file_name.str[file_name.size - 1]))
        {
            File_Attributes attribs = system_quick_file_attributes(scratch, full_file_name);
            if (HasFlag(attribs.flags, FileAttribute_IsDirectory)){
                set_hot_directory(app, full_file_name);
                continue;
            }
            if (string_looks_like_drive_letter(file_name)){
                set_hot_directory(app, file_name);
                continue;
            }
            if (query_create_folder(app, file_name)){
                set_hot_directory(app, full_file_name);
                continue;
            }
            break;
        }
        
        set_hot_directory(app, full_file_name);
        load_project(app);
        break;
    }
}

CUSTOM_COMMAND_SIG(f4_setup_new_project)
CUSTOM_DOC("Sets up a blank 4coder project provided some user folder.")
{
    Scratch_Block scratch(app);
    Query_Bar_Group bar_group(app);
    
    // NOTE(rjf): Query user for project folder.
    u8 project_folder_absolute[1024];
    {
        Query_Bar path_bar = {};
        path_bar.prompt = string_u8_litexpr("Absolute Path To Project Folder: ");
        path_bar.string = SCu8(project_folder_absolute, (u64)0);
        path_bar.string_capacity = sizeof(project_folder_absolute);
        if(query_user_string(app, &path_bar))
        {
            String_Const_u8 full_file_name = push_u8_stringf(scratch, "%.*s/",
                                                             string_expand(path_bar.string));
            set_hot_directory(app, full_file_name);
            
            String_Const_u8 project_file_path = push_u8_stringf(scratch, "%.*s/project.4coder", string_expand(path_bar.string));
            FILE *file = fopen((char *)project_file_path.str, "wb");
            if(file)
            {
                
                char *string = R"PROJ(version(1);
                  
                  project_name = "New Project";
                  
                  patterns =
                  {
                      "*.c",
                      "*.cpp",
                      "*.jai",
                      "*.odin",
                      "*.zig",
                      "*.h",
                      "*.inc",
                      "*.bat",
                      "*.sh",
                      "*.4coder",
                      "*.txt",
                  };
                  
                  blacklist_patterns =
                  {
                      ".*",
                  };
                  
                  load_paths =
                  {
                      {
                          { {"."}, .recursive = true, .relative = true }, .os = "win"
                      },
                  };
                  
                  command_list =
                  {
                      {
                          .name = "build",
                          .out = "*compilation*",
                          .footer_panel = true,
                          .save_dirty_files = true,
                          .cursor_at_end = false,
                          .cmd =
                          {
                              { "echo Windows build command not implemented for 4coder project.", .os = "win" },
        { "echo Linux build command not implemented for 4coder project.", .os = "linux" },
                          },
                      },
                      
                      {
                          .name = "run",
                          .out = "*compilation*",
                          .footer_panel = true,
                          .save_dirty_files = true,
                          .cursor_at_end = false,
                          .cmd =
                          {
                              { "echo Windows run command not implemented for 4coder project.", .os = "win" },
        { "echo Linux run command not implemented for 4coder project.", .os = "linux" },
                          },
                      },
                      
                  };
                  
                  fkey_command[1] = "build";
                  fkey_command[2] = "run";
        )PROJ";
                
                fprintf(file, "%s", string);
                fclose(file);
                load_project(app);
            }
            else
            {
                // TODO(rjf): Error.
            }
        }
    }
    
    load_project(app);
}

function i64
F4_Boundary_TokenAndWhitespace(Application_Links *app, Buffer_ID buffer, 
                               Side side, Scan_Direction direction, i64 pos)
{
    i64 result = boundary_non_whitespace(app, buffer, side, direction, pos);
    Token_Array tokens = get_token_array_from_buffer(app, buffer);
    if (tokens.tokens != 0){
        switch (direction){
            case Scan_Forward:
            {
                i64 buffer_size = buffer_get_size(app, buffer);
                result = buffer_size;
                if(tokens.count > 0)
                {
                    Token_Iterator_Array it = token_iterator_pos(0, &tokens, pos);
                    Token *token = token_it_read(&it);
                    
                    if(token == 0)
                    {
                        break;
                    }
                    
                    // NOTE(rjf): Comments/Strings
                    if(token->kind == TokenBaseKind_Comment ||
                       token->kind == TokenBaseKind_LiteralString)
                    {
                        result = boundary_non_whitespace(app, buffer, side, direction, pos);
                        break;
                    }
                    
                    // NOTE(rjf): All other cases.
                    else
                    {
                        if (token->kind == TokenBaseKind_Whitespace)
                        {
                            // token_it_inc_non_whitespace(&it);
                            // token = token_it_read(&it);
                        }
                        
                        if (side == Side_Max){
                            result = token->pos + token->size;
                            
                            token_it_inc_all(&it);
                            Token *ws = token_it_read(&it);
                            if(ws != 0 && ws->kind == TokenBaseKind_Whitespace &&
                               get_line_number_from_pos(app, buffer, ws->pos + ws->size) ==
                               get_line_number_from_pos(app, buffer, token->pos))
                            {
                                result = ws->pos + ws->size;
                            }
                        }
                        else{
                            if (token->pos <= pos){
                                token_it_inc_non_whitespace(&it);
                                token = token_it_read(&it);
                            }
                            if (token != 0){
                                result = token->pos;
                            }
                        }
                    }
                    
                }
            }break;
            
            case Scan_Backward:
            {
                result = 0;
                if (tokens.count > 0){
                    Token_Iterator_Array it = token_iterator_pos(0, &tokens, pos);
                    Token *token = token_it_read(&it);
                    
                    Token_Iterator_Array it2 = it;
                    token_it_dec_non_whitespace(&it2);
                    Token *token2 = token_it_read(&it2);
                    
                    // NOTE(rjf): Comments/Strings
                    if(token->kind == TokenBaseKind_Comment ||
                       token->kind == TokenBaseKind_LiteralString ||
                       (token2 && 
                        token2->kind == TokenBaseKind_Comment ||
                        token2->kind == TokenBaseKind_LiteralString))
                    {
                        result = boundary_non_whitespace(app, buffer, side, direction, pos);
                        break;
                    }
                    
                    if (token->kind == TokenBaseKind_Whitespace){
                        token_it_dec_non_whitespace(&it);
                        token = token_it_read(&it);
                    }
                    if (token != 0){
                        if (side == Side_Min){
                            if (token->pos >= pos){
                                token_it_dec_non_whitespace(&it);
                                token = token_it_read(&it);
                            }
                            result = token->pos;
                        }
                        else{
                            if (token->pos + token->size >= pos){
                                token_it_dec_non_whitespace(&it);
                                token = token_it_read(&it);
                            }
                            result = token->pos + token->size;
                        }
                    }
                }
            }break;
        }
    }
    return(result);
}

function i64
F4_Boundary_CursorTokenOrBlankLine(Application_Links *app, Buffer_ID buffer, 
                                   Side side, Scan_Direction direction, i64 pos)
{
    Scratch_Block scratch(app);
    
    Range_i64_Array scopes = get_enclosure_ranges(app, scratch, buffer, pos, FindNest_Scope);
    // NOTE(jack): The outermost scope
    Range_i64 outer_scope = scopes.ranges[scopes.count - 1];
    
    // NOTE(jack): As we are issuing a move command here I will assume that buffer is the active buffer.
    View_ID view = get_active_view(app, Access_Always);
    i64 active_cursor_pos = view_get_cursor_pos(app, view);
    Token_Array tokens = get_token_array_from_buffer(app, buffer);
    Token_Iterator_Array active_cursor_it = token_iterator_pos(0, &tokens, active_cursor_pos);
    Token *active_cursor_token = token_it_read(&active_cursor_it);
    
    String_Const_u8 cursor_string = push_buffer_range(app, scratch, buffer, Ii64(active_cursor_token));
    i64 cursor_offset = pos - active_cursor_token->pos;
    
    // NOTE(jack): If the cursor token is not an identifier, we will move to empty lines
    i64 result = get_pos_of_blank_line_grouped(app, buffer, direction, pos);
    result = view_get_character_legal_pos_from_pos(app, view, result);
    if (tokens.tokens != 0)
    {
        // NOTE(jack): if the the cursor token is an identifier, and we are inside of a scope
        // perform the cursor occurance movement.
        if (active_cursor_token->kind == TokenBaseKind_Identifier && !(scopes.count == 0))
        {
            // NOTE(jack): Reset result to prevent token movement to escape to blank line movement
            // when you are on the first/last token in the outermost scope.
            result = pos;
            Token_Iterator_Array it = token_iterator_pos(0, &tokens, pos);
            
            for (;;)
            {
                b32 done = false;
                // NOTE(jack): Incremenet first so we dont move to the same cursor that the cursor is on.
                switch (direction)
                {
                    // NOTE(jack): I am using it.ptr->pos because its easier than reading the token with
                    // token_it_read
                    case Scan_Forward:
                    {
                        if (!token_it_inc_non_whitespace(&it) || it.ptr->pos >= outer_scope.end) {
                            done = true;
                        }
                    } break;
                    
                    case Scan_Backward:
                    {
                        if (!token_it_dec_non_whitespace(&it) || it.ptr->pos < outer_scope.start) {
                            done = true;
                        }
                    } break;
                }
                
                if (!done) 
                {
                    Token *token = token_it_read(&it);
                    String_Const_u8 token_string = push_buffer_range(app, scratch, buffer, Ii64(token));
                    if (string_match(cursor_string, token_string)) {
                        result = token->pos + cursor_offset;
                        break;
                    }
                }
                else 
                {
                    break;
                }
            }
        }
    }
    
    return result ;
}

CUSTOM_COMMAND_SIG(f4_move_left)
CUSTOM_DOC("Moves the cursor one character to the left.")
{
    Scratch_Block scratch(app);
    Input_Modifier_Set mods = system_get_keyboard_modifiers(scratch);
    View_ID view = get_active_view(app, Access_ReadVisible);
    if(fcoder_mode != FCoderMode_NotepadLike || view_get_cursor_pos(app, view) == view_get_mark_pos(app, view) ||
       has_modifier(&mods, KeyCode_Shift))
    {
        view_set_cursor_by_character_delta(app, view, -1);
    }
    no_mark_snap_to_cursor_if_shift(app, view);
}

CUSTOM_COMMAND_SIG(f4_move_right)
CUSTOM_DOC("Moves the cursor one character to the right.")
{
    Scratch_Block scratch(app);
    Input_Modifier_Set mods = system_get_keyboard_modifiers(scratch);
    View_ID view = get_active_view(app, Access_ReadVisible);
    if(fcoder_mode != FCoderMode_NotepadLike || view_get_cursor_pos(app, view) == view_get_mark_pos(app, view) ||
       has_modifier(&mods, KeyCode_Shift))
    {
        view_set_cursor_by_character_delta(app, view, +1);
    }
    no_mark_snap_to_cursor_if_shift(app, view);
}

CUSTOM_COMMAND_SIG(f4_move_up_token_occurrence)
CUSTOM_DOC("Moves the cursor to the previous occurrence of the token that the cursor is over.")
{
    Scratch_Block scratch(app);
    current_view_scan_move(app, Scan_Backward, push_boundary_list(scratch, F4_Boundary_CursorTokenOrBlankLine_TEST));
}

CUSTOM_COMMAND_SIG(f4_move_down_token_occurrence)
CUSTOM_DOC("Moves the cursor to the next occurrence of the token that the cursor is over.")
{
    Scratch_Block scratch(app);
    current_view_scan_move(app, Scan_Forward, push_boundary_list(scratch, F4_Boundary_CursorTokenOrBlankLine_TEST));
}

CUSTOM_COMMAND_SIG(f4_move_right_token_boundary)
CUSTOM_DOC("Seek right for boundary between alphanumeric characters and non-alphanumeric characters.")
{
    Scratch_Block scratch(app);
    current_view_scan_move(app, Scan_Forward, push_boundary_list(scratch, F4_Boundary_TokenAndWhitespace));
}

CUSTOM_COMMAND_SIG(f4_move_left_token_boundary)
CUSTOM_DOC("Seek left for boundary between alphanumeric characters and non-alphanumeric characters.")
{
    Scratch_Block scratch(app);
    current_view_scan_move(app, Scan_Backward, push_boundary_list(scratch, F4_Boundary_TokenAndWhitespace));
}

CUSTOM_COMMAND_SIG(f4_move_up_token_occurance)
CUSTOM_DOC("Moves the cusror to the previoud")
{
    Scratch_Block scratch(app);
    current_view_scan_move(app, Scan_Backward, push_boundary_list(scratch, F4_Boundary_CursorTokenOrBlankLine));
}

CUSTOM_COMMAND_SIG(f4_move_down_token_occurance)
CUSTOM_DOC("Moves the cursor one charcter to the right.")
{
    Scratch_Block scratch(app);
    current_view_scan_move(app, Scan_Forward, push_boundary_list(scratch, F4_Boundary_CursorTokenOrBlankLine));
}

CUSTOM_COMMAND_SIG(f4_backspace_token_boundary)
CUSTOM_DOC("Deletes left to a token boundary.")
{
    Scratch_Block scratch(app);
    current_view_boundary_delete(app, Scan_Backward, push_boundary_list(scratch, F4_Boundary_TokenAndWhitespace));
}

CUSTOM_COMMAND_SIG(f4_delete_token_boundary)
CUSTOM_DOC("Deletes right to a token boundary.")
{
    Scratch_Block scratch(app);
    current_view_boundary_delete(app, Scan_Forward, push_boundary_list(scratch, F4_Boundary_TokenAndWhitespace));
}

CUSTOM_COMMAND_SIG(f4_backspace_alpha_numeric_or_camel_boundary)
CUSTOM_DOC("Deletes left to a alphanumeric or camel boundary.")
{
    Scratch_Block scratch(app);
    current_view_boundary_delete(app, Scan_Backward, push_boundary_list(scratch,
                                                                        boundary_alpha_numeric,
                                                                        boundary_alpha_numeric_camel));
}

CUSTOM_COMMAND_SIG(f4_delete_alpha_numeric_or_camel_boundary)
CUSTOM_DOC("Deletes right to an alphanumeric or camel boundary.")
{
    Scratch_Block scratch(app);
    current_view_boundary_delete(app, Scan_Forward, push_boundary_list(scratch,
                                                                       boundary_alpha_numeric,
                                                                       boundary_alpha_numeric_camel));
}

CUSTOM_COMMAND_SIG(f4_home_first_non_whitespace)
CUSTOM_DOC("Goes to the beginning of the line.")
{
    View_ID view = get_active_view(app, Access_Read);
    Buffer_ID buffer = view_get_buffer(app, view, Access_Read);
    if(view && buffer)
    {
        i64 start_pos = view_get_cursor_pos(app, view);
        seek_pos_of_visual_line(app, Side_Min);
        i64 end_pos = view_get_cursor_pos(app, view);
        i64 line = get_line_number_from_pos(app, buffer, start_pos);
        
        // NOTE(rjf): If we are on the first column, go to the first non-whitespace
        // in the line.
        if(start_pos == end_pos && start_pos == get_line_start_pos(app, buffer, line))
        {
            Scratch_Block scratch(app);
            String_Const_u8 string = push_buffer_line(app, scratch, buffer, line);
            for(u64 i = 0; i < string.size; i += 1)
            {
                if(!character_is_whitespace(string.str[i]))
                {
                    view_set_cursor_by_character_delta(app, view, (i64)i);
                    break;
                }
            }
        }
        
        // NOTE(rjf): If we hit any non-whitespace, move to the first possible
        // non-whitespace instead of the front of the line entirely.
        else 
        {
            Scratch_Block scratch(app);
            String_Const_u8 string = push_buffer_range(app, scratch, buffer, Ii64(start_pos, end_pos));
            
            b32 skipped_non_whitespace = 0;
            {
                for(i64 i = string.size-1; i >= 0; i -= 1)
                {
                    if(!character_is_whitespace(string.str[i]))
                    {
                        skipped_non_whitespace = 1;
                        break;
                    }
                }
            }
            
            if(skipped_non_whitespace)
            {
                for(i64 i = 0; i < (i64)string.size; i += 1)
                {
                    if(!character_is_whitespace(string.str[i]))
                    {
                        view_set_cursor_by_character_delta(app, view, i);
                        break;
                    }
                }
            }
        }
        
        // NOTE(rjf): Scroll all the way left.
        {
            Buffer_Scroll scroll = view_get_buffer_scroll(app, view);
            scroll.target.pixel_shift.x = 0;
            view_set_buffer_scroll(app, view, scroll, SetBufferScroll_NoCursorChange);
        }
    }
}

function void
F4_ReIndentLine(Application_Links *app, Buffer_ID buffer, i64 line, i64 indent_delta)
{
    Scratch_Block scratch(app);
    String_Const_u8 line_string = push_buffer_line(app, scratch, buffer, line);
    i64 line_start_pos = get_line_start_pos(app, buffer, line);
    
    Range_i64 line_indent_range = Ii64(0, 0);
    i64 tabs_at_beginning = 0;
    i64 spaces_at_beginning = 0;
    for(u64 i = 0; i < line_string.size; i += 1)
    {
        if(line_string.str[i] == '\t')
        {
            tabs_at_beginning += 1;
        }
        else if(character_is_whitespace(line_string.str[i]))
        {
            spaces_at_beginning += 1;
        }
        else if(!character_is_whitespace(line_string.str[i]))
        {
            line_indent_range.max = (i64)i;
            break;
        }
    }
    
    // NOTE(rjf): Indent lines.
    {
        Range_i64 indent_range =
        {
            line_indent_range.min + line_start_pos,
            line_indent_range.max + line_start_pos,
        };
        
        i64 indent_width = (i64)def_get_config_u64(app, vars_save_string_lit("indent_width"));
        b32 indent_with_tabs = def_get_config_b32(vars_save_string_lit("indent_with_tabs"));
        i64 spaces_per_indent_level = indent_width;
        i64 indent_level = spaces_at_beginning / spaces_per_indent_level + tabs_at_beginning;
        i64 new_indent_level = indent_level + indent_delta;
        
        String_Const_u8 indent_string = indent_with_tabs ? S8Lit("\t") : push_stringf(scratch, "%.*s", Min(indent_width, 16),
                                                                                      "                ");
        buffer_replace_range(app, buffer, indent_range, S8Lit(""));
        for(i64 i = 0; i < new_indent_level; i += 1)
        {
            buffer_replace_range(app, buffer, Ii64(line_start_pos), indent_string);
        }
    }
    
}

internal void
F4_ReIndentLineRange(Application_Links *app, Buffer_ID buffer, Range_i64 range, i64 indent_delta)
{
    for(i64 i = range.min; i <= range.max; i += 1)
    {
        F4_ReIndentLine(app, buffer, i, indent_delta);
    }
}

internal Range_i64
F4_LineRangeFromPosRange(Application_Links *app, Buffer_ID buffer, Range_i64 pos_range)
{
    Range_i64 lines_range =
        Ii64(get_line_number_from_pos(app, buffer, pos_range.min),
             get_line_number_from_pos(app, buffer, pos_range.max));
    return lines_range;
}

internal Range_i64
F4_PosRangeFromLineRange(Application_Links *app, Buffer_ID buffer, Range_i64 line_range)
{
    if(line_range.min > line_range.max)
    {
        i64 swap = line_range.max;
        line_range.max = line_range.min;
        line_range.min = swap;
    }
    Range_i64 pos_range =
        Ii64(get_line_start_pos(app, buffer, line_range.min),
             get_line_end_pos(app, buffer, line_range.max));
    return pos_range;
}

internal void
F4_ReIndentPosRange(Application_Links *app, Buffer_ID buffer, Range_i64 range, i64 indent_delta)
{
    F4_ReIndentLineRange(app, buffer,
                         F4_LineRangeFromPosRange(app, buffer, range),
                         indent_delta);
}

internal void
F4_AdjustCursorAndMarkForIndentation(Application_Links *app, View_ID view, i64 original_cursor, i64 original_mark, Range_i64 original_line_range)
{
    Buffer_ID buffer = view_get_buffer(app, view, Access_Read);
    Scratch_Block scratch(app);
    if(original_cursor == original_mark)
    {
        i64 start_pos = get_line_start_pos(app, buffer, original_line_range.min);
        i64 new_pos = start_pos;
        String_Const_u8 line = push_buffer_line(app, scratch, buffer, original_line_range.min);
        for(u64 i = 0; i < line.size; i += 1)
        {
            if(!character_is_whitespace(line.str[i]))
            {
                new_pos = start_pos + (i64)i;
                break;
            }
        }
        
        view_set_cursor(app, view, seek_pos(new_pos));
        view_set_mark(app, view, seek_pos(new_pos));
    }
    else
    {
        Range_i64 range = F4_PosRangeFromLineRange(app, buffer, original_line_range);
        view_set_cursor(app, view, seek_pos(original_cursor > original_mark ? range.max : range.min));
        view_set_mark(app, view, seek_pos(original_cursor > original_mark ? range.min : range.max));
    }
}

CUSTOM_COMMAND_SIG(f4_autocomplete_or_indent)
CUSTOM_DOC("Tries to autocomplete the word currently being typed, and inserts indentation if such a word is not found.")
{
    ProfileScope(app, "[F4] Word Complete");
    
    View_ID view = get_active_view(app, Access_ReadWriteVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible);
    
    if(buffer != 0)
    {
        Managed_Scope scope = view_get_managed_scope(app, view);
        
        b32 first_completion = false;
        Rewrite_Type *rewrite = scope_attachment(app, scope, view_rewrite_loc, Rewrite_Type);
        if (*rewrite != Rewrite_WordComplete){
            first_completion = true;
        }
        
        set_next_rewrite(app, view, Rewrite_WordComplete);
        
        Word_Complete_Iterator *it = word_complete_get_shared_iter(app);
        local_persist b32 initialized = false;
        local_persist Range_i64 range = {};
        
        if(first_completion || !initialized)
        {
            ProfileScope(app, "[F4] Word Complete State Init");
            initialized = false;
            i64 pos = view_get_cursor_pos(app, view);
            Range_i64 needle_range = get_word_complete_needle_range(app, buffer, pos);
            if(range_size(needle_range) > 0)
            {
                initialized = true;
                range = needle_range;
                word_complete_iter_init(buffer, needle_range, it);
            }
        }
        
        // NOTE(rjf): Word-Complete
        if(initialized)
        {
            ProfileScope(app, "[F4] Word Complete Apply");
            
            word_complete_iter_next(it);
            String_Const_u8 str = word_complete_iter_read(it);
            
            buffer_replace_range(app, buffer, range, str);
            
            range.max = range.min + str.size;
            view_set_cursor_and_preferred_x(app, view, seek_pos(range.max));
        }
        
        // NOTE(rjf): Insert indentation if autocomplete failed
        else if(initialized == 0)
        {
            i64 pos = view_get_cursor_pos(app, view);
            i64 mark = view_get_mark_pos(app, view);
            Range_i64 pos_range = Ii64(pos, mark);
            Range_i64 line_range = F4_LineRangeFromPosRange(app, buffer, pos_range);
            
            History_Group group = history_group_begin(app, buffer);
            F4_ReIndentPosRange(app, buffer, Ii64(pos, mark), +1);
            F4_AdjustCursorAndMarkForIndentation(app, view, pos, mark, line_range);
            history_group_end(group);
            no_mark_snap_to_cursor(app, view);
        }
    }
}

CUSTOM_COMMAND_SIG(f4_unindent)
CUSTOM_DOC("Unindent the selected range.")
{
    Scratch_Block scratch(app);
    
    View_ID view = get_active_view(app, Access_ReadWrite);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWrite);
    i64 pos = view_get_cursor_pos(app, view);
    i64 mark = view_get_mark_pos(app, view);
    Range_i64 pos_range = Ii64(pos, mark);
    Range_i64 line_range = F4_LineRangeFromPosRange(app, buffer, pos_range);
    History_Group group = history_group_begin(app, buffer);
    F4_ReIndentPosRange(app, buffer, Ii64(pos, mark), -1);
    F4_AdjustCursorAndMarkForIndentation(app, view, pos, mark, line_range);
    history_group_end(group);
    no_mark_snap_to_cursor(app, view);
}

function void
F4_GenerateHotDirectoryFileList_Project(Application_Links *app, Lister *lister)
{
#if 0
    Scratch_Block scratch(app, lister->arena);
    
    Project_File_Pattern_Array whitelist = current_project.pattern_array;
    Project_File_Pattern_Array blacklist = current_project.blacklist_pattern_array;
    
    Temp_Memory temp = begin_temp(lister->arena);
    String_Const_u8 hot = push_hot_directory(app, lister->arena);
    if (!character_is_slash(string_get_character(hot, hot.size - 1))){
        hot = push_u8_stringf(lister->arena, "%.*s/", string_expand(hot));
    }
    lister_set_text_field(lister, hot);
    lister_set_key(lister, string_front_of_path(hot));
    
    File_List file_list = system_get_file_list(scratch, hot);
    end_temp(temp);
    
    File_Info **one_past_last = file_list.infos + file_list.count;
    
    lister_begin_new_item_set(app, lister);
    
    hot = push_hot_directory(app, lister->arena);
    push_align(lister->arena, 8);
    if(hot.str != 0)
    {
        String_Const_u8 empty_string = string_u8_litexpr("");
        Lister_Prealloced_String empty_string_prealloced = lister_prealloced(empty_string);
        
        // NOTE(rjf): Add all directories.
        for (File_Info **info = file_list.infos; info < one_past_last; info += 1)
        {
            if (!HasFlag((**info).attributes.flags, FileAttribute_IsDirectory)) continue;
            String_Const_u8 file_name = push_u8_stringf(lister->arena, "%.*s/",
                                                        string_expand((**info).file_name));
            lister_add_item(lister, lister_prealloced(file_name), empty_string_prealloced, file_name.str, 0);
        }
        
        // NOTE(rjf): Add files, if they match with the project paths.
        for (File_Info **info = file_list.infos;
             info < one_past_last;
             info += 1){
            if (HasFlag((**info).attributes.flags, FileAttribute_IsDirectory)) continue;
            String_Const_u8 file_name = push_string_copy(lister->arena, (**info).file_name);
            
            if(match_in_pattern_array(file_name, whitelist) &&
               !match_in_pattern_array(file_name, blacklist))
            {
                char *is_loaded = "";
                char *status_flag = "";
                
                Buffer_ID buffer = {};
                
                {
                    Temp_Memory path_temp = begin_temp(lister->arena);
                    List_String_Const_u8 list = {};
                    string_list_push(lister->arena, &list, hot);
                    string_list_push_overlap(lister->arena, &list, '/', (**info).file_name);
                    String_Const_u8 full_file_path = string_list_flatten(lister->arena, list);
                    buffer = get_buffer_by_file_name(app, full_file_path, Access_Always);
                    end_temp(path_temp);
                }
                
                if (buffer != 0){
                    is_loaded = "LOADED";
                    Dirty_State dirty = buffer_get_dirty_state(app, buffer);
                    switch (dirty){
                        case DirtyState_UnsavedChanges:  status_flag = " *"; break;
                        case DirtyState_UnloadedChanges: status_flag = " !"; break;
                        case DirtyState_UnsavedChangesAndUnloadedChanges: status_flag = " *!"; break;
                    }
                }
                String_Const_u8 status = push_u8_stringf(lister->arena, "%s%s", is_loaded, status_flag);
                lister_add_item(lister, lister_prealloced(file_name), lister_prealloced(status), file_name.str, 0);
            }
        }
    }
#endif
}

function File_Name_Result
F4_GetFileNameFromUser_Project(Application_Links *app, Arena *arena, String_Const_u8 query, View_ID view)
{
    Lister_Handlers handlers = lister_get_default_handlers();
    handlers.refresh = F4_GenerateHotDirectoryFileList_Project;
    handlers.write_character = lister__write_character__file_path;
    handlers.backspace = lister__backspace_text_field__file_path;
    
    Lister_Result l_result = run_lister_with_refresh_handler(app, arena, query, handlers);
    
    File_Name_Result result = {};
    result.canceled = l_result.canceled;
    if (!l_result.canceled){
        result.clicked = l_result.activated_by_click;
        if (l_result.user_data != 0){
            String_Const_u8 name = SCu8((u8*)l_result.user_data);
            result.file_name_activated = name;
            result.is_folder = character_is_slash(string_get_character(name, name.size - 1));
        }
        result.file_name_in_text_field = string_front_of_path(l_result.text_field);
        
        String_Const_u8 path = {};
        if (l_result.user_data == 0 && result.file_name_in_text_field.size == 0 && l_result.text_field.size > 0){
            result.file_name_in_text_field = string_front_folder_of_path(l_result.text_field);
            path = string_remove_front_folder_of_path(l_result.text_field);
        }
        else{
            path = string_remove_front_of_path(l_result.text_field);
        }
        if (character_is_slash(string_get_character(path, path.size - 1))){
            path = string_chop(path, 1);
        }
        result.path_in_text_field = path;
    }
    
    return(result);
}

CUSTOM_UI_COMMAND_SIG(f4_interactive_open_or_new_in_project)
CUSTOM_DOC("Interactively open a file out of the file system, filtered to files only in the project.")
{
    for(;;)
    {
        Scratch_Block scratch(app);
        View_ID view = get_this_ctx_view(app, Access_Always);
        File_Name_Result result = F4_GetFileNameFromUser_Project(app, scratch, S8Lit("Open (File In Project):"), view);
        if (result.canceled) break;
        
        String_Const_u8 file_name = result.file_name_activated;
        if (file_name.size == 0){
            file_name = result.file_name_in_text_field;
        }
        if (file_name.size == 0) break;
        
        String_Const_u8 path = result.path_in_text_field;
        String_Const_u8 full_file_name = push_u8_stringf(scratch, "%.*s/%.*s",
                                                         string_expand(path), string_expand(file_name));
        
        if(result.is_folder)
        {
            set_hot_directory(app, full_file_name);
            continue;
        }
        
        if(character_is_slash(file_name.str[file_name.size - 1]))
        {
            File_Attributes attribs = system_quick_file_attributes(scratch, full_file_name);
            if(HasFlag(attribs.flags, FileAttribute_IsDirectory))
            {
                set_hot_directory(app, full_file_name);
                continue;
            }
            if(string_looks_like_drive_letter(file_name))
            {
                set_hot_directory(app, file_name);
                continue;
            }
            if(query_create_folder(app, file_name))
            {
                set_hot_directory(app, full_file_name);
                continue;
            }
            break;
        }
        
        Buffer_ID buffer = create_buffer(app, full_file_name, 0);
        if (buffer != 0){
            view_set_buffer(app, view, buffer, 0);
        }
        break;
    }
}

internal void
F4_SetLineCommentedOnLine(Application_Links *app, Buffer_ID buffer, i64 *cursor_p, i64 *mark_p, b32 commented)
{
    i64 cursor = *cursor_p;
    i64 mark = *mark_p;
    i64 cursor_line = get_line_number_from_pos(app, buffer, cursor);
    i64 mark_line = get_line_number_from_pos(app, buffer, mark);
    
    if(cursor_line == mark_line)
    {
        i64 line = cursor_line;
        i64 line_start = get_pos_past_lead_whitespace_from_line_number(app, buffer, line);
        b32 already_has_comment = c_line_comment_starts_at_position(app, buffer, line_start);
        
        if(commented)
        {
            if(!already_has_comment)
            {
                buffer_replace_range(app, buffer, Ii64(line_start), string_u8_litexpr("//"));
                cursor = mark += 2;
            }
        }
        else
        {
            if(already_has_comment)
            {
                buffer_replace_range(app, buffer, Ii64(line_start, line_start + 2), string_u8_empty);
                cursor = mark -= 2;
            }
        }
    }
    
    *cursor_p = cursor;
    *mark_p = mark;
}

internal void
F4_SetBlockCommentedOnRange(Application_Links *app, Buffer_ID buffer, i64 *cursor_p, i64 *mark_p, b32 commented)
{
    Scratch_Block scratch(app);
    
    i64 cursor = *cursor_p;
    i64 mark = *mark_p;
    Range_i64 range = Ii64(cursor, mark);
    
    if(commented)
    {
        buffer_replace_range(app, buffer, Ii64(range.max, range.max), S8Lit("*/"));
        buffer_replace_range(app, buffer, Ii64(range.min, range.min), S8Lit("/*"));
        if(cursor > mark) { cursor += 4; }
        else              { mark   += 4; }
    }
    else if(range.max - range.min >= 2)
    {
        String_Const_u8 opener = push_buffer_range(app, scratch, buffer, Ii64(range.min, range.min+2));
        String_Const_u8 closer = push_buffer_range(app, scratch, buffer, Ii64(range.max-2, range.max));
        if(string_match(opener, S8Lit("/*")) &&
           string_match(closer, S8Lit("*/")))
        {
            buffer_replace_range(app, buffer, Ii64(range.max-2, range.max), S8Lit(""));
            buffer_replace_range(app, buffer, Ii64(range.min, range.min+2), S8Lit(""));
            if(cursor > mark) { cursor -= 4; }
            if(mark > cursor) { mark -= 4; }
        }
    }
    
    *cursor_p = cursor;
    *mark_p = mark;
}

function b32
F4_CBlockCommentStartsAtPosition(Application_Links *app, Buffer_ID buffer, i64 pos)
{
    b32 alread_has_comment = false;
    u8 check_buffer[2];
    if(buffer_read_range(app, buffer, Ii64(pos, pos + 2), check_buffer))
    {
        if(check_buffer[0] == '/' && check_buffer[1] == '*')
        {
            alread_has_comment = true;
        }
    }
    return(alread_has_comment);
}

internal void
F4_SetCommentedOnRange(Application_Links *app, Buffer_ID buffer, i64 *cursor_p, i64 *mark_p, b32 commented)
{
    Scratch_Block scratch(app);
    
    i64 cursor = *cursor_p;
    i64 mark = *mark_p;
    Range_i64 range = Ii64(cursor, mark);
    Range_i64 line_range = F4_LineRangeFromPosRange(app, buffer, range);
    Token_Array tokens = get_token_array_from_buffer(app, buffer);
    
    // NOTE(rjf): No selection
    if(range.min == range.max)
    {
        F4_SetLineCommentedOnLine(app, buffer, &cursor, &mark, commented);
    }
    
    // NOTE(rjf): Single-Line Selection
    else if(line_range.min == line_range.max)
    {
        Token *min_token = get_token_from_pos(app, &tokens, (u64)range.min);
        Token *max_token = get_token_from_pos(app, &tokens, (u64)range.max);
        
        // NOTE(rjf): Selection is inside comment
        if(min_token == max_token && min_token && min_token->kind == TokenBaseKind_Comment)
        {
            (commented ? comment_line : uncomment_line)(app);
        }
        
        // NOTE(rjf): Selection is not inside comment
        else 
        {
            F4_SetBlockCommentedOnRange(app, buffer, &cursor, &mark, commented);
        }
    }
    
    // NOTE(rjf): Multi-Line Selection
    else if(line_range.min != line_range.max)
    {
        if(commented)
        {
            i64 min_pos = Min(cursor, mark);
            i64 line = get_line_number_from_pos(app, buffer, min_pos);
            i64 start_of_line = get_line_start_pos(app, buffer, line);
            
            // NOTE(rjf): Selection starts on first column.
            if(min_pos == start_of_line)
            {
                for(i64 i = line_range.min; i <= line_range.max; i += 1)
                {
                    i64 cursor2 = get_line_start_pos(app, buffer, i);
                    i64 mark2 = get_line_end_pos(app, buffer, i);
                    F4_SetLineCommentedOnLine(app, buffer, &cursor2, &mark2, commented);
                }
                if(cursor < mark)
                {
                    cursor = get_line_start_pos(app, buffer, line_range.min);
                    mark = get_line_end_pos(app, buffer, line_range.max);
                }
                else
                {
                    mark = get_line_start_pos(app, buffer, line_range.min);
                    cursor = get_line_end_pos(app, buffer, line_range.max);
                }
            }
            
            // NOTE(rjf): Selection does not start on first column.
            else
            {
                F4_SetBlockCommentedOnRange(app, buffer, &cursor, &mark, 1);
            }
        }
        else
        {
            b32 starts_with_block_comment = F4_CBlockCommentStartsAtPosition(app, buffer, range.min);
            if(starts_with_block_comment)
            {
                F4_SetBlockCommentedOnRange(app, buffer, &cursor, &mark, 0);
            }
            else
            {
                for(i64 i = line_range.min; i <= line_range.max; i += 1)
                {
                    i64 cursor2 = get_line_start_pos(app, buffer, i);
                    i64 mark2 = get_line_start_pos(app, buffer, i);
                    F4_SetLineCommentedOnLine(app, buffer, &cursor2, &mark2, 0);
                }
                if(cursor < mark)
                {
                    cursor = get_line_start_pos(app, buffer, line_range.min);
                    mark = get_line_end_pos(app, buffer, line_range.max);
                }
                else
                {
                    mark = get_line_start_pos(app, buffer, line_range.min);
                    cursor = get_line_end_pos(app, buffer, line_range.max);
                }
            }
        }
    }
    
    *cursor_p = cursor;
    *mark_p = mark;
}

CUSTOM_COMMAND_SIG(f4_comment_selection)
CUSTOM_DOC("Performs VS-style commenting on the selected range.")
{
    View_ID view = get_active_view(app, Access_ReadWriteVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible);
    i64 cursor = view_get_cursor_pos(app, view);
    i64 mark = view_get_mark_pos(app, view);
    
    History_Group group = history_group_begin(app, buffer);
    F4_SetCommentedOnRange(app, buffer, &cursor, &mark, 1);
    view_set_cursor(app, view, seek_pos(cursor));
    view_set_mark(app, view, seek_pos(mark));
    history_group_end(group);
    no_mark_snap_to_cursor(app, view);
}

CUSTOM_COMMAND_SIG(f4_uncomment_selection)
CUSTOM_DOC("Performs VS-style uncommenting on the selected range.")
{
    View_ID view = get_active_view(app, Access_ReadWriteVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible);
    i64 cursor = view_get_cursor_pos(app, view);
    i64 mark = view_get_mark_pos(app, view);
    History_Group group = history_group_begin(app, buffer);
    F4_SetCommentedOnRange(app, buffer, &cursor, &mark, 0);
    view_set_cursor(app, view, seek_pos(cursor));
    view_set_mark(app, view, seek_pos(mark));
    history_group_end(group);
    no_mark_snap_to_cursor(app, view);
}

struct F4_LOCInfo
{
    F4_LOCInfo *next;
    String_Const_u8 name;
    i64 lines;
    i64 whitespace_only_lines;
    i64 open_brace_only_lines;
};

function F4_LOCInfo *
F4_LOCInfoFromBuffer(Application_Links *app, Arena *arena, Buffer_ID buffer)
{
    F4_LOCInfo *first = 0;
    F4_LOCInfo *last = 0;
    
    F4_LOCInfo *file_info = push_array_zero(arena, F4_LOCInfo, 1);
    sll_queue_push(first, last, file_info);
    file_info->name = str8_lit("all");
    F4_LOCInfo *active_info = 0;
    
    i64 line_count = buffer_get_line_count(app, buffer);
    for(i64 line_idx = 0; line_idx < line_count; line_idx += 1)
    {
        Scratch_Block scratch(app, arena);
        String_Const_u8 line = push_buffer_line(app, scratch, buffer, line_idx);
        if(line.size != 0 && line.str[line.size-1] == '\r')
        {
            line.size -= 1;
        }
        
        //- rjf: begin a section if we find a root divider comment here
        if(line.size >= 3 && line.str[0] == '/' && line.str[1] == '/' && line.str[2] == '~')
        {
            active_info = push_array_zero(arena, F4_LOCInfo, 1);
            active_info->name = push_string_copy(arena, string_substring(line, Ii64(3, line.size)));
            sll_queue_push(first, last, active_info);
        }
        
        //- rjf: find out if this is a line with only whitespace
        b32 is_only_whitespace = true;
        {
            for(u64 i = 0; i < line.size; i += 1)
            {
                if(!character_is_whitespace(line.str[i]))
                {
                    is_only_whitespace = false;
                    break;
                }
            }
        }
        
        //- rjf: find out if this is a line with only whitespace and an open brace
        b32 is_only_open_brace = false;
        if(is_only_whitespace == false)
        {
            for(u64 i = 0; i < line.size; i += 1)
            {
                if(!character_is_whitespace(line.str[i]))
                {
                    is_only_open_brace = line.str[i] == '{';
                    if(!is_only_open_brace)
                    {
                        break;
                    }
                }
            }
        }
        
        //- rjf: increment line counts
        {
            file_info->lines += 1;
            if(active_info != 0)
            {
                active_info->lines += 1;
            }
            if(is_only_whitespace)
            {
                file_info->whitespace_only_lines += 1;
                if(active_info != 0)
                {
                    active_info->whitespace_only_lines += 1;
                }
            }
            if(is_only_open_brace)
            {
                file_info->open_brace_only_lines += 1;
                if(active_info != 0)
                {
                    active_info->open_brace_only_lines += 1;
                }
            }
        }
    }
    
    return first;
}

function int
F4_LOCInfoCompare(const void *a_void_fuck_cplusplus, const void *b_void_fuck_cplusplus)
{
    F4_LOCInfo *a = (F4_LOCInfo *)a_void_fuck_cplusplus;
    F4_LOCInfo *b = (F4_LOCInfo *)b_void_fuck_cplusplus;
    return ((a->lines < b->lines) ? +1 :
            (a->lines > b->lines) ? -1 :
            0);
}

CUSTOM_COMMAND_SIG(f4_loc)
CUSTOM_DOC("Counts the lines of code in the current buffer, breaks it down by section, and outputs to the *loc* buffer.")
{
    Scratch_Block scratch(app);
    View_ID view = get_active_view(app, Access_Read);
    Buffer_ID buffer = view_get_buffer(app, view, Access_Read);
    
    //- rjf: get all sections and counts from buffer
    F4_LOCInfo *infos_list = F4_LOCInfoFromBuffer(app, scratch, buffer);
    
    //- rjf: build unsorted info in array form
    int info_count = 0;
    F4_LOCInfo *info_array = 0;
    {
        for(F4_LOCInfo *info = infos_list; info; info = info->next, info_count += 1);
        info_array = push_array_zero(scratch, F4_LOCInfo, info_count);
        int i = 0;
        for(F4_LOCInfo *info = infos_list; info; info = info->next, i += 1)
        {
            info_array[i] = *info;
        }
    }
    
    //- rjf: sort array
    {
        qsort(info_array, info_count, sizeof(F4_LOCInfo), F4_LOCInfoCompare);
    }
    
    //- rjf: print loc info
    Buffer_ID loc_buffer = get_buffer_by_name(app, str8_lit("*loc*"), AccessFlag_Write);
    if(loc_buffer != 0)
    {
        clear_buffer(app, loc_buffer);
        
        for(int i = 0; i < info_count; i += 1)
        {
            F4_LOCInfo *info = info_array + i;
            
            Scratch_Block scratch2(app, scratch);
            int padding = 25;
            int chrs = (int)info->name.size;
            int spaces = 0;
            if(chrs > padding)
            {
                chrs = padding;
                spaces = 0;
            }
            else
            {
                spaces = padding - chrs;
            }
            
            if(spaces < 0)
            {
                spaces = 0;
            }
            
            String_Const_u8 string = push_stringf(scratch2,
                                                  ">>> %.*s%.*s: %6i lines; %6i whitespace; %6i open braces; %6i significant\n",
                                                  chrs, info->name.str,
                                                  spaces, "                                            ",
                                                  (int)info->lines,
                                                  (int)info->whitespace_only_lines,
                                                  (int)info->open_brace_only_lines,
                                                  (int)(info->lines - (info->whitespace_only_lines+info->open_brace_only_lines)));
            b32 write_successful = buffer_replace_range(app, loc_buffer, Ii64(buffer_get_size(app, loc_buffer)), string);
            write_successful = write_successful;
        }
    }
}

//~ NOTE(rjf): Deprecated names:
CUSTOM_COMMAND_SIG(fleury_write_text_input)
CUSTOM_DOC("Deprecated name. Please update to f4_write_text_input.")
{ f4_write_text_input(app); }
CUSTOM_COMMAND_SIG(fleury_write_text_and_auto_indent)
CUSTOM_DOC("Deprecated name. Please update to f4_write_text_and_auto_indent.")
{f4_write_text_and_auto_indent(app);}
CUSTOM_COMMAND_SIG(fleury_write_zero_struct)
CUSTOM_DOC("Deprecated name. Please update to f4_write_zero_struct.")
{f4_write_zero_struct(app);}
CUSTOM_COMMAND_SIG(fleury_home)
CUSTOM_DOC("Deprecated name. Please update to f4_home.")
{f4_home(app);}
CUSTOM_COMMAND_SIG(fleury_toggle_battery_saver)
CUSTOM_DOC("Deprecated name. Please update to f4_toggle_battery_saver.")
{f4_toggle_battery_saver(app);}
CUSTOM_COMMAND_SIG(fleury_toggle_compilation_expand)
CUSTOM_DOC("Deprecated name. Please update to f4_toggle_compilation_expand.")
{f4_toggle_compilation_expand(app);}
CUSTOM_COMMAND_SIG(fleury_go_to_definition)
CUSTOM_DOC("Deprecated name. Please update to f4_go_to_definition.")
{f4_go_to_definition(app);}
CUSTOM_COMMAND_SIG(fleury_go_to_definition_same_panel)
CUSTOM_DOC("Deprecated name. Please update to f4_go_to_definition_same_panel.")
{f4_go_to_definition_same_panel(app);}
