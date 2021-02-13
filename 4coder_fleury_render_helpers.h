/* date = January 29th 2021 7:54 pm */

#ifndef FCODER_FLEURY_RENDER_HELPERS_H
#define FCODER_FLEURY_RENDER_HELPERS_H

enum F4_RangeHighlightKind
{
    F4_RangeHighlightKind_Whole,
    F4_RangeHighlightKind_Underline,
    F4_RangeHighlightKind_MinorUnderline,
};

function void F4_DrawTooltipRect(Application_Links *app, Rect_f32 rect);
function void F4_RenderRangeHighlight(Application_Links *app, View_ID view_id, Text_Layout_ID text_layout_id, Range_i64 range, F4_RangeHighlightKind kind);
function void F4_PushTooltip(String_Const_u8 string, ARGB_Color color);

#endif // FCODER_FLEURY_RENDER_HELPERS_H
