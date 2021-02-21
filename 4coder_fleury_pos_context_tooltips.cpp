internal Vec2_f32
_F4_PosContext_RenderDefinitionTokens(Application_Links *app, Face_ID face,
                                      String_Const_u8 backing_string,
                                      Token_Array tokens, b32 do_render,
                                      int highlight_arg, Vec2_f32 text_position,
                                      f32 max_x)
{
    Scratch_Block scratch(app);
    Vec2_f32 starting_text_pos = text_position;
    Face_Metrics metrics = get_face_metrics(app, face);
    
    Token_Iterator_Array it = token_iterator_pos(0, &tokens, 0);
    b32 found_first_open_paren = 0;
    for(int arg_idx = 0;;)
    {
        Token *token = token_it_read(&it);
        if(token == 0) { break; }
        
        if(token->kind == TokenBaseKind_Whitespace)
        {
            text_position.x += get_string_advance(app, face, string_u8_litexpr(" "));
        }
        else
        {
            ARGB_Color color = finalize_color(defcolor_text_default, 0);
            if(token->kind == TokenBaseKind_StatementClose)
            {
                String_Const_u8 str = string_substring(backing_string, Ii64(token));
                if(string_match(str, S8Lit(",")))
                {
                    arg_idx += 1;
                }
            }
            else if(token->kind == TokenBaseKind_ParentheticalOpen)
            {
                found_first_open_paren = 1;
            }
            
            // NOTE(rjf): Highlight
            b32 highlight = 0;
            if(found_first_open_paren && arg_idx == highlight_arg &&
               (token->kind == TokenBaseKind_Identifier ||
                token->kind == TokenBaseKind_Operator ||
                token->kind == TokenBaseKind_Keyword))
            {
                color = finalize_color(fleury_color_token_highlight, 0);
                highlight = 1;
            }
            
            Vec2_f32 start_pos = text_position;
            String_Const_u8 token_string = string_substring(backing_string,
                                                            Ii64(token->pos, token->pos+token->size));
            f32 string_advance = get_string_advance(app, face, token_string);
            if(text_position.x + string_advance >= max_x)
            {
                text_position.x = starting_text_pos.x;
                text_position.y += metrics.line_height;
            }
            if(do_render)
            {
                draw_string(app, face, token_string, text_position, color);
            }
            text_position.x += string_advance;
            if(highlight)
            {
                if(do_render)
                {
                    draw_rectangle(app, Rf32(start_pos.x, start_pos.y + metrics.line_height,
                                             text_position.x, start_pos.y + metrics.line_height + 2.f),
                                   1.f, color);
                }
            }
        }
        
        if(token_it_inc_all(&it) == 0)
        {
            break;
        }
    }
    return text_position;
}

internal void
F4_PosContext_Render(Application_Links *app, View_ID view, Buffer_ID buffer,
                     Text_Layout_ID text_layout_id, i64 pos)
{
    if(def_get_config_b32(vars_save_string_lit("f4_disable_poscontext")))
    {
        return;
    }
    
    ProfileScope(app, "[F4] Pos Context Rendering");
    Scratch_Block scratch(app);
    
    Rect_f32 cursor_rect = text_layout_character_on_screen(app, text_layout_id, pos);
    Rect_f32 view_rect = view_get_screen_rect(app, view);
    Face_ID face = global_small_code_face;
    Face_Metrics metrics = get_face_metrics(app, face);
    F4_Language *language = F4_LanguageFromBuffer(app, buffer);
    f32 padding = 4.f;
    
    if(language != 0)
    {
        
        b32 render_at_cursor = 1;
        if(def_get_config_b32(vars_save_string_lit("f4_poscontext_draw_at_bottom_of_buffer")))
        {
            render_at_cursor = 0;
        }
        
        Vec2_f32 tooltip_position =
        {
            global_cursor_rect.x0,
            global_cursor_rect.y1,
        };
        
        F4_Language_PosContextData *ctx_list = language->PosContext(app, scratch, buffer, pos);
        if(render_at_cursor == 0)
        {
            f32 height = 0;
            for(F4_Language_PosContextData *ctx = ctx_list; ctx; ctx = ctx->next)
            {
                height += metrics.line_height + 2*padding;
            }
            tooltip_position = V2f32(view_rect.x0, view_rect.y1 - height);
        }
        
        for(F4_Language_PosContextData *ctx = ctx_list; ctx; ctx = ctx->next)
        {
            F4_Index_Note *note = ctx->relevant_note;
            if(note != 0 && note->file != 0)
            {
                
                //~ NOTE(rjf): Function arguments.
                if(note->kind == F4_Index_NoteKind_Function ||
                   note->kind == F4_Index_NoteKind_Macro)
                {
                    
                    // NOTE(rjf): Find range of definition + params
                    Range_i64 definition_range = note->range;
                    {
                        Token_Array defbuffer_tokens = get_token_array_from_buffer(app, note->file->buffer);
                        Token_Iterator_Array it = token_iterator_pos(0, &defbuffer_tokens, note->range.min);
                        int paren_nest = 0;
                        for(;token_it_inc_all(&it);)
                        {
                            Token *token = token_it_read(&it);
                            if(token)
                            {
                                if(token->kind == TokenBaseKind_ParentheticalOpen)
                                {
                                    paren_nest += 1;
                                }
                                if(token->kind == TokenBaseKind_ParentheticalClose)
                                {
                                    paren_nest -= 1;
                                    if(paren_nest == 0)
                                    {
                                        definition_range.max = token->pos + token->size;
                                        break;
                                    }
                                }
                            }
                        }
                    }
                    
                    String_Const_u8 definition_string = push_buffer_range(app, scratch, note->file->buffer, definition_range);
                    Token_Array definition_tokens = token_array_from_text(app, scratch, definition_string);
                    
                    // NOTE(rjf): Calculate needed size for this tooltip.
                    f32 max_x = view_rect.x1-view_rect.x0;
                    Vec2_f32 end_draw_position = _F4_PosContext_RenderDefinitionTokens(app, face, definition_string, definition_tokens,
                                                                                       false, 0, V2f32(0, 0), max_x);
                    Vec2_f32 needed_size =
                    {
                        (end_draw_position.y > 0) ? max_x : end_draw_position.x,
                        end_draw_position.y,
                    };
                    
                    Rect_f32 draw_rect =
                    {
                        tooltip_position.x,
                        tooltip_position.y,
                        tooltip_position.x + needed_size.x + 2*padding,
                        tooltip_position.y + needed_size.y + metrics.line_height + 2*padding,
                    };
                    if(draw_rect.x1 > view_rect.x1)
                    {
                        f32 width = rect_width(draw_rect);
                        draw_rect.x0 = (f32)(int)(view_rect.x1 - width);
                        draw_rect.x1 = view_rect.x1;
                    }
                    if(draw_rect.y1 > view_rect.y1)
                    {
                        f32 height = rect_height(draw_rect);
                        draw_rect.y0 = (f32)(int)(view_rect.y1 - height);
                        draw_rect.y1 = view_rect.y1;
                    }
                    
                    F4_DrawTooltipRect(app, draw_rect);
                    
                    // NOTE(rjf): Render tokens of definition
                    {
                        Vec2_f32 text_position =
                        {
                            draw_rect.x0 + padding,
                            draw_rect.y0 + padding,
                        };
                        
                        _F4_PosContext_RenderDefinitionTokens(app, face, definition_string, definition_tokens,
                                                              true, ctx->argument_index,
                                                              text_position,
                                                              view_rect.x1);
                    }
                    
                    f32 advance = draw_rect.y1 - draw_rect.y0;
                    tooltip_position.y += advance;
                }
                else if(note->kind == F4_Index_NoteKind_Type)
                {
                    Token_Array defbuffer_tokens = get_token_array_from_buffer(app, note->file->buffer);
                    for(F4_Index_Note *member = note->first_child; member; member = member->next_sibling)
                    {
                        
                        Range_i64 member_range = member->range;
                        Token_Iterator_Array it = token_iterator_pos(0, &defbuffer_tokens, member->range.min);
                        for(;;)
                        {
                            Token *token = token_it_read(&it);
                            if(token)
                            {
                                if(token->kind == TokenBaseKind_StatementClose)
                                {
                                    member_range.max = token->pos;
                                    break;
                                }
                            }
                            else { break; }
                            if(!token_it_inc_non_whitespace(&it))
                            {
                                break;
                            }
                        }
                        
                        String_Const_u8 member_string = push_buffer_range(app, scratch, note->file->buffer, member_range);
                        
                        Vec2_f32 needed_size = { get_string_advance(app, face, member_string), 0, };
                        Rect_f32 draw_rect =
                        {
                            tooltip_position.x,
                            tooltip_position.y,
                            tooltip_position.x + needed_size.x + 2*padding,
                            tooltip_position.y + needed_size.y + metrics.line_height + 2*padding,
                        };
                        
                        F4_DrawTooltipRect(app, draw_rect);
                        draw_string(app, face, member_string, V2f32(draw_rect.x0 + padding, draw_rect.y0 + padding), finalize_color(defcolor_text_default, 0));
                        
                        f32 advance = draw_rect.y1 - draw_rect.y0;
                        tooltip_position.y += advance;
                    }
                }
                
            }
            
        }
    }
}
