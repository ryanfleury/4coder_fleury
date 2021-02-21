/* date = January 29th 2021 7:54 pm */

#ifndef FCODER_FLEURY_RENDER_HELPERS_H
#define FCODER_FLEURY_RENDER_HELPERS_H

enum F4_RangeHighlightKind
{
    F4_RangeHighlightKind_Whole,
    F4_RangeHighlightKind_Underline,
    F4_RangeHighlightKind_MinorUnderline,
};

struct F4_Flash
{
    b32 active;
    f32 t;
    
    Buffer_ID buffer;
    Range_i64 range;
    ARGB_Color color;
    f32 decay_rate;
};

function void F4_DrawTooltipRect(Application_Links *app, Rect_f32 rect);
function void F4_RenderRangeHighlight(Application_Links *app, View_ID view_id, Text_Layout_ID text_layout_id, Range_i64 range, F4_RangeHighlightKind kind, ARGB_Color color);
function void F4_PushTooltip(String_Const_u8 string, ARGB_Color color);

function void F4_PushFlash(Application_Links *app, Buffer_ID buffer, Range_i64 range, ARGB_Color color, f32 decay_rate);
function void F4_UpdateFlashes(Application_Links *app, Frame_Info info);
function void F4_RenderFlashes(Application_Links *app, View_ID view, Text_Layout_ID text_layout);

#endif // FCODER_FLEURY_RENDER_HELPERS_H
