global F4_Flash f4_flashes[64];

function void
F4_DrawTooltipRect(Application_Links *app, Rect_f32 rect)
{
    ARGB_Color background_color = fcolor_resolve(fcolor_id(defcolor_back));
    ARGB_Color border_color = fcolor_resolve(fcolor_id(defcolor_margin_active));
    
    background_color &= 0x00ffffff;
    background_color |= 0xd0000000;
    
    border_color &= 0x00ffffff;
    border_color |= 0xd0000000;
    
    draw_rectangle(app, rect, 4.f, background_color);
    draw_rectangle_outline(app, rect, 4.f, 3.f, border_color);
}

function void
F4_RenderRangeHighlight(Application_Links *app, View_ID view_id, Text_Layout_ID text_layout_id,
                        Range_i64 range, F4_RangeHighlightKind kind, ARGB_Color color)
{
    Rect_f32 range_start_rect = text_layout_character_on_screen(app, text_layout_id, range.start);
    Rect_f32 range_end_rect = text_layout_character_on_screen(app, text_layout_id, range.end-1);
    Rect_f32 total_range_rect = {0};
    total_range_rect.x0 = MinimumF32(range_start_rect.x0, range_end_rect.x0);
    total_range_rect.y0 = MinimumF32(range_start_rect.y0, range_end_rect.y0);
    total_range_rect.x1 = MaximumF32(range_start_rect.x1, range_end_rect.x1);
    total_range_rect.y1 = MaximumF32(range_start_rect.y1, range_end_rect.y1);
    
    switch (kind) {
        case F4_RangeHighlightKind_Underline: {
            total_range_rect.y0 = total_range_rect.y1 - 1.f;
            total_range_rect.y1 += 1.f;
        } break;
        
        case F4_RangeHighlightKind_MinorUnderline: {
            total_range_rect.y0 = total_range_rect.y1 - 1.f;
            total_range_rect.y1 += 1.f;
        }
    }
    draw_rectangle(app, total_range_rect, 4.f, color);
}

function void
F4_PushTooltip(String_Const_u8 string, ARGB_Color color)
{
    if(global_tooltip_count < ArrayCount(global_tooltips))
    {
        String_Const_u8 string_copy = push_string_copy(&global_frame_arena, string);
        global_tooltips[global_tooltip_count].color = color;
        global_tooltips[global_tooltip_count].string = string_copy;
        global_tooltip_count += 1;
    }
}

function void
F4_PushFlash(Application_Links *app, Buffer_ID buffer, Range_i64 range, ARGB_Color color, f32 decay_rate)
{
    F4_Flash *flash = 0;
    for(int i = 0; i < ArrayCount(f4_flashes); i += 1)
    {
        if(f4_flashes[i].active == 0)
        {
            flash = f4_flashes + i;
            break;
        }
    }
    if(flash)
    {
        flash->active = 1;
        flash->t = 1;
        flash->buffer = buffer;
        flash->range = range;
        flash->color = color;
        flash->decay_rate = decay_rate;
    }
}

function void
F4_UpdateFlashes(Application_Links *app, Frame_Info frame)
{
    for(int i = 0; i < ArrayCount(f4_flashes); i += 1)
    {
        F4_Flash *flash = f4_flashes + i;
        if(flash->active)
        {
            animate_in_n_milliseconds(app, 0);
            flash->t += (0 - flash->t) * flash->decay_rate * frame.animation_dt;
            if(flash->t <= 0.05f)
            {
                flash->active = 0;
            }
        }
    }
}

function void
F4_RenderFlashes(Application_Links *app, View_ID view, Text_Layout_ID text_layout)
{
    Buffer_ID buffer = view_get_buffer(app, view, Access_Always);
    for(int i = 0; i < ArrayCount(f4_flashes); i += 1)
    {
        F4_Flash *flash = f4_flashes + i;
        if(flash->active && flash->buffer == buffer)
        {
            F4_RenderRangeHighlight(app, view, text_layout, flash->range, F4_RangeHighlightKind_Whole,
                                    argb_color_blend(flash->color, flash->t, 0, 1-flash->t));
        }
    }
}
