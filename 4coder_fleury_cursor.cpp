//~ NOTE(rjf): Cursor rendering

global int global_cursor_count = 1;
global i64 global_cursor_positions[16] = {0};
global i64 global_mark_positions[16] = {0};
global int global_hide_region_boundary = 0;

enum Cursor_Type
{
	cursor_none,
	cursor_insert,
	cursor_open_range,
	cursor_close_range,
};

function void
C4_RenderCursorSymbolThingy(Application_Links *app, Rect_f32 rect,
                            f32 roundness, f32 thickness,
                            ARGB_Color color, Cursor_Type type)
{
    f32 line_height = rect.y1 - rect.y0;
    f32 bracket_width = 0.5f*line_height;
    
    if(type == cursor_open_range)
    {
        Rect_f32 start_top, start_side, start_bottom;
        
		Vec2_f32 start_p = {rect.x0, rect.y0};
        
        start_top.x0 = start_p.x + thickness;
        start_top.x1 = start_p.x + bracket_width;
        start_top.y0 = start_p.y;
        start_top.y1 = start_p.y + thickness;
        
        start_bottom.x0 = start_top.x0;
        start_bottom.x1 = start_top.x1;
        start_bottom.y1 = start_p.y + line_height;
        start_bottom.y0 = start_bottom.y1 - thickness;
        
        start_side.x0 = start_p.x;
        start_side.x1 = start_p.x + thickness;
        start_side.y0 = start_top.y0;
        start_side.y1 = start_bottom.y1;
        
        draw_rectangle(app, start_top, roundness, color);
        draw_rectangle(app, start_side, roundness, color);
        
        // draw_rectangle(app, start_bottom, start_color);
    }
    else if(type == cursor_close_range)
	{
		Rect_f32 end_top, end_side, end_bottom;
        
		Vec2_f32 end_p = {rect.x0, rect.y0};
        
		end_top.x0 = end_p.x;
		end_top.x1 = end_p.x - bracket_width;
		end_top.y0 = end_p.y;
		end_top.y1 = end_p.y + thickness;
		
		end_side.x1 = end_p.x;
		end_side.x0 = end_p.x + thickness;
		end_side.y0 = end_p.y;
		end_side.y1 = end_p.y + line_height;
        
		end_bottom.x0 = end_top.x0;
		end_bottom.x1 = end_top.x1;
		end_bottom.y1 = end_p.y + line_height;
		end_bottom.y0 = end_bottom.y1 - thickness;
		
		draw_rectangle(app, end_bottom, roundness, color);
		draw_rectangle(app, end_side, roundness, color);
	}
	else if(type == cursor_insert)
	{
		Rect_f32 side;
		side.x0 = rect.x0;
		side.x1 = rect.x0 + thickness;
		side.y0 = rect.y0;
		side.y1 = rect.y1;
        
		draw_rectangle(app, side, roundness, color);
	}
}

function void
DoTheCursorInterpolation(Application_Links *app, Frame_Info frame_info,
                         Rect_f32 *rect, Rect_f32 *last_rect, Rect_f32 target)
{
    *last_rect = *rect;
    
    float x_change = target.x0 - rect->x0;
    float y_change = target.y0 - rect->y0;
    
    float cursor_size_x = (target.x1 - target.x0);
    float cursor_size_y = (target.y1 - target.y0) * (1 + fabsf(y_change) / 60.f);
    
    b32 should_animate_cursor = !global_battery_saver && !def_get_config_b32(vars_save_string_lit("f4_disable_cursor_trails"));
    if(should_animate_cursor)
    {
        if(fabs(x_change) > 1.f || fabs(y_change) > 1.f)
        {
            animate_in_n_milliseconds(app, 0);
        }
    }
    else
    {
        *rect = *last_rect = target;
        cursor_size_y = target.y1 - target.y0;
    }
    
    if(should_animate_cursor)
    {
        rect->x0 += (x_change) * frame_info.animation_dt * 30.f;
        rect->y0 += (y_change) * frame_info.animation_dt * 30.f;
        rect->x1 = rect->x0 + cursor_size_x;
        rect->y1 = rect->y0 + cursor_size_y;
    }
    
    if(target.y0 > last_rect->y0)
    {
        if(rect->y0 < last_rect->y0)
        {
            rect->y0 = last_rect->y0;
        }
    }
    else
    {
        if(rect->y1 > last_rect->y1)
        {
            rect->y1 = last_rect->y1;
        }
    }
    
}

function void
F4_Cursor_RenderEmacsStyle(Application_Links *app, View_ID view_id, b32 is_active_view,
                           Buffer_ID buffer, Text_Layout_ID text_layout_id,
                           f32 roundness, f32 outline_thickness, Frame_Info frame_info)
{
    Rect_f32 view_rect = view_get_screen_rect(app, view_id);
    Rect_f32 clip = draw_set_clip(app, view_rect);
    Range_i64 visible_range = text_layout_get_visible_range(app, text_layout_id);
    
    b32 has_highlight_range = draw_highlight_range(app, view_id, buffer, text_layout_id, roundness);
    
    ColorFlags flags = 0;
    flags |= !!global_keyboard_macro_is_recording * ColorFlag_Macro;
    flags |= !!power_mode.enabled * ColorFlag_PowerMode;
    ARGB_Color cursor_color = F4_GetColor(app, ColorCtx_Cursor(flags, GlobalKeybindingMode));
    ARGB_Color mark_color = cursor_color;
    ARGB_Color inactive_cursor_color = F4_ARGBFromID(active_color_table, fleury_color_cursor_inactive, 0);
    
    if(!F4_ARGBIsValid(inactive_cursor_color))
    {
        inactive_cursor_color = cursor_color;
    }
    
    if(is_active_view == 0)
    {
        cursor_color = inactive_cursor_color;
        mark_color = inactive_cursor_color;
    }
	
    // TODO(rjf): REMOVE THIS
    {
        i64 cursor_pos = view_get_cursor_pos(app, view_id);
        i64 mark_pos = view_get_mark_pos(app, view_id);
        global_cursor_positions[0] = cursor_pos;
        global_mark_positions[0] = mark_pos;
    }
    
    if(!has_highlight_range)
    {
        
        for(int i = 0; i < 1/*global_cursor_count*/; ++i)
        {
            i64 cursor_pos = global_cursor_positions[0];
            i64 mark_pos = global_mark_positions[0];
            
			Cursor_Type cursor_type = cursor_none;
			Cursor_Type mark_type = cursor_none;
			if(cursor_pos <= mark_pos)
			{
				cursor_type = cursor_open_range;
				mark_type = cursor_close_range;
			}
			else
			{
				cursor_type = cursor_close_range;
				mark_type = cursor_open_range;
			}
			
			if(global_hide_region_boundary)
			{
				cursor_type = cursor_insert;
				mark_type = cursor_none;
			}
            
            Rect_f32 target_cursor = text_layout_character_on_screen(app, text_layout_id, cursor_pos);
            Rect_f32 target_mark = text_layout_character_on_screen(app, text_layout_id, mark_pos);
			
            // NOTE(rjf): Draw cursor.
            {
                if(is_active_view)
                {
                    
                    if(cursor_pos < visible_range.start || cursor_pos > visible_range.end)
                    {
                        f32 width = target_cursor.x1 - target_cursor.x0;
                        target_cursor.x0 = view_rect.x0;
                        target_cursor.x1 = target_cursor.x0 + width;
                    }
                    
                    DoTheCursorInterpolation(app, frame_info, &global_cursor_rect,
                                             &global_last_cursor_rect, target_cursor);
                    
                    
                    if(mark_pos > visible_range.end)
                    {
                        target_mark.x0 = 0;
                        target_mark.y0 = view_rect.y1;
                        target_mark.y1 = view_rect.y1;
                    }
                    
                    if(mark_pos < visible_range.start || mark_pos > visible_range.end)
                    {
                        f32 width = target_mark.x1 - target_mark.x0;
                        target_mark.x0 = view_rect.x0;
                        target_mark.x1 = target_mark.x0 + width;
                    }
                    
                    DoTheCursorInterpolation(app, frame_info, &global_mark_rect, &global_last_mark_rect,
                                             target_mark);
                }
                
                // NOTE(rjf): Draw main cursor.
                {
                    C4_RenderCursorSymbolThingy(app, global_cursor_rect, roundness, 4.f, cursor_color, cursor_type);
					C4_RenderCursorSymbolThingy(app, target_cursor, roundness, 4.f, cursor_color, cursor_type);
                }
                
                // NOTE(rjf): GLOW IT UP
                for(int glow = 0; glow < 20; ++glow)
                {
                    f32 alpha = 0.1f - (power_mode.enabled ? (glow*0.005f) : (glow*0.015f));
                    if(alpha > 0)
                    {
                        Rect_f32 glow_rect = target_cursor;
                        glow_rect.x0 -= glow;
                        glow_rect.y0 -= glow;
                        glow_rect.x1 += glow;
                        glow_rect.y1 += glow;
                        C4_RenderCursorSymbolThingy(app, glow_rect, roundness + glow*0.7f, 2.f,
                                                    fcolor_resolve(fcolor_change_alpha(fcolor_argb(cursor_color), alpha)), cursor_type);
                    }
                    else
                    {
                        break;
                    }
                }
                
            }
            
            // paint_text_color_pos(app, text_layout_id, cursor_pos,
            // fcolor_id(defcolor_at_cursor));
            C4_RenderCursorSymbolThingy(app, global_mark_rect, roundness, 2.f,
                                        fcolor_resolve(fcolor_change_alpha(fcolor_argb(mark_color), 0.5f)), mark_type);
			C4_RenderCursorSymbolThingy(app, target_mark, roundness, 2.f,
                                        fcolor_resolve(fcolor_change_alpha(fcolor_argb(mark_color), 0.75f)), mark_type);
        }
    }
    
    draw_set_clip(app, clip);
}

internal b32
F4_Cursor_DrawHighlightRange(Application_Links *app, View_ID view_id,
                             Buffer_ID buffer, Text_Layout_ID text_layout_id,
                             f32 roundness)
{
    b32 has_highlight_range = false;
    Managed_Scope scope = view_get_managed_scope(app, view_id);
    Buffer_ID *highlight_buffer = scope_attachment(app, scope, view_highlight_buffer, Buffer_ID);
    if (*highlight_buffer != 0){
        if (*highlight_buffer != buffer){
            view_disable_highlight_range(app, view_id);
        }
        else{
            has_highlight_range = true;
            Managed_Object *highlight = scope_attachment(app, scope, view_highlight_range, Managed_Object);
            Marker marker_range[2];
            if (managed_object_load_data(app, *highlight, 0, 2, marker_range)){
                Range_i64 range = Ii64(marker_range[0].pos, marker_range[1].pos);
                draw_character_block(app, text_layout_id, range, roundness,
                                     fcolor_id(defcolor_highlight));
            }
        }
    }
    return(has_highlight_range);
}

function void
F4_Cursor_RenderNotepadStyle(Application_Links *app, View_ID view_id, b32 is_active_view,
                             Buffer_ID buffer, Text_Layout_ID text_layout_id,
                             f32 roundness, f32 outline_thickness, Frame_Info frame_info)
{
    Rect_f32 view_rect = view_get_screen_rect(app, view_id);
    b32 has_highlight_range = draw_highlight_range(app, view_id, buffer, text_layout_id, roundness);
    if(!has_highlight_range)
    {
        i64 cursor_pos = view_get_cursor_pos(app, view_id);
        i64 mark_pos = view_get_mark_pos(app, view_id);
        
        if (cursor_pos != mark_pos)
        {
            Range_i64 range = Ii64(cursor_pos, mark_pos);
            draw_character_block(app, text_layout_id, range, roundness, fcolor_id(defcolor_highlight));
        }
        
        // NOTE(rjf): Draw cursor
        {
            ARGB_Color cursor_color = F4_GetColor(app, ColorCtx_Cursor(0, GlobalKeybindingMode));
            ARGB_Color ghost_color = fcolor_resolve(fcolor_change_alpha(fcolor_argb(cursor_color), 0.5f));
            Rect_f32 rect = text_layout_character_on_screen(app, text_layout_id, cursor_pos);
            rect.x1 = rect.x0 + outline_thickness;
            if(rect.x0 < view_rect.x0)
            {
                rect.x0 = view_rect.x0;
                rect.x1 = view_rect.x0 + outline_thickness;
            }
            
            if(is_active_view)
            {
                DoTheCursorInterpolation(app, frame_info, &global_cursor_rect, &global_last_cursor_rect, rect);
            }
            draw_rectangle(app, global_cursor_rect, roundness, ghost_color);
            draw_rectangle(app, rect, roundness, cursor_color);
        }
    }
}

static void
F4_HighlightCursorMarkRange(Application_Links *app, View_ID view_id)
{
    Rect_f32 view_rect = view_get_screen_rect(app, view_id);
    Rect_f32 clip = draw_set_clip(app, view_rect);
    
    f32 lower_bound_y;
    f32 upper_bound_y;
    
    if(global_last_cursor_rect.y0 < global_last_mark_rect.y0)
    {
        lower_bound_y = global_last_cursor_rect.y0;
        upper_bound_y = global_last_mark_rect.y1;
    }
    else
    {
        lower_bound_y = global_last_mark_rect.y0;
        upper_bound_y = global_last_cursor_rect.y1;
    }
    
    draw_rectangle(app, Rf32(view_rect.x0, lower_bound_y, view_rect.x0 + 4, upper_bound_y), 3.f,
                   fcolor_resolve(fcolor_change_alpha(fcolor_id(defcolor_comment), 0.5f)));
    draw_set_clip(app, clip);
}


//~ NOTE(rjf): Mark Annotation

#if 0
function void
F4_RenderMarkAnnotation(Application_Links *app, Buffer_ID buffer, Text_Layout_ID text_layout_id,
                        View_ID view_id, b32 is_active_view)
{
    i64 pos = view_get_mark_pos(app, view_id);
    
    if(view_get_cursor_pos(app, view_id) > pos && is_active_view)
    {
        Scratch_Block scratch(app);
        ProfileScope(app, "[Fleury] Mark Annotation");
        
        Token_Array token_array = get_token_array_from_buffer(app, buffer);
        Face_ID face_id = global_small_code_face;
        Rect_f32 view_rect = view_get_screen_rect(app, view_id);
        
        Token *start_token = 0;
        
        if(token_array.tokens != 0)
        {
            Token_Iterator_Array it = token_iterator_pos(0, &token_array, pos);
            
            int max = 5;
            int count = 0;
            
            for(Token *token = 0; (token = token_it_read(&it)) != 0 && count < max;
                token_it_inc_non_whitespace(&it), ++count)
            {
                if(token->kind == TokenBaseKind_Comment ||
                   token->kind == TokenBaseKind_Identifier ||
                   token->kind == TokenBaseKind_Keyword)
                {
                    start_token = token;
                    break;
                }
            }
        }
        
        // NOTE(rjf): Draw.
        if(start_token)
        {
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
            
            Vec2_f32 draw_pos =
            {
                view_rect.x0 + 30,
                global_cursor_rect.y0,
            };
            
            if(draw_pos.y < view_rect.y0)
            {
                draw_pos.y = view_rect.y0;
            }
            
            u32 color = finalize_color(defcolor_comment, 0);
            color &= 0x00ffffff;
            color |= 0x80000000;
            draw_string_oriented(app, face_id, color, start_line, draw_pos, 0, V2f32(0.f, 1.f));
        }
    }
}
#endif