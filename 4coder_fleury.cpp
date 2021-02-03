
//~ @f4_readme Ryan Fleury's 4coder Custom Layer
//
// Hello! Thank you for checking out my custom layer. This layer is a collection
// of things I've found to be useful, funny, or interesting overtime. It includes
// a few things that makes 4coder feel a bit more like an IDE, a few things that
// are just for kicks and are ridiculous, and a few interesting editor experiments.
// Below is a list of features, along with more information about them and some
// options you can tweak to your liking. To get as much customization as possible,
// you'd obviously have to dig a bit more into the code and probably write your
// own custom layer and just use stuff from this custom layer as a library, but
// the below options are just to allow easy customization of the low-hanging
// fruit.

//~ @f4_features 4coder Fleury Feature/Option List

//~ @f4_lang_intro Multi-Language Support
//
// This custom layer has been tweaked to allow for better multi-language support
// in 4coder. It isn't perfect and will not support all languages, but it works
// for more than C/C++ (which historically 4coder has been fairly coupled to). 
//
// List of supported languages (as of 2021/01/29):
// - C
// - C++, partially... if you do a lot of modern C++ (I hope not), nothing
//   me or 4coder do can help you much (in fact, maybe nothing can help you)
// - Jai
// - Metadesk
//
// Adding a language requires a few steps.
//
// 1. Adding a lexer (probably the hardest part)
// 
// 4coder comes bundled with some lexer generation things. They are hard for me
// to understand, but all the languages I've had to deal with have been fairly
// easy to adapt from C++'s lexer. You can take a look at 4coder_cpp_lexer_gen.cpp,
// and copy/paste it into a new file that can generate the lexer for whatever
// language. This is what I did for the Jai lexer. Take a look at build_lexers.bat
// to see what I do to generate the Jai lexing code.
//
// Alternatively, you can hand-roll a lexer. This is a lot simpler (but of course
// gets harder with more complex languages). You can see an example of this in
// the Metadesk language implementation (4coder_fleury_lang_metadesk.cpp).
//
// 2. Implement language hooks
//
// The language system has each language expose a few hooks in v-table-like
// fashion. This allows general editor code to "ask questions" about specific
// language concepts. Here is a list of the hooks:
//
// - IndexFile hook, implement w/ F4_LANGUAGE_INDEXFILE(name) { /* code */ }
//   Used to scan tokens of a file and do heuristic parses to pull out language
//   info, like function/type definitions and stuff.
//
// - LexInit hook, implement w/ F4_LANGUAGE_LEXINIT(name) { /* code */ }
//   A hook that lets you initialize any state you want for your lexer.
//   This is also one of the functions that can be generated for you with
//   the language lexer generator that comes with 4coder. For C++, the name
//   of the function is lex_full_input_cpp_init, so take that and adjust as
//   needed for your language.
//
// - LexFullInput hook, implement w/ F4_LANGUAGE_LEXFULLINPUT(name) { /* code */ }
//   A hook to lex the whole input of a file that is determined to be in a given
//   language. This is also a hook that can be generated with 4coder's lexer
//   generator stuff. For C++, the name of the function is lex_full_input_breaks,
//   so take that and adjust as needed for your language.
//
// - PosContext hook, implement using F4_LANGUAGE_POSCONTEXT(name) { /* code */ }
//   This hook is called into by some rendering features looking for contextual
//   information, usually about the cursor position. For example, figuring out
//   if the cursor is inside some arguments for a function, so a tooltip can be
//   displayed. You can just return 0 if you don't want to deal with this.
//
// - Highlight hook, implement using F4_LANGUAGE_HIGHLIGHT(name) { /* code */ }
//   This hook does any additional syntax highlighting that you want for the
//   language. You don't need to do anything here if you don't want to, in fact
//   you won't need this most of the time probably.
//
// 3. Add the info for your language to the 4coder_fleury_lang_list.h file.
// More information is in that file.

//~ @f4_colors_intro Syntax Highlighting and Color Additions
//
// 4coder provides very basic syntax highlighting, but this layer implements
// some more features, particularly ones that rely on the code index. There
// are custom colors that this layer looks for from a theme file so that it
// can use good colors for these. Additionally, many other features (e.g.
// various rendering features that are mentioned elsewhere in here) also need
// custom colors. Here is a list of the colors this layer looks for:
//
// - fleury_color_syntax_crap: Used to change the color of textual syntax stuff
//   that is not really semantically important, but we have to type it anyways,
//   stuff like ;'s and parentheses and stuff.
//
// - fleury_color_operators: The color of operators.
//
// - fleury_color_inactive_pane_overlay: The color of a panel-sized rectangle
//   drawn over panels that are not active, to leave the active panel slightly
//   brighter.
//
// - fleury_color_inactive_pane_background: The color of a panel-sized rectangle
//   drawn *behind* panels that are not active, to leave the active panel slightly
//   brighter.
//
// - fleury_color_file_progress_bar: The color to render the progress bar, drawn
//   over the file bar in a panel.
//
// - fleury_color_brace_highlight: The color used for the brace highlight feature.
//
// - fleury_color_brace_line: The color used for the brace line feature.
//
// - fleury_color_brace_annotation: The color used for the brace annotation
//   feature.
//
// - fleury_color_index_sum_type: The color used for names of sum-types
//   (unions).
//
// - fleury_color_index_product_type: The color used for names of product-
//   types (structs, classes, whatever).
//
// - fleury_color_index_function: The color used for names of functions.
//
// - fleury_color_index_macro: The color used for names of macros.
//
// - fleury_color_index_constant: The color used for names of constants
//   (including enum values).
//
// - fleury_color_index_comment_tag: The color used for comment tags,
//   like this! @example_comment_tag
//
// - fleury_color_cursor_macro: The color used for the cursor when a macro
//   is being recorded.
//
// - fleury_color_cursor_power_mode: The color used for the cursor when
//   Power Mode is activated.
//
// - fleury_color_plot_cycle: The set of colors used for colors in plots.
//
// - fleury_color_token_highlight: The color used to underline/highlight
//   tokens that the cursor is over.
//
// - fleury_color_comment_user_name: The color used to highlight the
//   username in comments.
//
// - fleury_color_border: The color used to highlight the
//   border around the active panel.


//~ @f4_render_intro Rendering Features
//
// Many of the features in this layer are just rendering tweaks/improvements to
// 4coder's defaults. Below is a list of features and some options for them.
//
// - Brace Highlighting: When the cursor is inside of a pair of {}'s, highlight 
//   the brace symbols. Turn it off with "f4_disable_brace_highlight" in your
//   config file.
//
// - Close-Brace Annotations: When the cursor is inside a pair of {}'s, BUT
//   you cannot see the top of the scope, render a little annotation to the
//   right of the }, showing the code that was before the {. Turn it off with
//   "f4_disable_close_brace_annotation" in your config file.
//
// - Brace Lines: In many editors, they will draw a vertical line connecting
//   { and }'s in a file. This feature does that when the cursor inside the
//   scope. Turn it off with "f4_disable_brace_lines" in your config file.
//
// - Progress Bar: Colors the file bar (at the top of a 4coder panel) with
//   a "progress bar", showing you how far your cursor is in a file. Turn it
//   off with "f4_disable_progress_bar" in your config file.
//
// - Divider Comments: Any single-line comments starting with a ~ character
//   (e.g. //~) will render a horizontal line over them, acting as a divider
//   (used as an alternative to gigantic lines of -------'s or whatever).
//   Turn it off with "f4_disable_divider_comments" in your config file.
//
// - Error Annotations: When an error is parsed from compilation output in the
//   *compilation* window, it will display what that error is to the right of
//   the line. Turn it off with "f4_disable_error_annotations" in your config
//   file.

//~ @f4_calc_intro Built-In Calculator/Plotting
//
// This custom layer includes a tiny little calculator language, which has
// plotting capabilities. It has a few features that comprise it all:
//
// - Calculator Comments: Any comments starting with a 'c', e.g. //c or /*c,
//   will be interpreted as calculator code, including inline plots. This
//   can be useful to do quick calculations, see a math function represented
//   visually, or plot data. You can check out examples/demos in the
//   4coder_fleury_plots_demo.cpp file. Turn it off with "f4_disable_calc_comments"
//   in your config file.
//
// - *calc* Buffer: On startup, a *calc* buffer is opened that you can switch
//   to. The entire buffer is interpreted as calculator code, and it will
//   do all the same things that calc comments do.

//~ @f4_pos_context_intro Language Position-Context Tooltips
//
// This feature shows tooltips relating to the current cursor position, as it
// adheres to the programming language of the buffer in question. For example,
// if a buffer's language has implemented the PosContext hook, then when the
// cursor is inside a function call, it will show a tooltip of the arguments,
// and which one the cursor is over. Same thing with struct listers.
//
// Normally, this renders at the cursor location, but you can make it render
// in a perhaps less-distracting location (at the bottom of the buffer) with
// "f4_poscontext_draw_at_bottom_of_buffer" in your config file.
//
// Alternatively, you can turn the whole thing off with "f4_disable_poscontext"
// in your config file.

//~ @f4_commands
//
// There are a lot of commands you can bind in your bindings.4coder file that
// have been helpful for me and others. Here is a list of the most useful ones.
// If you want to check out the code, go look at 4coder_fleury_base_commands.cpp.
//
// - f4_search: Will search forward in a buffer. When in notepad-like mode, will
//   use the current selection as the search query.
//
// - f4_reverse_search: Same thing as f4_search, just goes backwards.
//
// - f4_write_text_input: Use this instead of the default 4coder write_text_input
//   if you want Power Mode features.
//
// - f4_write_text_and_auto_indent: Same thing as above. Use over the default 4coder
//   equivalent if you want Power Mode features.
//
// - f4_write_zero_struct: 4coder provides a helper like this already, which writes
//   a C++-style '= {};' initializer, but this doesn't work for C (which uses '= {0}'),
//   yeah I know why the fuck don't we have a concise way to zero-initialize things
//   in both languages :) Anyways, bind this command to whatever you want if you want
//   to use it.
//
// - f4_home: 4coder obviously provides a default binding for the Home key, but it
//   doesn't move the view back over to the left margin, and that bothers me, so
//   I wrote a command that does exactly that.
//
// - f4_toggle_battery_saver: Some of the rendering features in this custom layer
//   include some animations, because I like animations. This will disable them,
//   so your laptop doesn't get hot. Set its default value with "f4_battery_saver"
//   in your config file.
//
// - f4_toggle_compilation_expand: Expands/shrinks the *compilation* panel at the
//   bottom of the screen. Bind this to a key that you want to press when you are
//   checking out the output of a build, and be sure to send your build commands
//   here!
//
// - f4_go_to_definition: Uses the identifier that the cursor is currently over
//   as a lookup into the code index. Will open its definition, if it finds one,
//   in the panel that the cursor is not in.
//
// - f4_go_to_definition_same_panel: Same as above except does it in the same
//   panel.
//
// - f4_search_for_definition__project_wide: Opens a lister with all definitions
//   from all the code that is open. You can search by typing into it, and then
//   navigating to the right item, and hitting enter. It will then take you to
//   the location of that definition.
//
// - f4_search_for_definition__current_file: Same as above, except restricts the
//   search to definitions in the file that your cursor is in.
//
// - f4_toggle_enclosure_side: When inside a scope or parentheses, moves the
//   cursor between the starting/ending position of that scope/parentheses.
//
// - f4_open_project: Opens a project file. Navigate to a project file on disk,
//   and when you select it, it will be loaded.
//
// - f4_setup_new_project: Makes a new project file at the location that you
//   specify.
//
// - f4_move_left: Move the cursor left. I have my own to correct for some
//   behavior issues with notepad-like mode. I've tried to converge to VS-like
//   behavior as much as possible with notepad-like mode, so use this if that's
//   what you want.
//
// - f4_move_right: Same as above, except right.
//
// - f4_move_right_token_boundary: Implements the equivalent of your VS-style
//   Ctrl+Right.
//
// - f4_move_left_token_boundary: Implements the equivalent of your VS-style
//   Ctrl+Left.
//
// - f4_backspace_token_boundary: Implements the equivalent of your VS-style
//   Ctrl+Backspace.
//
// - f4_delete_token_boundary: Implements the equivalent of your VS-style
//   Ctrl+Delete.
//
// - f4_backspace_alpha_numeric_or_camel_boundary: Does backspace, but restricts
//   it to camel-case and alphanumeric boundaries. Kind of like Ctrl+Backspace
//   except with different groupings.
//
// - f4_delete_alpha_numeric_or_camel_boundary: Same as above, just does it forward.
//
// - f4_home_first_non_whitespace: Implements VS-style Home key, where it first
//   goes to the first non-whitespace character on the line, then all the way to
//   the left margin, then back again. Bind this to Home if you want VS-like
//   behavior.
//
// - f4_autocomplete_or_indent: Autocompletes an incomplete word at the cursor, or
//   indents the line.
//
// - f4_unindent: Similar to Shift+Tab in VS.
//
// - f4_comment_selection: VS-style selection commenting.
//
// - f4_uncomment_selection: VS-style uncommenting.

//~ TODO FOR SHAWN
//
// [ ] Project File Opener: Ignore directories, recursively list options.
// [X] Ctrl+Arrows: It's still weird... it skips punctuation weridly.
// [ ] Recent Files Menu: this isn't bad!  If it just defaulted to the first one
//     (but still allowed typing), and if the key that opened it continued to cycle
//     the items in the list (rather than closing the lister), that'd be pretty much
//     it!
// [ ] Tabbing: This is really close, the only problem is if the end of the selection
//     isn't at the beginning of a line it does something really weird -- I'm not sure 
//     what.  It shouldn't matter where the caret is, if multiple lines are selected
//     then tab should move the lines
// [ ] Comments: Very close!  The only problem is the multiple selection, single line 
//     comments: instead of putting the // at the start of each line after the whitespace, 
//     the // should be lined up with the shallowest whitespace..
// [ ] Pressing tab on an empty line inside a scope still doesn't do anything.
// [X] If the caret is at the start of the line and you press home, it doesn't take you to 
//     the first non-whitespace character... basically pressing home more than once should 
//     cycle between the first non-whitespace character and column 0
// [X] Token-like movement inside of comments.
// [ ] ctrl E centres the buffer on the current line,  vim has z. There's:
//     zz -> middle, zt-> top, zb->bottom, but also: z5t will centre it 5
//     lines from the top or whatever... is that something that's easy to add? 
//
// ORIGINAL REQUEST:
// [ ] Fix not showing .-lister when cursor is at end of query
// [ ] Cycle overloads, or list them all
// [ ] Ctrl+I but just for the project buffers
// [ ] Vertical edits
//
// DONE:
//

//~ TODO PLOTS
// [ ] Un-Bust Histograms
// [ ] Fix plot clip rect bugs when the plot is not 100% in the visible range
//     (caused by 4coder laying out characters off the screen as being at 0, 0)
// [ ] Labels for histogram bins
// [ ] Plots and Calc are so coupled and I hate it, please rewrite or something

//~ TODO META
// [ ] Project-wide todo list (by searching for TODO and DONE comments, lister for toggling)
// [ ] Project switcher
// [ ] Plan + do modal input scheme... Identifier-mode, text-mode, semantics mode, search mode...?

//~ NOTE(rjf): For DION team docs server stuff.
// {
#if OS_WINDOWS
#include <WinSock2.h>
#include <Ws2tcpip.h>
#include <windows.h>
typedef int socklen_t;
#pragma comment(lib, "Ws2_32.lib")
#endif
// }

//~ NOTE(rjf): Macros and pragmase stuff that have to be put here for various
// reasons
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "4coder_default_include.cpp"
#pragma warning(disable : 4706)
#pragma warning(disable : 4456)
#define COMMAND_SERVER_PORT 4041
#define COMMAND_SERVER_UPDATE_PERIOD_MS 200
#define COMMAND_SERVER_AUTO_LAUNCH_IF_FILE_PRESENT "project_namespaces.txt"

//~ NOTE(rjf): @f4_headers
#include "4coder_fleury_ubiquitous.h"
#include "4coder_fleury_audio.h"
#include "4coder_fleury_lang.h"
#include "4coder_fleury_index.h"
#include "4coder_fleury_colors.h"
#include "4coder_fleury_render_helpers.h"
#include "4coder_fleury_brace.h"
#include "4coder_fleury_error_annotations.h"
#include "4coder_fleury_divider_comments.h"
#include "4coder_fleury_power_mode.h"
#include "4coder_fleury_cursor.h"
#include "4coder_fleury_plot.h"
#include "4coder_fleury_calc.h"
#include "4coder_fleury_pos_context_tooltips.h"
#include "4coder_fleury_code_peek.h"
#include "4coder_fleury_recent_files.h"
#include "4coder_fleury_bindings.h"
#include "4coder_fleury_base_commands.h"
#if OS_WINDOWS
#include "4coder_fleury_command_server.h"
#endif
#include "4coder_fleury_hooks.h"

//~ NOTE(rjf): @f4_src
#include "4coder_fleury_ubiquitous.cpp"
#include "4coder_fleury_audio.cpp"
#include "4coder_fleury_lang.cpp"
#include "4coder_fleury_index.cpp"
#include "4coder_fleury_colors.cpp"
#include "4coder_fleury_render_helpers.cpp"
#include "4coder_fleury_brace.cpp"
#include "4coder_fleury_error_annotations.cpp"
#include "4coder_fleury_divider_comments.cpp"
#include "4coder_fleury_power_mode.cpp"
#include "4coder_fleury_cursor.cpp"
#include "4coder_fleury_plot.cpp"
#include "4coder_fleury_calc.cpp"
#include "4coder_fleury_pos_context_tooltips.cpp"
#include "4coder_fleury_code_peek.cpp"
#include "4coder_fleury_recent_files.cpp"
#include "4coder_fleury_bindings.cpp"
#include "4coder_fleury_base_commands.cpp"
#if OS_WINDOWS
#include "4coder_fleury_command_server.cpp"
#endif
#include "4coder_fleury_casey.cpp"
#include "4coder_fleury_hooks.cpp"

//~ NOTE(rjf): Plots Demo File
#include "4coder_fleury_plots_demo.cpp"

//~ NOTE(rjf): 4coder Stuff
#include "generated/managed_id_metadata.cpp"

//~ NOTE(rjf): @f4_custom_layer_initialization

void custom_layer_init(Application_Links *app)
{
    default_framework_init(app);
    global_frame_arena = make_arena(get_base_allocator_system());
    permanent_arena = make_arena(get_base_allocator_system());
    
    // NOTE(rjf): Set up hooks.
    {
        set_all_default_hooks(app);
        //t $          ($  , $                             , $                     );
        set_custom_hook(app, HookID_Tick,                    F4_Tick);
        set_custom_hook(app, HookID_RenderCaller,            F4_Render);
        set_custom_hook(app, HookID_BeginBuffer,             F4_BeginBuffer);
        set_custom_hook(app, HookID_Layout,                  F4_Layout);
        set_custom_hook(app, HookID_WholeScreenRenderCaller, F4_WholeScreenRender);
        set_custom_hook(app, HookID_DeltaRule,               F4_DeltaRule);
        set_custom_hook(app, HookID_BufferEditRange,         F4_BufferEditRange);
        set_custom_hook_memory_size(app, HookID_DeltaRule, delta_ctx_size(sizeof(Vec2_f32)));
    }
    
    // NOTE(rjf): Set up mapping.
    {
        Thread_Context *tctx = get_thread_context(app);
        mapping_init(tctx, &framework_mapping);
        String_Const_u8 bindings_file = string_u8_litexpr("bindings.4coder");
        F4_SetAbsolutelyNecessaryBindings(&framework_mapping);
        if(!dynamic_binding_load_from_file(app, &framework_mapping, bindings_file))
        {
            F4_SetDefaultBindings(&framework_mapping);
        }
        F4_SetAbsolutelyNecessaryBindings(&framework_mapping);
    }
    
    // NOTE(rjf): Set up custom code index.
    {
        F4_Index_Initialize();
    }
    
    // NOTE(rjf): Register languages.
    {
        F4_RegisterLanguages();
    }
}


//~ NOTE(rjf): @f4_startup Whenever 4coder's core is ready for the custom layer to start up,
// this is called.

// TODO(rjf): This is only being used to check if a font file exists because
// there's a bug in try_create_new_face that crashes the program if a font is
// not found. This function is only necessary until that is fixed.
function b32
IsFileReadable(String_Const_u8 path)
{
    b32 result = 0;
    FILE *file = fopen((char *)path.str, "r");
    if(file)
    {
        result = 1;
        fclose(file);
    }
    return result;
}

CUSTOM_COMMAND_SIG(fleury_startup)
CUSTOM_DOC("Fleury startup event")
{
    ProfileScope(app, "default startup");
    
    User_Input input = get_current_input(app);
    if(!match_core_code(&input, CoreCode_Startup))
    {
        return;
    }
    
    //~ NOTE(rjf): Default 4coder initialization.
    String_Const_u8_Array file_names = input.event.core.file_names;
    load_themes_default_folder(app);
    default_4coder_initialize(app, file_names);
    
    //~ NOTE(rjf): Open special buffers.
    {
        
        // NOTE(rjf): Open compilation buffer.
        {
            Buffer_ID comp_buffer = create_buffer(app, string_u8_litexpr("*compilation*"),
                                                  BufferCreate_NeverAttachToFile |
                                                  BufferCreate_AlwaysNew);
            buffer_set_setting(app, comp_buffer, BufferSetting_Unimportant, true);
            buffer_set_setting(app, comp_buffer, BufferSetting_ReadOnly, true);
        }
        
        // NOTE(rjf): Open calc buffer.
        {
            Buffer_ID calc_buffer = create_buffer(app, string_u8_litexpr("*calc*"),
                                                  BufferCreate_NeverAttachToFile |
                                                  BufferCreate_AlwaysNew);
            buffer_set_setting(app, calc_buffer, BufferSetting_Unimportant, true);
        }
        
        // NOTE(rjf): Open peek buffer.
        {
            Buffer_ID peek_buffer = create_buffer(app, string_u8_litexpr("*peek*"),
                                                  BufferCreate_NeverAttachToFile |
                                                  BufferCreate_AlwaysNew);
            buffer_set_setting(app, peek_buffer, BufferSetting_Unimportant, true);
        }
    }
    
    //~ NOTE(rjf): Initialize panels
    {
        Buffer_Identifier comp = buffer_identifier(string_u8_litexpr("*compilation*"));
        Buffer_Identifier left  = buffer_identifier(string_u8_litexpr("*calc*"));
        Buffer_Identifier right = buffer_identifier(string_u8_litexpr("*messages*"));
        Buffer_ID comp_id = buffer_identifier_to_id(app, comp);
        Buffer_ID left_id = buffer_identifier_to_id(app, left);
        Buffer_ID right_id = buffer_identifier_to_id(app, right);
        
        // NOTE(rjf): Left Panel
        View_ID view = get_active_view(app, Access_Always);
        new_view_settings(app, view);
        view_set_buffer(app, view, left_id, 0);
        
        // NOTE(rjf): Bottom panel
        View_ID compilation_view = 0;
        {
            compilation_view = open_view(app, view, ViewSplit_Bottom);
            new_view_settings(app, compilation_view);
            Buffer_ID buffer = view_get_buffer(app, compilation_view, Access_Always);
            Face_ID face_id = get_face_id(app, buffer);
            Face_Metrics metrics = get_face_metrics(app, face_id);
            view_set_split_pixel_size(app, compilation_view, (i32)(metrics.line_height*4.f));
            view_set_passive(app, compilation_view, true);
        }
        view_set_buffer(app, compilation_view, comp_id, 0);
        view_set_active(app, view);
        global_compilation_view = compilation_view;
        
        // NOTE(rjf): Right Panel
        open_panel_vsplit(app);
        
        View_ID right_view = get_active_view(app, Access_Always);
        view_set_buffer(app, right_view, right_id, 0);
        
        // NOTE(rjf): Restore Active to Left
        view_set_active(app, view);
    }
    
    //~ NOTE(rjf): Auto-Load Project.
    {
        b32 auto_load = def_get_config_b32(vars_save_string_lit("automatically_load_project"));
        if (auto_load)
        {
            load_project(app);
        }
    }
    
    //~ NOTE(rjf): Initialize audio.
    {
        def_audio_init();
    }
    
    //~ NOTE(rjf): Initialize bindings.
    {
        String_Const_u8 bindings_file = string_u8_litexpr("bindings.4coder");
        F4_SetAbsolutelyNecessaryBindings(&framework_mapping);
        if(!dynamic_binding_load_from_file(app, &framework_mapping, bindings_file))
        {
            F4_SetDefaultBindings(&framework_mapping);
        }
        F4_SetAbsolutelyNecessaryBindings(&framework_mapping);
    }
    
    //~ NOTE(rjf): Initialize stylish fonts.
    {
        Scratch_Block scratch(app);
        String_Const_u8 bin_path = system_get_path(scratch, SystemPath_Binary);
        
        // NOTE(rjf): Fallback font.
        Face_ID face_that_should_totally_be_there = get_face_id(app, 0);
        
        // NOTE(rjf): Title font.
        {
            Face_Description desc = {0};
            {
                desc.font.file_name =  push_u8_stringf(scratch, "%.*sfonts/RobotoCondensed-Regular.ttf", string_expand(bin_path));
                desc.parameters.pt_size = 18;
                desc.parameters.bold = 0;
                desc.parameters.italic = 0;
                desc.parameters.hinting = 0;
            }
            
            if(IsFileReadable(desc.font.file_name))
            {
                global_styled_title_face = try_create_new_face(app, &desc);
            }
            else
            {
                global_styled_title_face = face_that_should_totally_be_there;
            }
        }
        
        // NOTE(rjf): Label font.
        {
            Face_Description desc = {0};
            {
                desc.font.file_name =  push_u8_stringf(scratch, "%.*sfonts/RobotoCondensed-Regular.ttf", string_expand(bin_path));
                desc.parameters.pt_size = 10;
                desc.parameters.bold = 1;
                desc.parameters.italic = 1;
                desc.parameters.hinting = 0;
            }
            
            if(IsFileReadable(desc.font.file_name))
            {
                global_styled_label_face = try_create_new_face(app, &desc);
            }
            else
            {
                global_styled_label_face = face_that_should_totally_be_there;
            }
        }
        
        // NOTE(rjf): Small code font.
        {
            Face_Description normal_code_desc = get_face_description(app, get_face_id(app, 0));
            
            Face_Description desc = {0};
            {
                desc.font.file_name =  push_u8_stringf(scratch, "%.*sfonts/Inconsolata-Regular.ttf", string_expand(bin_path));
                desc.parameters.pt_size = normal_code_desc.parameters.pt_size - 1;
                desc.parameters.bold = 1;
                desc.parameters.italic = 1;
                desc.parameters.hinting = 0;
            }
            
            if(IsFileReadable(desc.font.file_name))
            {
                global_small_code_face = try_create_new_face(app, &desc);
            }
            else
            {
                global_small_code_face = face_that_should_totally_be_there;
            }
        }
    }
    
    //~ NOTE(rjf): Prep virtual whitespace.
    {
        def_enable_virtual_whitespace = def_get_config_b32(vars_save_string_lit("enable_virtual_whitespace"));
        clear_all_layouts(app);
    }
}
