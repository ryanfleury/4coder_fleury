/* date = December 17th 2020 6:16 pm */

#ifndef FCODER_FLEURY_INDEX_H
#define FCODER_FLEURY_INDEX_H

enum F4_Index_NoteKind
{
    F4_Index_NoteKind_Null,
    F4_Index_NoteKind_Scope,
    F4_Index_NoteKind_Type,
    F4_Index_NoteKind_Constant,
    F4_Index_NoteKind_Function,
    F4_Index_NoteKind_Decl,
    F4_Index_NoteKind_Macro,
    F4_Index_NoteKind_CommentTag,
    F4_Index_NoteKind_CommentToDo,
    F4_Index_NoteKind_COUNT
};

typedef u32 F4_Index_NoteFlags;
enum
{
    F4_Index_NoteFlag_Prototype   = (1<<0),
    F4_Index_NoteFlag_ProductType = (1<<1),
    F4_Index_NoteFlag_SumType     = (1<<2),
};

struct F4_Index_File;

struct F4_Index_Note
{
    F4_Index_Note *next;
    F4_Index_Note *prev;
    F4_Index_Note *hash_next;
	F4_Index_Note *hash_prev;
    F4_Index_Note *parent;
    F4_Index_Note *next_sibling;
    F4_Index_Note *prev_sibling;
    F4_Index_Note *first_child;
    F4_Index_Note *last_child;
    
    u64 hash;
    String_Const_u8 string;
    F4_Index_NoteKind kind;
    F4_Index_NoteFlags flags;
    Range_i64 range;
    F4_Index_File *file;
	int file_generation;
};

struct F4_Index_File
{
    F4_Index_File *hash_next;
    Arena arena;
    Buffer_ID buffer;
    F4_Index_Note *first_note;
    F4_Index_Note *last_note;
	int generation;
};

struct F4_Index_State
{
    System_Mutex mutex;
    Arena arena;
    F4_Index_Note *note_table[16384];
	F4_Index_Note *free_note;
    F4_Index_File *file_table[16384];
    F4_Index_File *free_file;
};

struct F4_Index_ParseCtx
{
    b32 done;
    Application_Links *app;
    F4_Index_File *file;
    String_Const_u8 string;
    Token_Array tokens;
    Token_Iterator_Array it;
    F4_Index_Note *active_parent;
};

typedef u32 F4_Index_TokenSkipFlags;
enum
{
    F4_Index_TokenSkipFlag_SkipWhitespace = (1<<0),
};

function void F4_Index_Initialize(void);
function void F4_Index_Lock(void);
function void F4_Index_Unlock(void);
function F4_Index_File *F4_Index_LookupFile(Application_Links *app, Buffer_ID buffer);
function F4_Index_File *F4_Index_LookupOrMakeFile(Application_Links *app, Buffer_ID buffer);
function void F4_Index_EraseFile(Application_Links *app, Buffer_ID id);
function void F4_Index_ClearFile(F4_Index_File *file);
function F4_Index_Note *F4_Index_LookupNote(String_Const_u8 string, F4_Index_Note *parent);
function F4_Index_Note *F4_Index_LookupNote(String_Const_u8 string);
function F4_Index_Note *F4_Index_AllocateNote(void);
function void F4_Index_InsertNote(F4_Index_ParseCtx *ctx, F4_Index_Note *note, Range_i64 name_range, F4_Index_NoteKind note_kind, F4_Index_NoteFlags note_flags);
function F4_Index_Note *F4_Index_MakeNote(F4_Index_ParseCtx *ctx, Range_i64 name_range, F4_Index_NoteKind note_kind, F4_Index_NoteFlags note_flags);
function void F4_Index_ParseFile(Application_Links *app, F4_Index_File *file, String_Const_u8 string, Token_Array tokens);
function b32 F4_Index_ParseCtx_Inc(F4_Index_ParseCtx *ctx, F4_Index_TokenSkipFlags flags);
function b32 F4_Index_RequireToken(F4_Index_ParseCtx *ctx, String_Const_u8 string, F4_Index_TokenSkipFlags flags);
function b32 F4_Index_RequireTokenKind(F4_Index_ParseCtx *ctx, Token_Base_Kind kind, Token **token_out, F4_Index_TokenSkipFlags flags);
function b32 F4_Index_RequireTokenSubKind(F4_Index_ParseCtx *ctx, int sub_kind, Token **token_out, F4_Index_TokenSkipFlags flags);
function b32 F4_Index_PeekToken(F4_Index_ParseCtx *ctx, String_Const_u8 string);
function void F4_Index_ParseComment(F4_Index_ParseCtx *ctx, Token *token);
function void F4_Index_SkipSoftTokens(F4_Index_ParseCtx *ctx, b32 preproc);
function void F4_Index_SkipOpTokens(F4_Index_ParseCtx *ctx);
function String_Const_u8 F4_Index_StringFromRange(F4_Index_ParseCtx *ctx, Range_i64 range);
function String_Const_u8 F4_Index_StringFromToken(F4_Index_ParseCtx *ctx, Token *token);
function F4_Index_Note *F4_Index_PushParent(F4_Index_ParseCtx *ctx, F4_Index_Note *new_parent);
function void F4_Index_PopParent(F4_Index_ParseCtx *ctx, F4_Index_Note *last_parent);
function void F4_Index_Tick(Application_Links *app);

// Format:
// %t -> token,         requires char * specifying token string
// %k -> token kind,    requires Token_Base_Kind and Token ** for output token
// %b -> token subkind, requires token subkind and Token ** for output token
// %n -> note,          requires F4_Index_NoteKind and F4_Index_Note ** for output note
// %s -> soft group,    requires no arguments
// %o -> operator group,requires no arguments
function b32 F4_Index_ParsePattern(F4_Index_ParseCtx *ctx, char *fmt, ...);

#endif // FCODER_FLEURY_INDEX_H
