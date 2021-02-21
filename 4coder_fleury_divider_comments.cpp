//~ NOTE(rjf): Divider Comments

String_Const_u8 strong_divider_comment_signifier = string_u8_litexpr("//~");
String_Const_u8 weak_divider_comment_signifier = string_u8_litexpr("//-");

function i64
_F4_Boundary_DividerComment(Application_Links *app, Buffer_ID buffer, 
                            Side side, Scan_Direction direction, i64 pos,
                            String_Const_u8 signifier)
{
    i64 result = pos;
    Scratch_Block scratch(app);
    
    Token_Array tokens = get_token_array_from_buffer(app, buffer);
    if(tokens.tokens != 0)
    {
        Token_Iterator_Array it = token_iterator_pos(0, &tokens, pos);
        switch(direction)
        {
            case Scan_Forward:
            {
                for(;token_it_inc_non_whitespace(&it);)
                {
                    Token *token = token_it_read(&it);
                    if(token == 0)
                    {
                        break;
                    }
                    if(token->kind == TokenBaseKind_Comment)
                    {
                        String_Const_u8 str = push_buffer_range(app, scratch, buffer, Ii64(token));
                        if(str.size >= signifier.size &&
                           string_match(string_substring(str, Ii64(0, signifier.size)), signifier))
                        {
                            result = token->pos;
                            break;
                        }
                    }
                }
            }break;
            
            case Scan_Backward:
            {
                for(;token_it_dec_non_whitespace(&it);)
                {
                    Token *token = token_it_read(&it);
                    if(token == 0)
                    {
                        break;
                    }
                    if(token->kind == TokenBaseKind_Comment)
                    {
                        String_Const_u8 str = push_buffer_range(app, scratch, buffer, Ii64(token));
                        if(str.size >= signifier.size &&
                           string_match(string_substring(str, Ii64(0, signifier.size)), signifier))
                        {
                            result = token->pos;
                            break;
                        }
                    }
                }
            }break;
            
        }
    }
    return(result);
}

function i64
F4_Boundary_DividerComment(Application_Links *app, Buffer_ID buffer, 
                           Side side, Scan_Direction direction, i64 pos,
                           String_Const_u8 signifier)
{
    return _F4_Boundary_DividerComment(app, buffer, side, direction, pos, strong_divider_comment_signifier);
}

CUSTOM_COMMAND_SIG(f4_move_to_next_divider_comment)
CUSTOM_DOC("Seek right for next divider comment in the buffer.")
{
    Scratch_Block scratch(app);
    current_view_scan_move(app, Scan_Forward, push_boundary_list(scratch, F4_Boundary_DividerComment));
}

CUSTOM_COMMAND_SIG(f4_move_to_prev_divider_comment)
CUSTOM_DOC("Seek left for previous divider comment in the buffer.")
{
    Scratch_Block scratch(app);
    current_view_scan_move(app, Scan_Backward, push_boundary_list(scratch, F4_Boundary_DividerComment));
}

function void
F4_RenderDividerComments(Application_Links *app, Buffer_ID buffer, View_ID view,
                         Text_Layout_ID text_layout_id)
{
    if(!def_get_config_b32(vars_save_string_lit("f4_disable_divider_comments")))
    {
        ProfileScope(app, "[F4] Divider Comments");
        
        Token_Array token_array = get_token_array_from_buffer(app, buffer);
        Range_i64 visible_range = text_layout_get_visible_range(app, text_layout_id);
        Scratch_Block scratch(app);
        
        if(token_array.tokens != 0)
        {
            i64 first_index = token_index_from_pos(&token_array, visible_range.first);
            Token_Iterator_Array it = token_iterator_index(0, &token_array, first_index);
            
            Token *token = 0;
            for(;;)
            {
                token = token_it_read(&it);
                
                if(token->pos >= visible_range.one_past_last || !token || !token_it_inc_non_whitespace(&it))
                {
                    break;
                }
                
                if(token->kind == TokenBaseKind_Comment)
                {
                    Rect_f32 comment_first_char_rect = text_layout_character_on_screen(app, text_layout_id, token->pos);
                    Rect_f32 comment_last_char_rect = text_layout_character_on_screen(app, text_layout_id, token->pos+token->size-1);
                    String_Const_u8 token_string = push_buffer_range(app, scratch, buffer, Ii64(token));
                    String_Const_u8 signifier_substring = string_substring(token_string, Ii64(0, 3));
                    f32 roundness = 4.f;
                    
                    // NOTE(rjf): Strong dividers.
                    if(string_match(signifier_substring, strong_divider_comment_signifier))
                    {
                        Rect_f32 rect =
                        {
                            comment_first_char_rect.x0,
                            comment_first_char_rect.y0-2,
                            10000,
                            comment_first_char_rect.y0,
                        };
                        draw_rectangle(app, rect, roundness, fcolor_resolve(fcolor_id(defcolor_comment)));
                    }
                    
                    // NOTE(rjf): Weak dividers.
                    else if(string_match(signifier_substring, weak_divider_comment_signifier))
                    {
                        f32 dash_size = 8;
                        Rect_f32 rect =
                        {
                            comment_last_char_rect.x1,
                            (comment_last_char_rect.y0 + comment_last_char_rect.y1)/2 - 1,
                            comment_last_char_rect.x1 + dash_size,
                            (comment_last_char_rect.y0 + comment_last_char_rect.y1)/2 + 1,
                        };
                        
                        for(int i = 0; i < 1000; i += 1)
                        {
                            draw_rectangle(app, rect, roundness, fcolor_resolve(fcolor_id(defcolor_comment)));
                            rect.x0 += dash_size*1.5f;
                            rect.x1 += dash_size*1.5f;
                        }
                    }
                    
                }
            }
        }
    }
}