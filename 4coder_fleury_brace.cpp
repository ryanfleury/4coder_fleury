
//~ NOTE(rjf): Brace highlight

function void
F4_Brace_RenderHighlight(Application_Links *app, Buffer_ID buffer, Text_Layout_ID text_layout_id,
                         i64 pos, ARGB_Color *colors, i32 color_count)
{
    if(!def_get_config_b32(vars_save_string_lit("f4_disable_brace_highlight")))
    {
        ProfileScope(app, "[F4] Brace Highlight");
        Token_Array token_array = get_token_array_from_buffer(app, buffer);
        if (token_array.tokens != 0)
        {
            Token_Iterator_Array it = token_iterator_pos(0, &token_array, pos);
            Token *token = token_it_read(&it);
            if(token != 0 && token->kind == TokenBaseKind_ScopeOpen)
            {
                pos = token->pos + token->size;
            }
            else
            {
                if(token_it_dec_all(&it))
                {
                    token = token_it_read(&it);
                    if (token->kind == TokenBaseKind_ScopeClose &&
                        pos == token->pos + token->size)
                    {
                        pos = token->pos;
                    }
                }
            }
        }
        draw_enclosures(app, text_layout_id, buffer,
                        pos, FindNest_Scope,
                        RangeHighlightKind_CharacterHighlight,
                        0, 0, colors, color_count);
    }
}

//~ NOTE(rjf): Closing-brace Annotation

function void
F4_Brace_RenderCloseBraceAnnotation(Application_Links *app, Buffer_ID buffer, Text_Layout_ID text_layout_id,
                                    i64 pos)
{
    if(!def_get_config_b32(vars_save_string_lit("f4_disable_close_brace_annotation")))
    {
        ProfileScope(app, "[F4] Brace Annotation");
        
        Range_i64 visible_range = text_layout_get_visible_range(app, text_layout_id);
        Token_Array token_array = get_token_array_from_buffer(app, buffer);
        Face_ID face_id = global_small_code_face;
        
        if(token_array.tokens != 0)
        {
            Token_Iterator_Array it = token_iterator_pos(0, &token_array, pos);
            Token *token = token_it_read(&it);
            
            if(token != 0 && token->kind == TokenBaseKind_ScopeOpen)
            {
                pos = token->pos + token->size;
            }
            else if(token_it_dec_all(&it))
            {
                token = token_it_read(&it);
                if (token->kind == TokenBaseKind_ScopeClose &&
                    pos == token->pos + token->size)
                {
                    pos = token->pos;
                }
            }
        }
        
        Scratch_Block scratch(app);
        Range_i64_Array ranges = get_enclosure_ranges(app, scratch, buffer, pos, RangeHighlightKind_CharacterHighlight);
        
        for (i32 i = ranges.count - 1; i >= 0; i -= 1)
        {
            Range_i64 range = ranges.ranges[i];
            
            // NOTE(rjf): Turn this on to only display end scope annotations where the top is off-screen.
#if 0
            if(range.start >= visible_range.start)
            {
                continue;
            }
#else
            // NOTE(jack): Prevent brace annotations from printing on single line scopes.
            if (get_line_number_from_pos(app, buffer, range.start) == get_line_number_from_pos(app, buffer, range.end))
            {
                continue;
            }
#endif
            
            i64 line = get_line_number_from_pos(app, buffer, range.end);
            i64 last_char = get_line_end_pos(app, buffer, line)-1;
            
            Rect_f32 close_scope_rect = text_layout_character_on_screen(app, text_layout_id, last_char);
            Vec2_f32 close_scope_pos = { close_scope_rect.x0 + 12, close_scope_rect.y0 };
            
            // NOTE(rjf): Find token set before this scope begins.
            Token *start_token = 0;
            i64 token_count = 0;
            {
                Token_Iterator_Array it = token_iterator_pos(0, &token_array, range.start-1);
                int paren_nest = 0;
                
                for(;;)
                {
                    Token *token = token_it_read(&it);
                    
                    if(token)
                    {
                        token_count += 1;
                        
                        if(token->kind == TokenBaseKind_ParentheticalClose)
                        {
                            ++paren_nest;
                        }
                        else if(token->kind == TokenBaseKind_ParentheticalOpen)
                        {
                            --paren_nest;
                        }
                        else if(paren_nest == 0 &&
                                (token->kind == TokenBaseKind_ScopeClose ||
                                 (token->kind == TokenBaseKind_StatementClose && token->sub_kind != TokenCppKind_Colon)))
                        {
                            break;
                        }
                        else if((token->kind == TokenBaseKind_Identifier || token->kind == TokenBaseKind_Keyword ||
                                 token->kind == TokenBaseKind_Comment) &&
                                !paren_nest)
                        {
                            start_token = token;
                            break;
                        }
                        
                    }
                    else
                    {
                        break;
                    }
                    
                    if(!token_it_dec_non_whitespace(&it))
                    {
                        break;
                    }
                }
            }
            
            // NOTE(rjf): Draw.
            if(start_token)
            {
                ARGB_Color color = finalize_color(defcolor_comment, 0);
                Color_Array colors = finalize_color_array(fleury_color_brace_annotation);
                if (colors.count >= 1 && F4_ARGBIsValid(colors.vals[0])) {
                    color = colors.vals[(ranges.count - i - 1) % colors.count];
                }
                
                String_Const_u8 start_line = push_buffer_line(app, scratch, buffer,
                                                              get_line_number_from_pos(app, buffer, start_token->pos));
                
                u64 first_non_whitespace_offset = 0;
                for(u64 c = 0; c < start_line.size; ++c)
                {
                    if(start_line.str[c] <= 32)
                    {
                        ++first_non_whitespace_offset;
                    }
                    else
                    {
                        break;
                    }
                }
                start_line.str += first_non_whitespace_offset;
                start_line.size -= first_non_whitespace_offset;
                // NOTE(rjf): Special case to handle CRLF-newline files.
                if(start_line.str[start_line.size - 1] == 13)
                {
                    start_line.size -= 1;
                }
                
                draw_string(app, face_id, start_line, close_scope_pos, color);
            }
        }
    }
    
}

//~ NOTE(rjf): Brace lines

static void
F4_Brace_RenderLines(Application_Links *app, Buffer_ID buffer, View_ID view,
                     Text_Layout_ID text_layout_id, i64 pos)
{
    if(!def_get_config_b32(vars_save_string_lit("f4_disable_brace_lines")))
    {
        ProfileScope(app, "[F4] Brace Lines");
        
        Face_ID face_id = get_face_id(app, buffer);
        Token_Array token_array = get_token_array_from_buffer(app, buffer);
        Range_i64 visible_range = text_layout_get_visible_range(app, text_layout_id);
        
        if (token_array.tokens != 0)
        {
            Token_Iterator_Array it = token_iterator_pos(0, &token_array, pos);
            Token *token = token_it_read(&it);
            if(token != 0 && token->kind == TokenBaseKind_ScopeOpen)
            {
                pos = token->pos + token->size;
            }
            else
            {
                
                if(token_it_dec_all(&it))
                {
                    token = token_it_read(&it);
                    
                    if (token->kind == TokenBaseKind_ScopeClose &&
                        pos == token->pos + token->size)
                    {
                        pos = token->pos;
                    }
                }
            }
        }
        
        Face_Metrics metrics = get_face_metrics(app, face_id);
        
        Scratch_Block scratch(app);
        Range_i64_Array ranges = get_enclosure_ranges(app, scratch, buffer, pos, RangeHighlightKind_CharacterHighlight);
        
        Rect_f32 line_number_rect = {};
        b32 show_line_number_margins = def_get_config_b32(vars_save_string_lit("show_line_number_margins"));
        if(show_line_number_margins)
        {
            Rect_f32 rect = view_get_screen_rect(app, view);
            
            Face_Metrics face_metrics = get_face_metrics(app, face_id);
            f32 digit_advance = face_metrics.decimal_digit_advance;
            
            Rect_f32_Pair pair = layout_line_number_margin(app, buffer, rect, digit_advance);
            line_number_rect = pair.min;
            line_number_rect.x1 += 4;
        }
        
        float x_offset = view_get_screen_rect(app, view).x0 + 4 -
            view_get_buffer_scroll(app, view).position.pixel_shift.x +
            (line_number_rect.x1 - line_number_rect.x0);
        float x_position = 0.f;
        
        u64 vw_indent = def_get_config_u64(app, vars_save_string_lit("virtual_whitespace_regular_indent"));
        
        for (i32 i = ranges.count - 1; i >= 0; i -= 1)
        {
            Range_i64 range = ranges.ranges[i];
            
            Rect_f32 range_start_rect = text_layout_character_on_screen(app, text_layout_id, range.start);
            Rect_f32 range_end_rect = text_layout_character_on_screen(app, text_layout_id, range.end);
            
            if(def_enable_virtual_whitespace)
            {
                x_position = (ranges.count - i - 1) * metrics.space_advance * vw_indent;
            }
            else
            {
                String_Const_u8 line = push_buffer_line(app, scratch, buffer, get_line_number_from_pos(app, buffer, range.end));
                for(u64 char_idx = 0; char_idx < line.size; char_idx += 1)
                {
                    if(!character_is_whitespace(line.str[char_idx]))
                    {
                        x_position = metrics.space_advance * char_idx;
                        break;
                    }
                }
            }
            
            float y_start = 0;
            float y_end = 10000;
            if(range.start >= visible_range.start)
            {
                y_start = range_start_rect.y0 + metrics.line_height;
            }
            if(range.end <= visible_range.end)
            {
                y_end = range_end_rect.y0;
            }
            
            Rect_f32 line_rect = {0};
            line_rect.x0 = x_position+x_offset;
            line_rect.x1 = x_position+1+x_offset;
            line_rect.y0 = y_start;
            line_rect.y1 = y_end;
            
            Color_Array colors = finalize_color_array(fleury_color_brace_line);
            if (colors.count >= 1 && F4_ARGBIsValid(colors.vals[0])) {
                draw_rectangle(app, line_rect, 0.5f, 
                               colors.vals[(ranges.count - i - 1) % colors.count]);
            }
        }
    }
}
