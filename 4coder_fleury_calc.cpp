static f32 global_calc_time = 0.f;

function void
F4_CLC_Tick(Frame_Info frame_info)
{
    global_calc_time += frame_info.literal_dt;
}

enum CalcTokenType
{
    CalcTokenType_Invalid,
    CalcTokenType_Identifier,
    CalcTokenType_SourceCodeIdentifier,
    CalcTokenType_Number,
    CalcTokenType_Symbol,
    CalcTokenType_StringConstant,
};

typedef struct CalcToken CalcToken;
struct CalcToken
{
    CalcTokenType type;
    char *string;
    int string_length;
};

static CalcToken
GetNextCalcToken(char *buffer)
{
    CalcToken token = { CalcTokenType_Invalid };
    
    enum
    {
        ReadMode_Normal,
        ReadMode_SingleLineComment,
        ReadMode_MultilineComment,
    };
    int read_mode = 0;
    
    if(buffer)
    {
        for(int i = 0; buffer[i]; ++i)
        {
            if(read_mode == ReadMode_SingleLineComment)
            {
                if(buffer[i] == '\n')
                {
                    read_mode = ReadMode_Normal;
                }
            }
            else if(read_mode == ReadMode_MultilineComment)
            {
                if(buffer[i] == '#' && buffer[i+1] == '#')
                {
                    read_mode = ReadMode_Normal;
                }
                else if(buffer[i] == '*' && buffer[i+1] == '/')
                {
                    read_mode = ReadMode_Normal;
                }
            }
            else
            {
                if(buffer[i] == '#')
                {
                    if(buffer[i+1] == '#')
                    {
                        read_mode = ReadMode_MultilineComment;
                    }
                    else
                    {
                        read_mode = ReadMode_SingleLineComment;
                    }
                }
                else if(buffer[i] == '/' && (buffer[i+1] == '/' || buffer[i+1] == '*'))
                {
                    if(buffer[i+1] == '/')
                    {
                        read_mode = ReadMode_SingleLineComment;
                    }
                    else
                    {
                        read_mode = ReadMode_MultilineComment;
                    }
                }
                else if(buffer[i] == '@')
                {
                    token.type = CalcTokenType_SourceCodeIdentifier;
                    token.string = buffer+i+1;
                    int j;
                    for(j = i+1; buffer[j] &&
                        (CharIsDigit(buffer[j]) || buffer[j] == '_' ||
                         CharIsAlpha(buffer[j]));
                        ++j);
                    token.string_length = j - i - 1;
                    break;
                }
                else if(CharIsAlpha(buffer[i]))
                {
                    token.type = CalcTokenType_Identifier;
                    token.string = buffer+i;
                    int j;
                    for(j = i+1; buffer[j] &&
                        (CharIsDigit(buffer[j]) || buffer[j] == '_' ||
                         CharIsAlpha(buffer[j]));
                        ++j);
                    token.string_length = j - i;
                    break;
                }
                else if(CharIsDigit(buffer[i]))
                {
                    token.type = CalcTokenType_Number;
                    token.string = buffer+i;
                    int j;
                    for(j = i+1; buffer[j] &&
                        (CharIsDigit(buffer[j]) || buffer[j] == '.' ||
                         CharIsAlpha(buffer[j]));
                        ++j);
                    token.string_length = j - i;
                    break;
                }
                else if(CharIsSymbol(buffer[i]))
                {
                    token.type = CalcTokenType_Symbol;
                    token.string = buffer+i;
                    
                    // NOTE(rjf): Assumes 1-length symbols. Might not always be true.
                    int j = i+1;
                    // for(j = i+1; buffer[j] && CharIsSymbol(buffer[j]); ++j);
                    
                    token.string_length = j - i;
                    break;
                }
                else if(buffer[i] == '"' || buffer[i] == '\'')
                {
                    int starting_char = buffer[i];
                    token.type = CalcTokenType_StringConstant;
                    token.string = buffer+i;
                    int j;
                    for(j = i+1; buffer[j] && buffer[j] != starting_char; ++j);
                    token.string_length = j - i + 1;
                    break;
                }
            }
        }
    }
    
    return token;
}

static CalcToken
NextCalcToken(char **at)
{
    CalcToken token = GetNextCalcToken(*at);
    *at = token.string + token.string_length;
    return token;
}

static CalcToken
PeekCalcToken(char **at)
{
    CalcToken token = GetNextCalcToken(*at);
    return token;
}

static int
CalcTokenMatch(CalcToken token, char *str)
{
    int match = 0;
    
    if(token.string && token.string_length > 0 &&
       token.type != CalcTokenType_Invalid)
    {
        match = 1;
        for(int i = 0; i < token.string_length; ++i)
        {
            if(token.string[i] == str[i])
            {
                if(i == token.string_length-1)
                {
                    if(str[i+1] != 0)
                    {
                        match = 0;
                        break;
                    }
                }
            }
            else
            {
                match = 0;
                break;
            }
        }
    }
    return match;
}

static int
RequireCalcToken(char **at, char *str)
{
    int result = 0;
    CalcToken token = GetNextCalcToken(*at);
    if(CalcTokenMatch(token, str))
    {
        result = 1;
        *at = token.string + token.string_length;
    }
    return result;
}

static int
RequireCalcTokenType(char **at, CalcTokenType type, CalcToken *token_ptr)
{
    int result = 0;
    CalcToken token = GetNextCalcToken(*at);
    if(token.type == type)
    {
        result = 1;
        *at = token.string + token.string_length;
        if(token_ptr)
        {
            *token_ptr = token;
        }
    }
    return result;
}

static int
RequireNewline(char **at)
{
    int result = 0;
    
    CalcToken next_token = PeekCalcToken(at);
    char *newline = 0;
    for(int i = 0; (*at)[i]; ++i)
    {
        if((*at)[i] == '\n')
        {
            newline = (*at)+i;
            break;
        }
    }
    
    if(newline)
    {
        if(next_token.string > newline)
        {
            result = 1;
        }
    }
    
    return result;
}

static int
RequireEndOfBuffer(char **at)
{
    int result = 0;
    CalcToken next_token = PeekCalcToken(at);
    result = next_token.string == 0;
    return result;
}


//~ NOTE(rjf): Calc node-types and types.

#define CalcNodeType_LIST              \
CalcNodeType(Invalid,                0)\
CalcNodeType(Error,                  0)\
CalcNodeType(Number,                 0)\
CalcNodeType(StringConstant,         0)\
CalcNodeType(Array,                  0)\
CalcNodeType(Identifier,             0)\
CalcNodeType(SourceCodeIdentifier,   0)\
CalcNodeType(FunctionCall,           0)\
CalcNodeType(Add,                    1)\
CalcNodeType(Subtract,               1)\
CalcNodeType(Multiply,               2)\
CalcNodeType(Divide,                 2)\
CalcNodeType(Modulus,                2)\
CalcNodeType(RaiseToPower,           0)\
CalcNodeType(Negate,                 0)\
CalcNodeType(Assignment,             0)\
CalcNodeType(ArrayIndex,             0)\

#define CalcType_LIST                                    \
CalcType(Error,                 "error")                \
CalcType(None,                  "none")                 \
CalcType(Number,                "number")               \
CalcType(Array,                 "array")                \
CalcType(String,                "string")               \
CalcType(SourceCodeReference,   "source code reference")



//~

enum CalcNodeType
{
#define CalcNodeType(name, precedence) CalcNodeType_##name,
    CalcNodeType_LIST
#undef CalcNodeType
};

static int
CalcOperatorPrecedence(CalcNodeType type)
{
    static int precedence_table[] =
    {
#define CalcNodeType(name, precedence) precedence,
        CalcNodeType_LIST
#undef CalcNodeType
    };
    return precedence_table[type];
}

enum CalcType
{
#define CalcType(name, str) CalcType_##name,
    CalcType_LIST
#undef CalcType
};

static char *
CalcTypeName(CalcType type)
{
    static char *name_table[] =
    {
#define CalcType(name, str) str,
        CalcType_LIST
#undef CalcType
    };
    return name_table[type];
}

//~

typedef struct CalcNode CalcNode;
struct CalcNode
{
    CalcNodeType type;
    double value;
    union
    {
        CalcNode *operand;
        CalcNode *left;
    };
    CalcNode *right;
    union
    {
        CalcNode *first_parameter;
        CalcNode *first_member;
    };
    CalcNode *next;
    CalcToken token;
    int num_params;
    String_Const_u8 error_string;
    char *at_source;
};

static CalcNode *
AllocateCalcNode(Arena *arena, CalcNodeType type, char *at_source)
{
    CalcNode *node = push_array(arena, CalcNode, 1);
    MemorySet(node, 0, sizeof(*node));
    node->type = type;
    node->at_source = at_source;
    return node;
}

static CalcNode *
ErrorCalcNode(Arena *arena, char *format, ...)
{
    CalcNode *node = push_array(arena, CalcNode, 1);
    MemorySet(node, 0, sizeof(*node));
    node->type = CalcNodeType_Error;
    va_list args;
    va_start(args, format);
    node->error_string = push_stringfv(arena, format, args);
    va_end(args);
    return node;
}

static CalcNode *ParseCalcExpression(Arena *arena, char **at_ptr);

static CalcNode *
ParseCalcUnaryExpression(Arena *arena, char **at_ptr)
{
    CalcNode *expression = 0;
    CalcToken token = PeekCalcToken(at_ptr);
    char *at_source = token.string;
    
    if(CalcTokenMatch(token, "-"))
    {
        NextCalcToken(at_ptr);
        expression = AllocateCalcNode(arena, CalcNodeType_Negate, at_source);
        expression->operand = ParseCalcUnaryExpression(arena, at_ptr);
    }
    else if(token.type == CalcTokenType_SourceCodeIdentifier)
    {
        NextCalcToken(at_ptr);
        expression = AllocateCalcNode(arena, CalcNodeType_SourceCodeIdentifier, at_source);
        expression->token = token;
    }
    else if(token.type == CalcTokenType_Identifier)
    {
        NextCalcToken(at_ptr);
        
        // NOTE(rjf): Function call.
        if(RequireCalcToken(at_ptr, "("))
        {
            expression = AllocateCalcNode(arena, CalcNodeType_FunctionCall, at_source);
            expression->token = token;
            
            CalcNode **target_param = &expression->first_parameter;
            for(;;)
            {
                CalcToken next_token = PeekCalcToken(at_ptr);
                if(next_token.type == CalcTokenType_Invalid ||
                   CalcTokenMatch(next_token, ")"))
                {
                    break;
                }
                
                CalcNode *param = ParseCalcExpression(arena, at_ptr);
                
                if(param)
                {
                    *target_param = param;
                    target_param = &(*target_param)->next;
                    RequireCalcToken(at_ptr, ",");
                }
                else
                {
                    expression = ErrorCalcNode(arena, "Invalid parameter.");
                    goto end_parse;
                }
            }
            
            if(!RequireCalcToken(at_ptr, ")"))
            {
                expression = ErrorCalcNode(arena, "Missing ')'.");
                goto end_parse;
            }
        }
        
        // NOTE(rjf): Constant or variable.
        else
        {
            expression = AllocateCalcNode(arena, CalcNodeType_Identifier, at_source);
            expression->token = token;
        }
    }
    else if(CalcTokenMatch(token, "("))
    {
        NextCalcToken(at_ptr);
        expression = ParseCalcExpression(arena, at_ptr);
        RequireCalcToken(at_ptr, ")");
    }
    else if(token.type == CalcTokenType_Number)
    {
        NextCalcToken(at_ptr);
        expression = AllocateCalcNode(arena, CalcNodeType_Number, at_source);
        expression->value = GetFirstDoubleFromBuffer(token.string);
    }
    else if(token.type == CalcTokenType_StringConstant)
    {
        NextCalcToken(at_ptr);
        expression = AllocateCalcNode(arena, CalcNodeType_StringConstant, at_source);
        expression->token = token;
    }
    else if(CalcTokenMatch(token, "["))
    {
        NextCalcToken(at_ptr);
        
        expression = AllocateCalcNode(arena, CalcNodeType_Array, at_source);
        CalcNode **target_member = &expression->first_member;
        
        for(;;)
        {
            token = PeekCalcToken(at_ptr);
            if(CalcTokenMatch(token, "]") || token.type == CalcTokenType_Invalid)
            {
                break;
            }
            
            CalcNode *member_expression = ParseCalcExpression(arena, at_ptr);
            if(!RequireCalcToken(at_ptr, ","))
            {
                expression = ErrorCalcNode(arena, "Missing ','.");
                goto end_parse;
            }
            if(member_expression)
            {
                *target_member = member_expression;
                target_member = &(*target_member)->next;
            }
            else
            {
                break;
            }
        }
        
        RequireCalcToken(at_ptr, "]");
    }
    
    if(RequireCalcToken(at_ptr, "^"))
    {
        CalcNode *old_expr = expression;
        expression = AllocateCalcNode(arena, CalcNodeType_RaiseToPower, at_source);
        expression->left = old_expr;
        expression->right = ParseCalcUnaryExpression(arena, at_ptr);
    }
    
    // NOTE(rjf): Array index.
    if(expression)
    {
        while(RequireCalcToken(at_ptr, "["))
        {
            CalcNode *old_expr = expression;
            expression = AllocateCalcNode(arena, CalcNodeType_ArrayIndex, at_source);
            expression->token = token;
            expression->left = old_expr;
            expression->right = ParseCalcExpression(arena, at_ptr);
            
            if(!expression->right)
            {
                expression = ErrorCalcNode(arena, "Missing array index inside of '[' and ']'.");
                goto end_parse;
            }
            
            if(!RequireCalcToken(at_ptr, "]"))
            {
                expression = ErrorCalcNode(arena, "Missing ']'.");
                goto end_parse;
            }
        }
    }
    
    end_parse:;
    return expression;
}

static CalcNodeType
GetCalcBinaryOperatorTypeFromToken(CalcToken token)
{
    CalcNodeType type = CalcNodeType_Invalid;
    switch(token.type)
    {
        case CalcTokenType_Symbol:
        {
            if(token.string[0] == '+')
            {
                type = CalcNodeType_Add;
            }
            else if(token.string[0] == '-')
            {
                type = CalcNodeType_Subtract;
            }
            else if(token.string[0] == '*')
            {
                type = CalcNodeType_Multiply;
            }
            else if(token.string[0] == '/')
            {
                type = CalcNodeType_Divide;
            }
            else if(token.string[0] == '%')
            {
                type = CalcNodeType_Modulus;
            }
            break;
        }
        default: break;
    }
    return type;
}

static CalcNode *
ParseCalcExpression_(Arena *arena, char **at_ptr, int precedence_in)
{
    CalcNode *expression = ParseCalcUnaryExpression(arena, at_ptr);
    
    if(expression)
    {
        CalcToken token = PeekCalcToken(at_ptr);
        CalcNodeType operator_type = GetCalcBinaryOperatorTypeFromToken(token);
        
        char *at_source = token.string;
        
        if(token.string && operator_type != CalcNodeType_Invalid &&
           operator_type != CalcNodeType_Number)
        {
            for(int precedence = CalcOperatorPrecedence(operator_type);
                precedence >= precedence_in;
                --precedence)
            {
                for(;;)
                {
                    token = PeekCalcToken(at_ptr);
                    
                    operator_type = GetCalcBinaryOperatorTypeFromToken(token);
                    int operator_precedence = CalcOperatorPrecedence(operator_type);
                    
                    if(operator_precedence != precedence)
                    {
                        break;
                    }
                    
                    if(operator_type == CalcNodeType_Invalid)
                    {
                        break;
                    }
                    
                    NextCalcToken(at_ptr);
                    
                    CalcNode *right = ParseCalcExpression_(arena, at_ptr, precedence+1);
                    CalcNode *existing_expression = expression;
                    expression = AllocateCalcNode(arena, operator_type, at_source);
                    expression->type = operator_type;
                    expression->left = existing_expression;
                    expression->right = right;
                    
                    if(!right)
                    {
                        goto end_parse;
                    }
                }
            }
            
            end_parse:;
        }
    }
    
    
    return expression;
}

static CalcNode *
ParseCalcExpression(Arena *arena, char **at_ptr)
{
    return ParseCalcExpression_(arena, at_ptr, 1);
}

static CalcNode *
ParseCalcCode(Arena *arena, char **at_ptr)
{
    CalcNode *root = 0;
    CalcNode **target = &root;
    
    for(;;)
    {
        CalcToken token = PeekCalcToken(at_ptr);
        
        // NOTE(rjf): Parse assignment.
        if(token.type == CalcTokenType_Identifier)
        {
            char *at_source = token.string;
            
            char *at_reset = *at_ptr;
            NextCalcToken(at_ptr);
            
            // NOTE(rjf): Variable assignment
            if(RequireCalcToken(at_ptr, "="))
            {
                CalcNode *identifier = AllocateCalcNode(arena, CalcNodeType_Identifier, at_source);
                identifier->token = token;
                
                CalcNode *assignment = AllocateCalcNode(arena, CalcNodeType_Assignment, at_source);
                assignment->left = identifier;
                assignment->right = ParseCalcExpression(arena, at_ptr);
                
                if(assignment == 0)
                {
                    break;
                }
                
                if(assignment->right == 0)
                {
                    assignment = ErrorCalcNode(arena, "Syntax error.");
                    *target = assignment;
                    break;
                }
                else if(assignment->right->type == CalcNodeType_Error)
                {
                    assignment = assignment->right;
                    *target = assignment;
                    break;
                }
                
                *target = assignment;
                target = &(*target)->next;
                goto end_parse;
            }
            else
            {
                *at_ptr = at_reset;
            }
        }
        
        // NOTE(rjf): Parse expression.
        {
            CalcNode *expression = ParseCalcExpression(arena, at_ptr);
            if(expression == 0)
            {
                break;
            }
            *target = expression;
            target = &(*target)->next;
            goto end_parse;
        }
        
        end_parse:;
        
        if(!RequireCalcToken(at_ptr, ";") && !RequireNewline(at_ptr) &&
           !RequireEndOfBuffer(at_ptr))
        {
            *target = ErrorCalcNode(arena, "Expected end-of-statement (semicolon or newline).");
            target = &(*target)->next;
            break;
        }
    }
    
    return root;
}

typedef struct CalcInterpretGraph CalcInterpretGraph;
struct CalcInterpretGraph
{
    CalcInterpretGraph *next;
    CalcNode *parent_call;
    Plot2DMode mode;
    String_Const_u8 plot_title;
    String_Const_u8 x_axis;
    String_Const_u8 y_axis;
    int num_function_samples;
    Rect_f32 plot_view;
    int num_bins;
    Range_f32 bin_data_range;
    union
    {
        float *x_data;
        float *data;
    };
    float *y_data;
    int data_count;
    i32 style_flags;
};

typedef struct CalcValue CalcValue;
struct CalcValue
{
    union
    {
        struct
        {
            String_Const_u8 as_string;
        };
        
        struct
        {
            int array_count;
            CalcValue *as_array;
        };
        
        struct
        {
            String_Const_u8 as_error;
        };
        
        struct
        {
            double as_f64;
        };
        
        struct
        {
            i64 as_token_offset;
        };
    };
    
    CalcType type;
};

typedef struct CalcInterpretResult CalcInterpretResult;
struct CalcInterpretResult
{
    CalcValue value;
    CalcInterpretGraph *first_graph;
};

typedef struct CalcSymbolKey CalcSymbolKey;
struct CalcSymbolKey
{
    char *string;
    i32 string_length;
    b32 deleted;
};

typedef struct CalcSymbolValue CalcSymbolValue;
struct CalcSymbolValue
{
    CalcValue value;
};

typedef struct CalcSymbolTable CalcSymbolTable;
struct CalcSymbolTable
{
    unsigned int size;
    CalcSymbolKey *keys;
    CalcSymbolValue *values;
};

static CalcValue
CalcValueNone(void)
{
    CalcValue calc_value = {0};
    calc_value.type = CalcType_None;
    return calc_value;
}

static CalcValue
CalcValueF64(double num)
{
    CalcValue val = {0};
    val.type = CalcType_Number;
    val.as_f64 = num;
    return val;
}

static CalcValue
CalcValueError(String_Const_u8 string)
{
    CalcValue val = {0};
    val.type = CalcType_Error;
    val.as_error = string;
    return val;
}

static CalcValue
CalcValueString(String_Const_u8 string)
{
    CalcValue val = {0};
    val.type = CalcType_String;
    val.as_string = string;
    return val;
}

static CalcValue
CalcValueSourceCodeReference(i64 token_position)
{
    CalcValue val = {0};
    val.type = CalcType_SourceCodeReference;
    val.as_token_offset = token_position;
    return val;
}

typedef struct CalcInterpretContext CalcInterpretContext;
struct CalcInterpretContext
{
    Application_Links *app;
    Buffer_ID buffer;
    Text_Layout_ID text_layout_id;
    Arena *arena;
    CalcSymbolTable *symbol_table;
    f32 current_time;
    
    // NOTE(rjf): Plot data.
    struct
    {
        String_Const_u8 plot_title;
        String_Const_u8 x_axis;
        String_Const_u8 y_axis;
        Rect_f32 plot_view;
        int num_function_samples;
        int num_bins;
        f32 bin_range_low;
        f32 bin_range_high;
    };
};

// NOTE(rjf): WHY DOESN'T C++ ALLOW DECLARING THINGS IN THE ORDER I WANT THIS SUCKS SO BAD
static CalcInterpretResult
InterpretCalcExpression(CalcInterpretContext *context, CalcNode *root);

static CalcValue
CalcValueArray(CalcInterpretContext *context, CalcNode *first_member)
{
    CalcValue val = {0};
    val.type = CalcType_Array;
    
    CalcType array_type = CalcType_None;
    int count = 0;
    
    for(CalcNode *member = first_member; member; member = member->next)
    {
        ++count;
    }
    
    CalcValue *array = push_array(context->arena, CalcValue, count);
    
    int write_pos = 0;
    
    for(CalcNode *member = first_member; member; member = member->next)
    {
        CalcInterpretResult result = InterpretCalcExpression(context, member);
        
        if(member == first_member)
        {
            array_type = result.value.type;
            if(array_type == CalcType_Error)
            {
                val = result.value;
                goto end_create;
            }
            else if(array_type == CalcType_None)
            {
                val = CalcValueError(string_u8_litexpr("Cannot make arrays of 'none' type."));
                goto end_create;
            }
        }
        else
        {
            if(result.value.type != array_type)
            {
                val = CalcValueError(string_u8_litexpr("Cannot have multiple types in an array."));
                goto end_create;
            }
        }
        
        array[write_pos++] = result.value;
    }
    
    if(array && count)
    {
        val.as_array = array;
        val.array_count = count;
    }
    
    end_create:;
    return val;
}

static CalcSymbolTable
CalcSymbolTableInit(Arena *arena, unsigned int size)
{
    CalcSymbolTable table = {0};
    table.size = size;
    table.keys = push_array(arena, CalcSymbolKey, size);
    table.values = push_array(arena, CalcSymbolValue, size);
    MemorySet(table.keys, 0, sizeof(*table.keys)*size);
    MemorySet(table.values, 0, sizeof(*table.values)*size);
    return table;
}

static CalcSymbolValue *
CalcSymbolTableLookup_(CalcSymbolTable *table, char *string, int length)
{
    CalcSymbolValue *result = 0;
    
    unsigned int hash = StringCRC32(string, length) % table->size;
    unsigned int original_hash = hash;
    
    CalcSymbolValue *value = 0;
    
    for(;;)
    {
        if(table->keys[hash].string || table->keys[hash].deleted)
        {
            if(!table->keys[hash].deleted &&
               StringMatchCaseSensitive(table->keys[hash].string, table->keys[hash].string_length,
                                        string, length))
            {
                value = table->values + hash;
                break;
            }
            else
            {
                if(++hash >= table->size)
                {
                    hash = 0;
                }
                if(hash == original_hash)
                {
                    break;
                }
            }
        }
        else
        {
            break;
        }
    }
    
    if(value)
    {
        result = value;
    }
    
    return result;
}

static CalcValue
CalcSymbolTableLookup(CalcSymbolTable *table, char *string, int string_length)
{
    CalcValue result = {0};
    CalcSymbolValue *value = CalcSymbolTableLookup_(table, string, string_length);
    if(value)
    {
        result = value->value;
    }
    else
    {
        result.type = CalcType_Error;
    }
    return result;
}

static CalcSymbolValue *
CalcSymbolTableAdd(CalcSymbolTable *table, char *string, int string_length, CalcValue value)
{
    CalcSymbolValue *result = 0;
    
    unsigned int hash = StringCRC32(string, string_length) % table->size;
    unsigned int original_hash = hash;
    unsigned int found_hash = 0;
    int found = 0;
    
    for(;;)
    {
        if(table->keys[hash].string || table->keys[hash].deleted)
        {
            if(!table->keys[hash].deleted &&
               StringMatchCaseSensitive(table->keys[hash].string, table->keys[hash].string_length,
                                        string, string_length))
            {
                found = 1;
                found_hash = hash;
                break;
            }
            else if(table->keys[hash].deleted)
            {
                found = 1;
                found_hash = hash;
            }
            
            if(++hash >= table->size)
            {
                hash = 0;
            }
            if(hash == original_hash)
            {
                break;
            }
        }
        else
        {
            found = 1;
            found_hash = hash;
            break;
        }
    }
    
    if(found)
    {
        table->keys[found_hash].string = string;
        table->keys[found_hash].string_length = string_length;
        table->values[found_hash].value = value;
        result = table->values + found_hash;
    }
    
    return result;
}

static void
CalcSymbolTableRemove(CalcSymbolTable *table, char *string, int length)
{
    unsigned int hash = StringCRC32(string, length) % table->size;
    unsigned int original_hash = hash;
    
    for(;;)
    {
        if(table->keys[hash].string || table->keys[hash].deleted)
        {
            if(!table->keys[hash].deleted &&
               StringMatchCaseSensitive(table->keys[hash].string, table->keys[hash].string_length,
                                        string, length))
            {
                table->keys[hash].deleted = 1;
                break;
            }
            
            if(++hash >= table->size)
            {
                hash = 0;
            }
            if(hash == original_hash)
            {
                break;
            }
        }
        else
        {
            break;
        }
    }
}

static void
GetDataFromSourceCode(Application_Links *app, Buffer_ID buffer, Text_Layout_ID text_layout_id,
                      i64 start_pos, Arena *arena, float **data_ptr, int *data_count_ptr)
{
    Token_Array token_array = get_token_array_from_buffer(app, buffer);
    Range_i64 visible_range = text_layout_get_visible_range(app, text_layout_id);
    
    if(token_array.tokens != 0)
    {
        Token_Iterator_Array it = token_iterator_pos(0, &token_array, start_pos);
        Token *token = 0;
        
        b32 found = 0;
        
        // NOTE(rjf): Find scope open (opening brace of initializer).
        for(;;)
        {
            token = token_it_read(&it);
            if(token->pos >= start_pos + 30 || !token ||
               !token_it_inc_non_whitespace(&it))
            {
                found = 0;
                break;
            }
            
            if(token->kind == TokenBaseKind_ScopeOpen)
            {
                found = 1;
                break;
            }
        }
        
        // NOTE(rjf): Read data.
        if(found)
        {
            struct DataChunk
            {
                DataChunk *next;
                u64 value_count;
                float values[1024];
            };
            
            u64 total_value_count = 0;
            DataChunk *data_chunk = push_array_zero(arena, DataChunk, 1);
            DataChunk *first_data_chunk = data_chunk;
            DataChunk *last_data_chunk = data_chunk;
            
            b32 is_negative = 0;
            int data_count;
            float *data;
            for(;;)
            {
                token = token_it_read(&it);
                if(!token || !token_it_inc_non_whitespace(&it))
                {
                    goto end_read_data;
                }
                
                if(token->kind == TokenBaseKind_Operator &&
                   token->sub_kind == TokenCppKind_Minus)
                {
                    is_negative = 1;
                }
                
                if(token->kind == TokenBaseKind_LiteralFloat ||
                   token->kind == TokenBaseKind_LiteralInteger)
                {
                    Range_i64 token_range =
                    {
                        token->pos,
                        token->pos + (token->size > 256 ? 256 : token->size),
                    };
                    
                    u8 token_buffer[256];
                    buffer_read_range(app, buffer, token_range, token_buffer);
                    
                    float sign = is_negative ? -1.f : 1.f;
                    is_negative = 0;
                    
                    float value = sign * (float)GetFirstDoubleFromBuffer((char *)token_buffer);
                    if(last_data_chunk->value_count >= ArrayCount(last_data_chunk->values))
                    {
                        DataChunk *new_chunk = push_array_zero(arena, DataChunk, 1);
                        last_data_chunk->next = new_chunk;
                        last_data_chunk = new_chunk;
                    }
                    last_data_chunk->values[last_data_chunk->value_count++] = value;
                    total_value_count += 1;
                }
                else if(token->kind == TokenBaseKind_ScopeClose)
                {
                    break;
                }
            }
            
            data_count = 0;
            data = push_array_zero(arena, float, total_value_count);
            for(DataChunk *chunk = first_data_chunk; chunk; chunk = chunk->next)
            {
                for(int i = 0; i < ArrayCount(chunk->values); i += 1)
                {
                    data[data_count] = chunk->values[i];
                    data_count += 1;
                }
            }
            
            *data_ptr = data;
            *data_count_ptr = data_count;
            
            end_read_data:;
        }
        
    }
}

static void
GraphCalcExpression(Application_Links *app, Face_ID face_id,
                    Rect_f32 rect, CalcInterpretGraph *first_graph,
                    CalcInterpretContext *context)
{
    CalcNode *parent_call = first_graph->parent_call;
    Rect_f32 plot_view = first_graph->plot_view;
    
    int plot_count = 0;
    for(CalcInterpretGraph *graph = first_graph; graph && graph->parent_call == parent_call;
        graph = graph->next)
    {
        ++plot_count;
    }
    
    Plot2DInfo plot_data = {0};
    {
        plot_data.mode           = first_graph->mode;
        plot_data.title          = first_graph->plot_title;
        plot_data.x_axis         = first_graph->x_axis;
        plot_data.y_axis         = first_graph->y_axis;
        plot_data.screen_rect    = rect;
        plot_data.app            = app;
        plot_data.title_face_id  = global_styled_title_face;
        plot_data.label_face_id  = global_styled_label_face;
        plot_data.plot_view      = plot_view;
        plot_data.num_bins       = first_graph->num_bins;
        plot_data.bin_data_range = first_graph->bin_data_range;
        
        if(first_graph->num_bins > 0)
        {
            plot_data.bin_group_count = plot_count;
            plot_data.bins = push_array_zero(context->arena, int, plot_data.num_bins*plot_data.bin_group_count);
        }
    }
    Plot2DBegin(&plot_data);
    
    for(CalcInterpretGraph *graph = first_graph; graph && graph->parent_call == parent_call;
        graph = graph->next)
    {
        
        switch(plot_data.mode)
        {
            
            //~ NOTE(rjf): Line Graphs
            case Plot2DMode_Line:
            {
                Plot2DPoints(&plot_data, graph->style_flags, graph->x_data, graph->y_data, graph->data_count);
                break;
            }
            
            //~ NOTE(rjf): Histogram
            case Plot2DMode_Histogram:
            {
                Plot2DHistogram(&plot_data, graph->data, graph->data_count);
                break;
            }
            
            default: break;
        }
    }
    
    Plot2DEnd(&plot_data);
}

typedef struct CalcFindInputResult CalcFindInputResult;
struct CalcFindInputResult
{
    CalcNode *unknown;
    int number_unknowns;
};

static CalcFindInputResult
FindUnknownForGraph(CalcSymbolTable *table, CalcNode *expression)
{
    CalcFindInputResult result = {0};
    
    if(expression && expression->type != CalcNodeType_Invalid)
    {
        if(expression->type == CalcNodeType_Identifier)
        {
            CalcSymbolValue *symbol_value =
                CalcSymbolTableLookup_(table, expression->token.string,
                                       expression->token.string_length);
            
            if(!symbol_value)
            {
                result.unknown = expression;
                ++result.number_unknowns;
            }
        }
        else
        {
            CalcFindInputResult results[] =
            {
                FindUnknownForGraph(table, expression->left),
                FindUnknownForGraph(table, expression->right),
                FindUnknownForGraph(table, expression->first_parameter),
                FindUnknownForGraph(table, expression->next),
            };
            
            for(int i = 0; i < ArrayCount(results); ++i)
            {
                if(results[i].unknown)
                {
                    if(!result.unknown)
                    {
                        result.unknown = results[i].unknown;
                        ++result.number_unknowns;
                    }
                    else
                    {
                        if(!StringMatchCaseSensitive(results[i].unknown->token.string, results[i].unknown->token.string_length,
                                                     result.unknown->token.string, result.unknown->token.string_length))
                        {
                            ++result.number_unknowns;
                        }
                    }
                }
            }
        }
    }
    
    return result;
}

#define CALC_BUILT_IN_FUNCTION(name) CalcInterpretResult name(CalcInterpretContext *context, int param_count, CalcInterpretResult *params)
typedef CALC_BUILT_IN_FUNCTION(CalcBuiltInFunction);

static
CALC_BUILT_IN_FUNCTION(CalcSin)
{
    CalcInterpretResult result = {0};
    result.value = CalcValueF64(sin(params[0].value.as_f64));
    return result;
}

static
CALC_BUILT_IN_FUNCTION(CalcCos)
{
    CalcInterpretResult result = {0};
    result.value = CalcValueF64(cos(params[0].value.as_f64));
    return result;
}

static
CALC_BUILT_IN_FUNCTION(CalcTan)
{
    CalcInterpretResult result = {0};
    result.value = CalcValueF64(tan(params[0].value.as_f64));
    return result;
}

static
CALC_BUILT_IN_FUNCTION(CalcAbs)
{
    CalcInterpretResult result = {0};
    result.value = CalcValueF64(fabs(params[0].value.as_f64));
    return result;
}

static
CALC_BUILT_IN_FUNCTION(CalcPlotTitle)
{
    context->plot_title = params[0].value.as_string;
    context->plot_title.str += 1;
    context->plot_title.size -= 2;
    CalcInterpretResult result = {0};
    result.value = CalcValueNone();
    return result;
}

static
CALC_BUILT_IN_FUNCTION(CalcPlotFunctionSamples)
{
    context->num_function_samples = (int)params[0].value.as_f64;
    CalcInterpretResult result = {0};
    result.value = CalcValueNone();
    return result;
}

static
CALC_BUILT_IN_FUNCTION(CalcPlotBinCount)
{
    context->num_bins = (int)params[0].value.as_f64;
    CalcInterpretResult result = {0};
    result.value = CalcValueNone();
    return result;
}

static
CALC_BUILT_IN_FUNCTION(CalcPlotBinRange)
{
    context->bin_range_low = (f32)params[0].value.as_f64;
    context->bin_range_high = (f32)params[1].value.as_f64;
    CalcInterpretResult result = {0};
    result.value = CalcValueNone();
    return result;
}

static
CALC_BUILT_IN_FUNCTION(CalcTime)
{
    CalcInterpretResult result = {0};
    result.value = CalcValueF64((f64)context->current_time);
    animate_in_n_milliseconds(context->app, 0);
    return result;
}

static void
GenerateLinePlotData(CalcInterpretContext *context, CalcNode *expression,
                     CalcNode *input_variable, float **x_data, float **y_data,
                     int *data_count, i32 *style_flags_ptr)
{
    CalcInterpretResult expression_result = InterpretCalcExpression(context, expression);
    
    *x_data = 0;
    *y_data = 0;
    *data_count = 0;
    *style_flags_ptr = 0;
    
    i32 style_flags = 0;
    
    //~ NOTE(rjf): Plotting scripting arrays.
    if(expression_result.value.type == CalcType_Array)
    {
        style_flags |= Plot2DStyleFlags_Points;
        
        // NOTE(rjf): X/Y data arrays.
        if(expression_result.value.array_count == 2 &&
           expression_result.value.as_array[0].type == CalcType_Array &&
           expression_result.value.as_array[1].type == CalcType_Array &&
           expression_result.value.as_array[0].array_count > 0 &&
           expression_result.value.as_array[0].array_count ==
           expression_result.value.as_array[1].array_count &&
           expression_result.value.as_array[0].as_array[0].type ==
           expression_result.value.as_array[1].as_array[0].type &&
           expression_result.value.as_array[0].as_array[0].type == CalcType_Number)
        {
            int values_to_plot = expression_result.value.as_array[0].array_count;
            float *x_values = push_array(context->arena, float, values_to_plot);
            float *y_values = push_array(context->arena, float, values_to_plot);
            
            for(int i = 0; i < values_to_plot; ++i)
            {
                x_values[i] = (float)expression_result.value.as_array[0].as_array[i].as_f64;
                y_values[i] = (float)expression_result.value.as_array[1].as_array[i].as_f64;
            }
            
            *x_data = x_values;
            *y_data = y_values;
            *data_count = values_to_plot;
        }
        
        // NOTE(rjf): Just Y data.
        else if(expression_result.value.array_count > 0 &&
                expression_result.value.as_array[0].type == CalcType_Number)
        {
            int values_to_plot = expression_result.value.array_count;
            float *x_values = push_array(context->arena, float, values_to_plot);
            float *y_values = push_array(context->arena, float, values_to_plot);
            
            for(int i = 0; i < values_to_plot; ++i)
            {
                x_values[i] = (float)i;
                y_values[i] = (float)expression_result.value.as_array[i].as_f64;
            }
            
            *x_data = x_values;
            *y_data = y_values;
            *data_count = values_to_plot;
        }
        
    }
    
    
    //~ NOTE(rjf): Graphing data from source code.
    else if(expression_result.value.type == CalcType_SourceCodeReference)
    {
        style_flags |= Plot2DStyleFlags_Points;
        
        float *y_values = 0;
        int values_to_plot = 0;
        GetDataFromSourceCode(context->app, context->buffer, context->text_layout_id,
                              expression_result.value.as_token_offset, context->arena,
                              &y_values, &values_to_plot);
        
        // NOTE(rjf): Plot data.
        if(y_values && values_to_plot)
        {
            float *x_values = push_array(context->arena, float, values_to_plot);
            for(int i = 0; i < values_to_plot; ++i)
            {
                x_values[i] = (float)i;
            }
            *x_data = x_values;
            *y_data = y_values;
            *data_count = values_to_plot;
        }
    }
    
    
    //~ NOTE(rjf): Graphing scripting functions.
    else
    {
        style_flags |= Plot2DStyleFlags_Lines;
        
        CalcNode *input_node = input_variable;
        CalcSymbolValue *symbol_value_ptr = 0;
        if(input_node)
        {
            CalcValue value = CalcValueF64(0);
            symbol_value_ptr =
                CalcSymbolTableAdd(context->symbol_table, input_node->token.string,
                                   input_node->token.string_length, value);
        }
        
        // NOTE(rjf): Find function sample points.
        int values_to_plot = context->num_function_samples;
        float *x_values = push_array(context->arena, float, values_to_plot);
        float *y_values = push_array(context->arena, float, values_to_plot);
        {
            for(int i = 0; i < values_to_plot; ++i)
            {
                double new_x_value = (context->plot_view.x0 + (i / (float)values_to_plot) *
                                      (context->plot_view.x1 - context->plot_view.x0));
                if(symbol_value_ptr)
                {
                    symbol_value_ptr->value.as_f64 = new_x_value;
                }
                
                CalcInterpretResult result = InterpretCalcExpression(context, expression);
                if(result.value.type != CalcType_Number)
                {
                    break;
                }
                else
                {
                    x_values[i] = (float)new_x_value;
                    y_values[i] = (float)result.value.as_f64;
                }
            }
        }
        
        if(input_node)
        {
            CalcSymbolTableRemove(context->symbol_table, input_node->token.string,
                                  input_node->token.string_length);
        }
        
        *x_data = x_values;
        *y_data = y_values;
        *data_count = values_to_plot;
    }
    
    *style_flags_ptr = style_flags;
}

static void
GenerateHistogramPlotData(CalcInterpretContext *context, CalcNode *expression,
                          CalcNode *input_variable, float **data, int *data_count)
{
    CalcInterpretResult expression_result = InterpretCalcExpression(context, expression);
    
    *data = 0;
    *data_count = 0;
    
    // NOTE(rjf): Graphing scripting arrays.
    if(expression_result.value.type == CalcType_Array)
    {
        
        if(expression_result.value.array_count > 0 &&
           expression_result.value.as_array[0].type == CalcType_Number)
        {
            int values_to_plot = expression_result.value.array_count;
            float *values = push_array(context->arena, float, values_to_plot);
            
            for(int i = 0; i < values_to_plot; ++i)
            {
                values[i] = (float)expression_result.value.as_array[i].as_f64;
            }
            
            *data = values;
            *data_count = values_to_plot;
        }
        
    }
    
    // NOTE(rjf): Graphing data from source code.
    else if(expression_result.value.type == CalcType_SourceCodeReference)
    {
        float *values = 0;
        int values_to_plot = 0;
        GetDataFromSourceCode(context->app, context->buffer, context->text_layout_id,
                              expression_result.value.as_token_offset, context->arena,
                              &values, &values_to_plot);
        
        // NOTE(rjf): Plot data.
        if(values && values_to_plot)
        {
            *data = values;
            *data_count = values_to_plot;
        }
    }
    
    // NOTE(rjf): Graphing scripting functions.
    else
    {
        
    }
    
}

static CalcInterpretResult
CallCalcBuiltInFunction(CalcInterpretContext *context, CalcNode *root)
{
    
#define MAX_BUILTIN_PARAM 4
    
    CalcInterpretResult result = {0};
    
    b32 function_valid = 0;
    
    if(!root || root->type != CalcNodeType_FunctionCall)
    {
        result.value = CalcValueError(string_u8_litexpr("Internal parsing error, function call expected."));
        goto end_func_call;
    }
    
    static struct
    {
        char *name;
        CalcBuiltInFunction *proc;
        CalcType return_type;
        int required_parameter_count;
        CalcType parameter_types[MAX_BUILTIN_PARAM];
    }
    functions[] =
    {
        { "sin", CalcSin, CalcType_Number, 1, { CalcType_Number }, },
        { "cos", CalcCos, CalcType_Number, 1, { CalcType_Number }, },
        { "tan", CalcTan, CalcType_Number, 1, { CalcType_Number }, },
        { "abs", CalcAbs, CalcType_Number, 1, { CalcType_Number }, },
        
        {
            "plot_title",
            CalcPlotTitle,
            CalcType_None,
            1, { CalcType_String },
        },
        
        {
            "plot_function_samples",
            CalcPlotFunctionSamples,
            CalcType_None,
            1, { CalcType_Number },
        },
        
        {
            "plot_bin_count",
            CalcPlotBinCount,
            CalcType_None,
            1, { CalcType_Number },
        },
        
        {
            "plot_bin_range",
            CalcPlotBinRange,
            CalcType_None,
            2, { CalcType_Number, CalcType_Number },
        },
        
        { "time", CalcTime, CalcType_Number, },
        
    };
    
    for(int i = 0; i < ArrayCount(functions); ++i)
    {
        if(CalcTokenMatch(root->token, functions[i].name))
        {
            function_valid = 1;
            
            int param_count = 0;
            CalcInterpretResult param_results[MAX_BUILTIN_PARAM] = {0};
            for(CalcNode *param = root->first_parameter; param; param = param->next)
            {
                param_results[param_count++] = InterpretCalcExpression(context, param);
                if(param_count >= ArrayCount(param_results))
                {
                    break;
                }
            }
            
            int correct_call = 1;
            
            if(param_count < functions[i].required_parameter_count)
            {
                String_Const_u8 error_string = push_stringf(context->arena, "%s expects at least %i parameters.",
                                                            functions[i].name, functions[i].required_parameter_count);
                result.value = CalcValueError(error_string);
                correct_call = 0;
            }
            
            if(correct_call)
            {
                for(int j = 0; j < param_count; ++j)
                {
                    if(param_results[j].value.type != functions[i].parameter_types[j])
                    {
                        correct_call = 0;
                        String_Const_u8 error_string = push_stringf(context->arena, "'%s' expects a '%s' for parameter %i.",
                                                                    functions[i].name, CalcTypeName(functions[i].parameter_types[j]),
                                                                    j+1);
                        result.value = CalcValueError(error_string);
                        break;
                    }
                }
            }
            
            if(correct_call)
            {
                result = functions[i].proc(context, param_count, param_results);
            }
        }
        else if(i == ArrayCount(functions) - 1)
        {
            if(CalcTokenMatch(root->token, "plot_xaxis") ||
               CalcTokenMatch(root->token, "plot_yaxis"))
            {
                function_valid = 1;
                int is_y_axis = CalcTokenMatch(root->token, "plot_yaxis");
                
                result.value = CalcValueNone();
                
                CalcNode *title_param = 0;
                CalcNode *low_param = 0;
                CalcNode *high_param = 0;
                
                CalcInterpretResult title_result = {0};
                CalcInterpretResult low_result = {0};
                CalcInterpretResult high_result = {0};
                
                for(CalcNode *param = root->first_parameter;
                    param; param = param->next)
                {
                    CalcInterpretResult interpret =
                        InterpretCalcExpression(context, param);
                    
                    if(interpret.value.type == CalcType_String)
                    {
                        if(title_param)
                        {
                            result.value = CalcValueError(is_y_axis
                                                          ? string_u8_litexpr("plot_yaxis only accepts one string.")
                                                          : string_u8_litexpr("plot_xaxis only accepts one string."));
                            goto end_func_call;
                        }
                        else
                        {
                            title_param = param;
                            title_result = interpret;
                        }
                    }
                    else if(interpret.value.type == CalcType_Number)
                    {
                        if(low_param)
                        {
                            if(high_param)
                            {
                                result.value = CalcValueError(is_y_axis
                                                              ? string_u8_litexpr("plot_yaxis only accepts two numbers.")
                                                              : string_u8_litexpr("plot_xaxis only accepts two numbers."));
                                
                                goto end_func_call;
                            }
                            else
                            {
                                high_param = param;
                                high_result = interpret;
                            }
                        }
                        else
                        {
                            low_param = param;
                            low_result = interpret;
                        }
                    }
                    else
                    {
                        result = interpret;
                        break;
                    }
                }
                
                if(low_param && high_param)
                {
                    if(is_y_axis)
                    {
                        if(title_result.value.as_string.size)
                        {
                            context->y_axis = StringStripBorderCharacters(title_result.value.as_string);
                        }
                        else
                        {
                            context->y_axis = {};
                        }
                        
                        context->plot_view.y0 = (f32)low_result.value.as_f64;
                        context->plot_view.y1 = (f32)high_result.value.as_f64;
                    }
                    else
                    {
                        if(title_result.value.as_string.size)
                        {
                            context->x_axis = StringStripBorderCharacters(title_result.value.as_string);
                        }
                        else
                        {
                            context->x_axis = {};
                        }
                        
                        context->plot_view.x0 = (f32)low_result.value.as_f64;
                        context->plot_view.x1 = (f32)high_result.value.as_f64;
                    }
                }
                else
                {
                    result.value = CalcValueError(is_y_axis
                                                  ? string_u8_litexpr("plot_yaxis needs two bounds (title optional).")
                                                  : string_u8_litexpr("plot_xaxis needs two bounds (title optional)."));
                }
            }
            
            else if(CalcTokenMatch(root->token, "plot") ||
                    CalcTokenMatch(root->token, "plot_histogram"))
            {
                function_valid = 1;
                
                struct
                {
                    char *name;
                    Plot2DMode mode;
                }
                plot_functions[] =
                {
                    { "plot",           Plot2DMode_Line,       },
                    { "plot_histogram", Plot2DMode_Histogram,  },
                };
                
                Plot2DMode mode = Plot2DMode_Line;
                for(int j = 0; j < ArrayCount(plot_functions); ++j)
                {
                    if(CalcTokenMatch(root->token, plot_functions[j].name))
                    {
                        mode = plot_functions[j].mode;
                    }
                }
                
                result.value = CalcValueNone();
                
                CalcInterpretGraph **target = &result.first_graph;
                for(CalcNode *graph_expression = root->first_parameter;
                    graph_expression; graph_expression = graph_expression->next)
                {
                    CalcFindInputResult input_find = FindUnknownForGraph(context->symbol_table,
                                                                         graph_expression);
                    if(input_find.number_unknowns <= 1)
                    {
                        CalcNode *input_variable = input_find.unknown;
                        CalcInterpretGraph *new_graph = push_array_zero(context->arena, CalcInterpretGraph, 1);
                        
                        new_graph->next = 0;
                        new_graph->parent_call = root;
                        
                        new_graph->mode = mode;
                        new_graph->plot_title = context->plot_title;
                        new_graph->x_axis = context->x_axis;
                        new_graph->y_axis = context->y_axis;
                        
                        new_graph->num_function_samples = context->num_function_samples;
                        new_graph->plot_view = context->plot_view;
                        new_graph->num_bins = context->num_bins;
                        new_graph->bin_data_range.min = context->bin_range_low;
                        new_graph->bin_data_range.max = context->bin_range_high;
                        
                        // NOTE(rjf): Generate the plotting data.
                        {
                            if(mode == Plot2DMode_Line)
                            {
                                GenerateLinePlotData(context, graph_expression,
                                                     input_variable, &new_graph->x_data,
                                                     &new_graph->y_data,
                                                     &new_graph->data_count,
                                                     &new_graph->style_flags);
                            }
                            else if(mode == Plot2DMode_Histogram)
                            {
                                GenerateHistogramPlotData(context, graph_expression,
                                                          input_variable, &new_graph->data,
                                                          &new_graph->data_count);
                            }
                        }
                        
                        *target = new_graph;
                        target = &(*target)->next;
                    }
                    else
                    {
                        result.value = CalcValueError(string_u8_litexpr("Too many unknowns in graphing expression."));
                        break;
                    }
                }
            }
            
        }
    }
    
    end_func_call:;
    
    if(!function_valid)
    {
        result.value = CalcValueError(string_u8_litexpr("Unknown function."));
    }
    
    return result;
}

static CalcInterpretResult
InterpretCalcExpression(CalcInterpretContext *context, CalcNode *root)
{
    CalcInterpretResult result = {0};
    
    if(root == 0)
    {
        result.value = CalcValueError(string_u8_litexpr("Syntax error."));
    }
    else
    {
        switch(root->type)
        {
            case CalcNodeType_Error:
            {
                result.value = CalcValueError(root->error_string);
                break;
            }
            
            case CalcNodeType_Number:
            {
                result.value = CalcValueF64(root->value);
                break;
            }
            
            case CalcNodeType_Array:
            {
                result.value = CalcValueArray(context, root->first_member);
                break;
            }
            
            case CalcNodeType_ArrayIndex:
            {
                result = InterpretCalcExpression(context, root->left);
                if(result.value.type == CalcType_Array)
                {
                    CalcInterpretResult index = InterpretCalcExpression(context, root->right);
                    
                    if(index.value.type == CalcType_Number)
                    {
                        int array_index = (int)index.value.as_f64;
                        if(array_index >= 0 && array_index < result.value.array_count)
                        {
                            result.value = result.value.as_array[array_index];
                        }
                        else
                        {
                            result.value = CalcValueError(string_u8_litexpr("Array index out of bounds."));
                        }
                    }
                    else
                    {
                        result.value = CalcValueError(string_u8_litexpr("Cannot use non-numbers to index arrays."));
                        goto end_interpret;
                    }
                }
                else
                {
                    result.value = CalcValueError(string_u8_litexpr("Cannot index a non-array."));
                    goto end_interpret;
                }
                
                break;
            }
            
            case CalcNodeType_StringConstant:
            {
                result.value = CalcValueString({(u8 *)root->token.string, (u64)root->token.string_length});
                break;
            }
            
            case CalcNodeType_Add:
            case CalcNodeType_Subtract:
            case CalcNodeType_Multiply:
            case CalcNodeType_Divide:
            case CalcNodeType_Modulus:
            case CalcNodeType_RaiseToPower:
            {
                if(root->left && root->right)
                {
                    CalcInterpretResult left_result = InterpretCalcExpression(context, root->left);
                    CalcInterpretResult right_result = InterpretCalcExpression(context, root->right);
                    
                    if(left_result.value.type == CalcType_Error)
                    {
                        result = left_result;
                        goto end_interpret;
                    }
                    else if(right_result.value.type == CalcType_Error)
                    {
                        result = right_result;
                        goto end_interpret;
                    }
                    
                    else if(left_result.value.type != CalcType_Number ||
                            right_result.value.type != CalcType_Number)
                    {
                        result.value = CalcValueError(string_u8_litexpr("Cannot use non-numbers in expressions."));
                        goto end_interpret;
                    }
                    
                    switch(root->type)
                    {
                        case CalcNodeType_Add:            result.value = CalcValueF64(left_result.value.as_f64 + right_result.value.as_f64); break;
                        case CalcNodeType_Subtract:       result.value = CalcValueF64(left_result.value.as_f64 - right_result.value.as_f64); break;
                        case CalcNodeType_Multiply:       result.value = CalcValueF64(left_result.value.as_f64 * right_result.value.as_f64); break;
                        case CalcNodeType_Divide:
                        {
                            if(right_result.value.as_f64 == 0)
                            {
                                result.value = CalcValueF64(NAN);
                            }
                            else
                            {
                                result.value = CalcValueF64(left_result.value.as_f64 / right_result.value.as_f64);
                            }
                            break;
                        }
                        case CalcNodeType_Modulus:
                        {
                            if(right_result.value.as_f64 == 0)
                            {
                                result.value = CalcValueF64(NAN);
                            }
                            else
                            {
                                result.value = CalcValueF64(fmod(left_result.value.as_f64, right_result.value.as_f64));
                            }
                            break;
                        }
                        case CalcNodeType_RaiseToPower:
                        {
                            result.value = CalcValueF64(pow(left_result.value.as_f64, right_result.value.as_f64));
                            break;
                        }
                    }
                    
                }
                else
                {
                    result.value = CalcValueError(string_u8_litexpr("Binary operators require two operands."));
                }
                
                break;
            }
            
            case CalcNodeType_Negate:
            {
                result = InterpretCalcExpression(context, root->operand);
                if(result.value.type == CalcType_Number)
                {
                    result.value = CalcValueF64(-result.value.as_f64);
                }
                break;
            }
            
            case CalcNodeType_FunctionCall:
            {
                result = CallCalcBuiltInFunction(context, root);
                break;
            }
            
            case CalcNodeType_Identifier:
            {
                result.value = CalcSymbolTableLookup(context->symbol_table, root->token.string, root->token.string_length);
                if(result.value.type == CalcType_Error)
                {
                    result.value = CalcValueError(push_stringf(context->arena, "'%.*s' is not declared.", root->token.string_length, root->token.string));
                }
                
                break;
            }
            
            case CalcNodeType_SourceCodeIdentifier:
            {
                Token_Array token_array = get_token_array_from_buffer(context->app, context->buffer);
                Range_i64 visible_range = text_layout_get_visible_range(context->app, context->text_layout_id);
                i64 first_index = token_index_from_pos(&token_array, visible_range.first);
                Token_Iterator_Array it = token_iterator_index(0, &token_array, first_index);
                Token *token = 0;
                
                for(;;)
                {
                    token = token_it_read(&it);
                    
                    if(token->pos >= visible_range.one_past_last + 4096 || !token || !token_it_inc_non_whitespace(&it))
                    {
                        break;
                    }
                    
                    if(token->kind == TokenBaseKind_Identifier)
                    {
                        String_Const_u8 token_string;
                        {
                            Range_i64 token_range =
                            {
                                token->pos,
                                token->pos + (token->size > (i64)256
                                              ? (i64)256
                                              : token->size),
                            };
                            
                            u8 token_buffer[256] = {0};
                            buffer_read_range(context->app, context->buffer, token_range, token_buffer);
                            token_string = { token_buffer, (u64)(token_range.end - token_range.start) };
                        }
                        
                        if(StringMatchCaseSensitive((char *)token_string.str, (int)token_string.size,
                                                    root->token.string, root->token.string_length))
                        {
                            result.value = CalcValueSourceCodeReference(token->pos);
                            break;
                        }
                    }
                }
                
                break;
            }
            
            default: break;
        }
    }
    
    end_interpret:;
    return result;
}

static int
IdentifierExistsInCalcExpression(CalcNode *root, char *string, int string_length)
{
    int result = 0;
    
    if(root && root->type != CalcNodeType_Invalid)
    {
        if(StringMatchCaseSensitive(root->token.string, root->token.string_length, string, string_length))
        {
            result = 1;
        }
        else
        {
            result |= IdentifierExistsInCalcExpression(root->left, string, string_length);
            result |= IdentifierExistsInCalcExpression(root->right, string, string_length);
            result |= IdentifierExistsInCalcExpression(root->first_parameter, string, string_length);
            result |= IdentifierExistsInCalcExpression(root->next, string, string_length);
        }
    }
    
    return result;
}

static CalcInterpretResult
InterpretCalcCode(CalcInterpretContext *context, CalcNode *root)
{
    CalcInterpretResult result = {0};
    CalcInterpretResult last_result = result;
    
    if(root)
    {
        last_result = result;
        
        if(root->type == CalcNodeType_Error)
        {
            result.value = CalcValueError(root->error_string);
            goto end_interpret;
        }
        else if(root->type == CalcNodeType_Assignment)
        {
            if(root->left->type == CalcNodeType_Identifier)
            {
                if(!IdentifierExistsInCalcExpression(root->right, root->left->token.string, root->left->token.string_length))
                {
                    CalcInterpretResult right_result = InterpretCalcExpression(context, root->right);
                    CalcSymbolTableAdd(context->symbol_table, root->left->token.string,
                                       root->left->token.string_length, right_result.value);
                    result = InterpretCalcExpression(context, root->right);
                    result.first_graph = last_result.first_graph;
                }
                else
                {
                    result.value = CalcValueError(string_u8_litexpr("Recursive definition."));
                    result.first_graph = last_result.first_graph;
                }
            }
            else
            {
                result.value = CalcValueError(string_u8_litexpr("Assignment to non-identifier."));
                result.first_graph = last_result.first_graph;
            }
        }
        else
        {
            result = InterpretCalcExpression(context, root);
            if(last_result.first_graph)
            {
                for(CalcInterpretGraph *graph = last_result.first_graph; graph; graph = graph->next)
                {
                    if(graph->next == 0)
                    {
                        graph->next = result.first_graph;
                        break;
                    }
                }
                
                result.first_graph = last_result.first_graph;
            }
            else if(result.value.type == CalcType_Error)
            {
                goto end_interpret;
            }
        }
    }
    
    end_interpret:;
    
    return result;
}

static CalcInterpretContext
CalcInterpretContextInit(Application_Links *app, Buffer_ID buffer, Text_Layout_ID text_layout_id,
                         Arena *arena, CalcSymbolTable *symbol_table, f32 current_time)
{
    CalcInterpretContext context = {0};
    context.app = app;
    context.buffer = buffer;
    context.text_layout_id = text_layout_id;
    context.arena = arena;
    context.symbol_table = symbol_table;
    context.current_time = current_time;
    
    // NOTE(rjf): Default plot settings.
    {
        context.plot_title = string_u8_litexpr("");
        context.x_axis = string_u8_litexpr("x");
        context.y_axis = string_u8_litexpr("y");
        context.plot_view = Rf32(-1, -1, +1, +1);
        context.num_function_samples = 128;
        context.num_bins = 10;
        context.bin_range_low = -1;
        context.bin_range_high = 1;
    }
    
    return context;
}

function void
F4_CLC_RenderCode(Application_Links *app, Buffer_ID buffer,
                  View_ID view, Text_Layout_ID text_layout_id,
                  Frame_Info frame_info, Arena *arena, char *code_buffer,
                  i64 start_char_offset)
{
    ProfileScope(app, "[Fleury] Render Calc Code");
    
    f32 current_time = global_calc_time;
    CalcSymbolTable symbol_table = CalcSymbolTableInit(arena, 1024);
    
    // NOTE(rjf): Add default symbols.
    {
        // NOTE(rjf): Pi
        {
            CalcValue value = CalcValueF64(3.1415926535897);
            CalcSymbolTableAdd(&symbol_table, "pi", 2, value);
        }
        
        // NOTE(rjf): e
        {
            CalcValue value = CalcValueF64(2.71828);
            CalcSymbolTableAdd(&symbol_table, "e", 1, value);
        }
    }
    
    CalcInterpretContext context_ = CalcInterpretContextInit(app, buffer, text_layout_id, arena,
                                                             &symbol_table, current_time);
    CalcInterpretContext *context = &context_;
    
    char *at = code_buffer;
    CalcNode *expr = ParseCalcCode(arena, &at);
    
    Rect_f32 last_graph_rect = {0};
    
    for(CalcNode *interpret_expression = expr; interpret_expression;
        interpret_expression = interpret_expression->next)
    {
        char *at_source = interpret_expression->at_source;
        
        // NOTE(rjf): Find starting result layout position.
        Vec2_f32 result_layout_position = {0};
        if(at_source)
        {
            i64 offset = (i64)(at_source - code_buffer);
            for(int i = 0; at_source[i] && at_source[i] != '\n'; ++i)
            {
                ++offset;
            }
            i64 buffer_offset = start_char_offset + offset;
            Rect_f32 last_character_rect = text_layout_character_on_screen(app, text_layout_id,
                                                                           buffer_offset);
            result_layout_position.x = last_character_rect.x0;
            result_layout_position.y = last_character_rect.y0;
            result_layout_position.x += 20;
        }
        
        CalcInterpretResult result = InterpretCalcCode(context, interpret_expression);
        
        if(result_layout_position.x > 0 && result_layout_position.y > 0)
        {
            
            // NOTE(rjf): Draw result, if there's one.
            {
                String_Const_u8 result_string = {0};
                
                switch(result.value.type)
                {
                    case CalcType_Error:
                    {
                        if(expr == 0 || !result.value.as_error.size)
                        {
                            result_string = push_stringf(arena, "(error: Parse failure.)");
                        }
                        else
                        {
                            result_string = push_stringf(arena, "(error: %.*s)", string_expand(result.value.as_error));
                        }
                        break;
                    }
                    case CalcType_Number:
                    {
                        result_string = push_stringf(arena, "= %f", result.value.as_f64);
                        break;
                    }
                    case CalcType_String:
                    {
                        result_string = push_stringf(arena, "= %.*s", string_expand(result.value.as_string));
                        break;
                    }
                    default: break;
                }
                
                Vec2_f32 point = result_layout_position;
                
                u32 color = finalize_color(defcolor_comment, 0);
                color &= 0x00ffffff;
                color |= 0x80000000;
                draw_string(app, get_face_id(app, buffer), result_string, point, color);
            }
            
            // NOTE(rjf): Draw graphs.
            {
                Rect_f32 view_rect = view_get_screen_rect(app, view);
                
                Rect_f32 graph_rect = {0};
                {
                    graph_rect.x0 = view_rect.x1 - 30 - 300;
                    graph_rect.y0 = result_layout_position.y + 30 - 100;
                    graph_rect.x1 = graph_rect.x0 + 300;
                    graph_rect.y1 = graph_rect.y0 + 200;
                }
                
                CalcNode *last_parent_call = 0;
                for(CalcInterpretGraph *graph = result.first_graph; graph;
                    graph = graph->next)
                {
                    if(last_parent_call == 0 || graph->parent_call != last_parent_call)
                    {
                        if(last_graph_rect.x0 != 0 && rect_overlap(graph_rect, last_graph_rect))
                        {
                            graph_rect.y0 = last_graph_rect.y1 + 50;
                            graph_rect.y1 = graph_rect.y0 + 200;
                        }
                        
                        last_graph_rect = graph_rect;
                        
                        GraphCalcExpression(app, get_face_id(app, buffer), graph_rect, graph, context);
                        
                        // NOTE(rjf): Bump graph rect forward.
                        {
                            f32 rect_height = graph_rect.y1 - graph_rect.y0;
                            graph_rect.y0 += rect_height + 50;
                            graph_rect.y1 += rect_height + 50;
                            result_layout_position.y += rect_height + 50;
                        }
                        
                        last_parent_call = graph->parent_call;
                    }
                }
            }
        }
    }
}

function void
F4_CLC_RenderBuffer(Application_Links *app, Buffer_ID buffer, View_ID view,
                    Text_Layout_ID text_layout_id, Frame_Info frame_info)
{
    Scratch_Block scratch(app);
    Range_i64 visible_range = text_layout_get_visible_range(app, text_layout_id);
    String_Const_u8 code_string = push_whole_buffer(app, scratch, buffer);
    F4_CLC_RenderCode(app, buffer, view, text_layout_id, frame_info, scratch,
                      (char *)code_string.str, visible_range.start);
}

function void
F4_CLC_RenderComments(Application_Links *app, Buffer_ID buffer, View_ID view,
                      Text_Layout_ID text_layout_id, Frame_Info frame_info)
{
    if(def_get_config_b32(vars_save_string_lit("f4_disable_calc_comments")))
    {
        return;
    }
    
    ProfileScope(app, "[Fleury] Calc Comments");
    
    Scratch_Block scratch(app);
    Token_Array token_array = get_token_array_from_buffer(app, buffer);
    Range_i64 visible_range = text_layout_get_visible_range(app, text_layout_id);
    
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
                Range_i64 token_range =
                {
                    token->pos,
                    token->pos + (token->size > 1024
                                  ? 1024
                                  : token->size),
                };
                
                u32 token_buffer_size = (u32)(token_range.end - token_range.start);
                if(token_buffer_size < 4)
                {
                    token_buffer_size = 4;
                }
                u8 *token_buffer = push_array(scratch, u8, token_buffer_size+1);
                buffer_read_range(app, buffer, token_range, token_buffer);
                token_buffer[token_buffer_size] = 0;
                
                if((token_buffer[0] == '/' && token_buffer[1] == '/' && token_buffer[2] == 'c' &&
                    character_is_whitespace(token_buffer[3])) ||
                   (token_buffer[0] == '/' && token_buffer[1] == '*' && token_buffer[2] == 'c'))
                {
                    int is_multiline_comment = (token_buffer[1] == '*');
                    if(is_multiline_comment)
                    {
                        if(token_buffer[token_buffer_size-1] == '/' &&
                           token_buffer[token_buffer_size-2] == '*')
                        {
                            token_buffer[token_buffer_size-2] = 0;
                        }
                    }
                    char *at = (char *)token_buffer + 3;
                    F4_CLC_RenderCode(app, buffer, view, text_layout_id, frame_info, scratch, at, token_range.start + 3);
                }
            }
        }
    }
}
