
typedef u32 F4_SyntaxFlags;
enum
{
    F4_SyntaxFlag_Functions    = (1<<0),
    F4_SyntaxFlag_Macros       = (1<<1),
    F4_SyntaxFlag_Types        = (1<<2),
    F4_SyntaxFlag_Operators    = (1<<3),
    F4_SyntaxFlag_Constants    = (1<<4),
    F4_SyntaxFlag_Literals     = (1<<5),
    F4_SyntaxFlag_Preprocessor = (1<<6),
    F4_SyntaxFlag_Keywords     = (1<<7),
    F4_SyntaxFlag_HighlightAll = (1<<15),
};
#define F4_SyntaxFlag_All 0xffffffff

struct F4_SyntaxOptions
{
    String8 name;
    F4_SyntaxFlags flags;
};

global f32 f4_syntax_flag_transitions[32] = {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,};
global F4_SyntaxOptions f4_syntax_opts[] =
{
    { S8Lit("All"),            F4_SyntaxFlag_All },
    { S8Lit("None"),           0 },
    { S8Lit("Functions Only"), F4_SyntaxFlag_Functions },
    { S8Lit("Macros Only"),    F4_SyntaxFlag_Macros },
    { S8Lit("Function-Likes Only"), F4_SyntaxFlag_Functions | F4_SyntaxFlag_Macros },
    { S8Lit("Types Only"),     F4_SyntaxFlag_Types },
    { S8Lit("Externals Only"), F4_SyntaxFlag_Functions | F4_SyntaxFlag_Macros | F4_SyntaxFlag_Types | F4_SyntaxFlag_Constants },
};
global i32 f4_active_syntax_opt_idx = 0;

function b32
F4_ARGBIsValid(ARGB_Color color)
{
    return color != 0xFF990099;
}

internal void
F4_TickColors(Application_Links *app, Frame_Info frame_info)
{
    F4_SyntaxOptions opts = f4_syntax_opts[f4_active_syntax_opt_idx];
    for(int i = 0; i < sizeof(F4_SyntaxFlags)*8; i += 1)
    {
        f32 delta = ((f32)!!(opts.flags & (1<<i)) - f4_syntax_flag_transitions[i]) * frame_info.animation_dt * 8.f;
        f4_syntax_flag_transitions[i] += delta;
        if(fabsf(delta) > 0.001f)
        {
            animate_in_n_milliseconds(app, 0);
        }
    }
}

CUSTOM_COMMAND_SIG(f4_switch_syntax_option)
CUSTOM_DOC("Switches the syntax highlighting mode.")
{
    f4_active_syntax_opt_idx = (f4_active_syntax_opt_idx + 1) % ArrayCount(f4_syntax_opts);
}

internal String8
F4_SyntaxOptionString(void)
{
    return f4_syntax_opts[f4_active_syntax_opt_idx].name;
}

typedef u32 ColorFlags;
enum
{
    ColorFlag_Macro = (1<<0),
    ColorFlag_PowerMode = (1<<1),
};

struct ColorCtx
{
    Token token;
    Buffer_ID buffer;
    ColorFlags flags;
    keybinding_mode mode;
};

internal ColorCtx
ColorCtx_Token(Token token, Buffer_ID buffer)
{
    ColorCtx ctx = {0};
    ctx.token = token;
    ctx.buffer = buffer;
    return ctx;
}

internal ColorCtx
ColorCtx_Cursor(ColorFlags flags, keybinding_mode mode)
{
    ColorCtx ctx = {0};
    ctx.flags = flags;
    ctx.mode = mode;
    return ctx;
}

static ARGB_Color
F4_ARGBFromID(Color_Table table, Managed_ID id, int subindex)
{
    ARGB_Color result = 0;
    FColor color = fcolor_id(id);
    if (color.a_byte == 0){
        if (color.id != 0){
            result = finalize_color(table, color.id, subindex);
        }
    }
    else{
        result = color.argb;
    }
    return(result);
}

static ARGB_Color
F4_ARGBFromID(Color_Table table, Managed_ID id)
{
    return F4_ARGBFromID(table, id, 0);
}

internal ARGB_Color
F4_GetColor(Application_Links *app, ColorCtx ctx)
{
    Color_Table table = active_color_table;
    ARGB_Color default_color = F4_ARGBFromID(table, defcolor_text_default);
    ARGB_Color color = default_color;
    f32 t = 1;
    
#define FillFromFlag(flag) do{ t = f4_syntax_flag_transitions[BitOffset(flag)]; }while(0)
    
    //~ NOTE(rjf): Token Color
    if(ctx.token.size != 0)
    {
        Scratch_Block scratch(app);
        
        switch(ctx.token.kind)
        {
            case TokenBaseKind_Identifier:
            {
                String_Const_u8 string = push_buffer_range(app, scratch, ctx.buffer, Ii64(ctx.token.pos, ctx.token.pos + ctx.token.size));
                F4_Index_Note *note = F4_Index_LookupNote(string);
                if(note)
                {
                    color = 0xffff0000;
                    switch(note->kind)
                    {
                        case F4_Index_NoteKind_Type:
                        {
                            FillFromFlag(F4_SyntaxFlag_Types);
                            color = F4_ARGBFromID(table,
                                                  note->flags & F4_Index_NoteFlag_SumType
                                                  ? fleury_color_index_sum_type
                                                  : fleury_color_index_product_type);
                        }break;
                        
                        case F4_Index_NoteKind_Macro:
                        {
                            FillFromFlag(F4_SyntaxFlag_Macros);
                            color = F4_ARGBFromID(table, fleury_color_index_macro);
                        }break;
                        
                        case F4_Index_NoteKind_Function:
                        {
                            FillFromFlag(F4_SyntaxFlag_Functions);
                            color = F4_ARGBFromID(table, fleury_color_index_function);
                        }break;
                        
                        case F4_Index_NoteKind_Constant:
                        {
                            FillFromFlag(F4_SyntaxFlag_Constants);
                            color = F4_ARGBFromID(table, fleury_color_index_constant);
                        }break;
                        
                        case F4_Index_NoteKind_Decl:
                        {
                            FillFromFlag(F4_SyntaxFlag_Constants);
                            color = F4_ARGBFromID(table, fleury_color_index_decl);
                        }break;
                        
                        default: color = 0xffff00ff; break;
                    }
                    
                    if(!F4_ARGBIsValid(color)) { color = default_color; }
                }
                
            }break;
            
            case TokenBaseKind_Preprocessor:     { FillFromFlag(F4_SyntaxFlag_Preprocessor); color = F4_ARGBFromID(table, defcolor_preproc); } break;
            case TokenBaseKind_Keyword:          { FillFromFlag(F4_SyntaxFlag_Keywords); color = F4_ARGBFromID(table, defcolor_keyword); } break;
            case TokenBaseKind_Comment:          { color = F4_ARGBFromID(table, defcolor_comment); } break;
            case TokenBaseKind_LiteralString:    { FillFromFlag(F4_SyntaxFlag_Literals); color = F4_ARGBFromID(table, defcolor_str_constant); } break;
            case TokenBaseKind_LiteralInteger:   { FillFromFlag(F4_SyntaxFlag_Literals); color = F4_ARGBFromID(table, defcolor_int_constant); } break;
            case TokenBaseKind_LiteralFloat:     { FillFromFlag(F4_SyntaxFlag_Literals); color = F4_ARGBFromID(table, defcolor_float_constant); } break;
            case TokenBaseKind_Operator:         { FillFromFlag(F4_SyntaxFlag_Operators); color = F4_ARGBFromID(table, fleury_color_operators); if(!F4_ARGBIsValid(color)) { color = default_color; } } break;
            
            case TokenBaseKind_ScopeOpen:
            case TokenBaseKind_ScopeClose:
            case TokenBaseKind_ParentheticalOpen:
            case TokenBaseKind_ParentheticalClose:
            case TokenBaseKind_StatementClose:
            {
                color = F4_ARGBFromID(table, fleury_color_syntax_crap);
                if(!F4_ARGBIsValid(color)) { color = default_color; }
                break;
            }
            
            default:
            {
                switch(ctx.token.sub_kind)
                {
                    case TokenCppKind_LiteralTrue:
                    case TokenCppKind_LiteralFalse:
                    {
                        color = F4_ARGBFromID(table, defcolor_bool_constant);
                        FillFromFlag(F4_SyntaxFlag_Literals); 
                        break;
                    }
                    case TokenCppKind_LiteralCharacter:
                    case TokenCppKind_LiteralCharacterWide:
                    case TokenCppKind_LiteralCharacterUTF8:
                    case TokenCppKind_LiteralCharacterUTF16:
                    case TokenCppKind_LiteralCharacterUTF32:
                    {
                        color = F4_ARGBFromID(table, defcolor_char_constant);
                        FillFromFlag(F4_SyntaxFlag_Literals); 
                        break;
                    }
                    case TokenCppKind_PPIncludeFile:
                    {
                        color = F4_ARGBFromID(table, defcolor_include);
                        FillFromFlag(F4_SyntaxFlag_Literals); 
                        break;
                    }
                }
            }break;
            
        }
    }
    
    //~ NOTE(rjf): Cursor Color
    else
    {
        if(ctx.flags & ColorFlag_Macro)
        {
            color = F4_ARGBFromID(table, fleury_color_cursor_macro);
        }
        else if(ctx.flags & ColorFlag_PowerMode)
        {
            color = F4_ARGBFromID(table, fleury_color_cursor_power_mode);
        }
        else
        {
            color = F4_ARGBFromID(table, defcolor_cursor, ctx.mode);
        }
    }
    
    return color_blend(default_color, t, color);
}

static void
F4_SyntaxHighlight(Application_Links *app, Text_Layout_ID text_layout_id, Token_Array *array)
{
    Color_Table table = active_color_table;
    Buffer_ID buffer = text_layout_get_buffer(app, text_layout_id);
    Range_i64 visible_range = text_layout_get_visible_range(app, text_layout_id);
    i64 first_index = token_index_from_pos(array, visible_range.first);
    Token_Iterator_Array it = token_iterator_index(0, array, first_index);
    ARGB_Color comment_tag_color = F4_ARGBFromID(table, fleury_color_index_comment_tag, 0);
    
    for(;;)
    {
        Token *token = token_it_read(&it);
        if(!token || token->pos >= visible_range.one_past_last)
        {
            break;
        }
        ARGB_Color argb = F4_GetColor(app, ColorCtx_Token(*token, buffer));
        paint_text_color(app, text_layout_id, Ii64_size(token->pos, token->size), argb);
        
        // NOTE(rjf): Substrings from comments
        if(F4_ARGBIsValid(comment_tag_color))
        {
            if(token->kind == TokenBaseKind_Comment)
            {
                Scratch_Block scratch(app);
                String_Const_u8 string = push_buffer_range(app, scratch, buffer, Ii64(token->pos, token->pos + token->size));
                for(u64 i = 0; i < string.size; i += 1)
                {
                    if(string.str[i] == '@')
                    {
                        u64 j = i+1;
                        for(; j < string.size; j += 1)
                        {
                            if(character_is_whitespace(string.str[j]))
                            {
                                break;
                            }
                        }
                        paint_text_color(app, text_layout_id, Ii64(token->pos + (i64)i, token->pos + (i64)j), comment_tag_color);
                    }
                }
            }
        }
        
        if(!token_it_inc_all(&it))
        {
            break;
        }
    }
    
    F4_Language *lang = F4_LanguageFromBuffer(app, buffer);
    if(lang != 0 && lang->Highlight != 0)
    {
        lang->Highlight(app, text_layout_id, array, table);
    }
    
}
