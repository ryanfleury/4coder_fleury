#define F4_MAX_LEGOS 12
global F4_Lego f4_legos[F4_MAX_LEGOS];

function F4_Lego *
F4_LegoFromIndex(int index)
{
    F4_Lego *result = 0;
    if(index >= 0 && index < F4_MAX_LEGOS)
    {
        result = f4_legos + index;
    }
    return result;
}

function void
_F4_Lego_Initialize(F4_Lego *lego)
{
    if(lego == 0)
    {
        return;
    }
    if(lego->initialized == 0)
    {
        memset(lego, 0, sizeof(*lego));
        lego->initialized = 1;
        lego->arena = make_arena(get_base_allocator_system());
    }
}

function void
F4_Lego_Store(F4_Lego *lego, F4_LegoKind kind, String8 string)
{
    _F4_Lego_Initialize(lego);
    linalloc_clear(&lego->arena);
    lego->kind = kind;
    lego->string = push_string_copy(&lego->arena, string);
}

function F4_Lego *
F4_LegoFromUserInput(User_Input in)
{
    F4_Lego *lego = 0;
    Input_Event event = in.event;
    if(event.kind == InputEventKind_KeyStroke &&
       event.key.code >= KeyCode_F1 && event.key.code <= KeyCode_F24)
    {
        int index = event.key.code - KeyCode_F1;
        index = index % 4;
        lego = F4_LegoFromIndex(index);
    }
    return lego;
}

function void
F4_Lego_BufferPlace(Application_Links *app, View_ID view, Buffer_ID buffer, i64 pos, F4_Lego *lego)
{
    switch(lego->kind)
    {
        case F4_LegoKind_String:
        {
            buffer_replace_range(app, buffer, Ii64(pos, pos), lego->string);
            view_set_mark(app, view, seek_pos(pos));
            view_set_cursor_and_preferred_x(app, view, seek_pos(pos + (i32)lego->string.size));
            
            F4_PushFlash(app, buffer, Ii64(pos, pos+lego->string.size), fcolor_resolve(fcolor_id(fleury_color_lego_splat)), 0.8f);
        }break;
        default: break;
    }
}

CUSTOM_COMMAND_SIG(f4_lego_buffer_place)
CUSTOM_DOC("Will place the lego, determined by the pressed F-key, at the cursor in the active buffer.")
{
    F4_Lego *lego = F4_LegoFromUserInput(get_current_input(app));
    View_ID view = get_active_view(app, Access_Write);
    Buffer_ID buffer = view_get_buffer(app, view, Access_Write);
    if(buffer)
    {
        F4_Lego_BufferPlace(app, view, buffer, view_get_cursor_pos(app, view), lego);
    }
}

CUSTOM_COMMAND_SIG(f4_lego_store_token)
CUSTOM_DOC("Will store the token under the cursor into the lego determined by the associated F-key.")
{
    Scratch_Block scratch(app);
    F4_Lego *lego = F4_LegoFromUserInput(get_current_input(app));
    if(lego)
    {
        View_ID view = get_active_view(app, Access_Always);
        Buffer_ID buffer = view_get_buffer(app, view, Access_Always);
        Token *token = get_token_from_pos(app, buffer, view_get_cursor_pos(app, view));
        if(token != 0)
        {
            F4_Lego_Store(lego, F4_LegoKind_String, push_token_lexeme(app, scratch, buffer, token));
            F4_PushFlash(app, buffer, Ii64(token), fcolor_resolve(fcolor_id(fleury_color_lego_grab)), 0.8f);
        }
    }
}

CUSTOM_COMMAND_SIG(f4_lego_store_range)
CUSTOM_DOC("Will store the selected range into the lego determined by the associated F-key.")
{
    Scratch_Block scratch(app);
    F4_Lego *lego = F4_LegoFromUserInput(get_current_input(app));
    if(lego)
    {
        View_ID view = get_active_view(app, Access_Always);
        Buffer_ID buffer = view_get_buffer(app, view, Access_Always);
        Range_i64 range = Ii64(view_get_cursor_pos(app, view), view_get_mark_pos(app, view));
        F4_Lego_Store(lego, F4_LegoKind_String, push_buffer_range(app, scratch, buffer, range));
        F4_PushFlash(app, buffer, range, fcolor_resolve(fcolor_id(fleury_color_lego_grab)), 0.8f);
    }
}

CUSTOM_COMMAND_SIG(f4_lego_store_line)
CUSTOM_DOC("Will store the selected range into the lego determined by the associated F-key.")
{
    Scratch_Block scratch(app);
    F4_Lego *lego = F4_LegoFromUserInput(get_current_input(app));
    if(lego)
    {
        View_ID view = get_active_view(app, Access_Always);
        Buffer_ID buffer = view_get_buffer(app, view, Access_Always);
        i64 cursor_pos = view_get_cursor_pos(app, view);
        i64 line_num = get_line_number_from_pos(app, buffer, cursor_pos);
        Range_i64 range = get_line_pos_range(app, buffer, line_num);
        F4_Lego_Store(lego, F4_LegoKind_String, push_buffer_range(app, scratch, buffer, range));
        F4_PushFlash(app, buffer, range, fcolor_resolve(fcolor_id(fleury_color_lego_grab)), 0.8f);
    }
}

function void
F4_Lego_StoreClickedToken(Application_Links *app, F4_Lego *lego)
{
    Scratch_Block scratch(app);
    View_ID view = get_active_view(app, Access_Always);
    Buffer_ID buffer = view_get_buffer(app, view, Access_Always);
    Mouse_State mouse = get_mouse_state(app);
    i64 pos = view_pos_from_xy(app, view, V2f32(mouse.p));
    Token *token = get_token_from_pos(app, buffer, pos);
    if(token != 0)
    {
        F4_Lego_Store(lego, F4_LegoKind_String, push_token_lexeme(app, scratch, buffer, token));
        F4_PushFlash(app, buffer, Ii64(token), fcolor_resolve(fcolor_id(fleury_color_lego_grab)), 0.8f);
    }
}

CUSTOM_COMMAND_SIG(f4_lego_click_store_token_1)
CUSTOM_DOC("Sets the cursor to the clicked position, and then stores the token under that position into the F1 slot.")
{
    F4_Lego_StoreClickedToken(app, F4_LegoFromIndex(0));
}

CUSTOM_COMMAND_SIG(f4_lego_click_store_token_2)
CUSTOM_DOC("Sets the cursor to the clicked position, and then stores the token under that position into the F2 slot.")
{
    F4_Lego_StoreClickedToken(app, F4_LegoFromIndex(1));
}