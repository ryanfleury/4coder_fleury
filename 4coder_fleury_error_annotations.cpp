//~ NOTE(rjf): Error annotations

function void
F4_RenderErrorAnnotations(Application_Links *app, Buffer_ID buffer,
                          Text_Layout_ID text_layout_id,
                          Buffer_ID jump_buffer)
{
    if(def_get_config_b32(vars_save_string_lit("f4_disable_error_annotations")))
    {
        return;
    }
    
    ProfileScope(app, "[Fleury] Error Annotations");
    
    Heap *heap = &global_heap;
    Scratch_Block scratch(app);
    
    Locked_Jump_State jump_state = {};
    {
        ProfileScope(app, "[Fleury] Error Annotations (Get Locked Jump State)");
        jump_state = get_locked_jump_state(app, heap);
    }
    
    Face_ID face = global_small_code_face;
    Face_Metrics metrics = get_face_metrics(app, face);
    
    if(jump_buffer != 0 && jump_state.view != 0)
    {
        Managed_Scope buffer_scopes[2];
        {
            ProfileScope(app, "[Fleury] Error Annotations (Buffer Get Managed Scope)");
            buffer_scopes[0] = buffer_get_managed_scope(app, jump_buffer);
            buffer_scopes[1] = buffer_get_managed_scope(app, buffer);
        }
        
        Managed_Scope comp_scope = 0;
        {
            ProfileScope(app, "[Fleury] Error Annotations (Get Managed Scope)");
            comp_scope = get_managed_scope_with_multiple_dependencies(app, buffer_scopes, ArrayCount(buffer_scopes));
        }
        
        Managed_Object *buffer_markers_object = 0;
        {
            ProfileScope(app, "[Fleury] Error Annotations (Scope Attachment)");
            buffer_markers_object = scope_attachment(app, comp_scope, sticky_jump_marker_handle, Managed_Object);
        }
        
        // NOTE(rjf): Get buffer markers (locations where jumps point at).
        i32 buffer_marker_count = 0;
        Marker *buffer_markers = 0;
        {
            ProfileScope(app, "[Fleury] Error Annotations (Load Managed Object Data)");
            buffer_marker_count = managed_object_get_item_count(app, *buffer_markers_object);
            buffer_markers = push_array(scratch, Marker, buffer_marker_count);
            managed_object_load_data(app, *buffer_markers_object, 0, buffer_marker_count, buffer_markers);
        }
        
        i64 last_line = -1;
        
        for(i32 i = 0; i < buffer_marker_count; i += 1)
        {
            ProfileScope(app, "[Fleury] Error Annotations (Buffer Loop)");
            
            i64 jump_line_number = get_line_from_list(app, jump_state.list, i);
            i64 code_line_number = get_line_number_from_pos(app, buffer, buffer_markers[i].pos);
            
            if(code_line_number != last_line)
            {
                ProfileScope(app, "[Fleury] Error Annotations (Jump Line)");
                
                String_Const_u8 jump_line = push_buffer_line(app, scratch, jump_buffer, jump_line_number);
                
                // NOTE(rjf): Remove file part of jump line.
                {
                    u64 index = string_find_first(jump_line, string_u8_litexpr("error"), StringMatch_CaseInsensitive);
                    if(index == jump_line.size)
                    {
                        index = string_find_first(jump_line, string_u8_litexpr("warning"), StringMatch_CaseInsensitive);
                        if(index == jump_line.size)
                        {
                            index = 0;
                        }
                    }
                    jump_line.str += index;
                    jump_line.size -= index;
                }
                
                // NOTE(rjf): Render annotation.
                {
                    Range_i64 line_range = Ii64(code_line_number);
                    
                    Range_f32 y1 = text_layout_line_on_screen(app, text_layout_id, line_range.min);
                    Range_f32 y2 = text_layout_line_on_screen(app, text_layout_id, line_range.max);
                    Range_f32 y = range_union(y1, y2);
                    Rect_f32 last_character_on_line_rect =
                        text_layout_character_on_screen(app, text_layout_id, get_line_end_pos(app, buffer, code_line_number)-1);
                    
                    if(range_size(y) > 0.f)
                    {
                        Rect_f32 region = text_layout_region(app, text_layout_id);
                        Vec2_f32 draw_position =
                        {
                            region.x1 - metrics.max_advance*jump_line.size -
                                (y.max-y.min)/2 - metrics.line_height/2,
                            y.min + (y.max-y.min)/2 - metrics.line_height/2,
                        };
                        
                        if(draw_position.x < last_character_on_line_rect.x1 + 30)
                        {
                            draw_position.x = last_character_on_line_rect.x1 + 30;
                        }
                        
                        draw_string(app, face, jump_line, draw_position, fcolor_id(fleury_color_error_annotation));
                        
                        Mouse_State mouse_state = get_mouse_state(app);
                        if(mouse_state.x >= region.x0 && mouse_state.x <= region.x1 &&
                           mouse_state.y >= y.min && mouse_state.y <= y.max)
                        {
                            F4_PushTooltip(jump_line, 0xffff0000);
                        }
                    }
                }
            }
            
            last_line = code_line_number;
        }
    }
}
