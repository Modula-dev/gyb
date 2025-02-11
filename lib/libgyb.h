#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/stat.h>
#define symboltype_undefined    '\0'    // self-explanatory
#define symboltype_constant     '\1'    // replaced where is, not a ptr
#define symboltype_external     '@'     // expect definition elsewhere
#define symboltype_readable     'R'     // ptr to somewhere in readable section
#define symboltype_writeable    'W'     // ptr to somewhere in writable section
#define symboltype_executable   'E'     // ptr to somewhere in executable section
#define gyb_bytecode_buffer_default 1024
#define section_readonly 'R'
#define section_readwrite 'W'
#define section_executable 'E'
#define gyb_symbol_depth 1024
#define gyb_symbol_namelen 64 - (sizeof(unsigned char)*2) - sizeof(uint32_t) - 4
typedef struct {
    unsigned char bytemagic[4]; // GYB0 or GYO0
    uint32_t entry;
    uint32_t readonly[2];   // offset, size
    uint32_t writeable[2];  // offset, size
    uint32_t executable[2]; // offset, size
    uint32_t symbols[2];    // offset, size
} gyb_header_t;

typedef struct {
    char name[gyb_symbol_namelen];
    uint32_t offset;
    unsigned char section;
    unsigned char defined;
} gyb_symbol_t;

typedef struct {
    gyb_symbol_t entries[gyb_symbol_depth];
} gyb_symboltable_t;

typedef struct {
    gyb_header_t header;
    char *readonly; int rbuffersize;
    char *writeable; int wbuffersize;
    char *executable; int ebuffersize;
    gyb_symboltable_t symtable;
} gybfile_t;
// bytecode population functions

bool gyb_bp_move  (gybfile_t *object, register_t base, register_t source);
bool gyb_bp_load  (gybfile_t *object, register_t base, gyb_symbol_t source);
bool gyb_bp_store (gybfile_t *object, register_t base, gyb_symbol_t source);
bool gyb_bp_imm   (gybfile_t *object, register_t base, gyb_symbol_t source);
bool gyb_bp_cmp   (gybfile_t *object, register_t base, register_t adder);
bool gyb_bp_add   (gybfile_t *object, register_t base, register_t adder);
bool gyb_bp_sub   (gybfile_t *object, register_t base, register_t adder);
bool gyb_bp_mul   (gybfile_t *object, register_t base, register_t adder);
bool gyb_bp_div   (gybfile_t *object, register_t base, register_t adder);
bool gyb_bp_left  (gybfile_t *object, register_t base, register_t adder);
bool gyb_bp_right (gybfile_t *object, register_t base, register_t adder);
bool gyb_bp_and   (gybfile_t *object, register_t base, register_t adder);
bool gyb_bp_or    (gybfile_t *object, register_t base, register_t adder);
bool gyb_bp_xor   (gybfile_t *object, register_t base, register_t adder);
bool gyb_bp_flip  (gybfile_t *object, register_t base);
bool gyb_bp_fadd  (gybfile_t *object, register_t base, register_t adder);
bool gyb_bp_fsub  (gybfile_t *object, register_t base, register_t adder);
bool gyb_bp_fmul  (gybfile_t *object, register_t base, register_t adder);
bool gyb_bp_fdiv  (gybfile_t *object, register_t base, register_t adder);
bool gyb_bp_goto  (gybfile_t *object, gyb_symbol_t symbol);
bool gyb_bp_if    (gybfile_t *object, gyb_symbol_t symbol);
bool gyb_bp_else  (gybfile_t *object, gyb_symbol_t symbol);
bool gyb_bp_lt    (gybfile_t *object, gyb_symbol_t symbol);
bool gyb_bp_gt    (gybfile_t *object, gyb_symbol_t symbol);
bool gyb_bp_le    (gybfile_t *object, gyb_symbol_t symbol);
bool gyb_bp_ge    (gybfile_t *object, gyb_symbol_t symbol);
bool gyb_bp_fmov  (gybfile_t *object, register_t base);
bool gyb_bp_fcast (gybfile_t *object, register_t base);
bool gyb_bp_imov  (gybfile_t *object, register_t base);
bool gyb_bp_icast (gybfile_t *object, register_t base);
bool gyb_bp_push  (gybfile_t *object, register_t base);
bool gyb_bp_pop   (gybfile_t *object, register_t base);
bool gyb_bp_sys1  (gybfile_t *object, register_t arg0);
bool gyb_bp_sys2  (gybfile_t *object, register_t arg0, register_t arg1);
bool gyb_bp_sys3  (gybfile_t *object, register_t arg0, register_t arg1, register_t arg2);
bool gyb_bp_sys4  (gybfile_t *object, register_t arg0, register_t arg1, register_t arg2, register_t arg3);

// Bytecode magic numbers

#define gybh_sys_class      0x8
#define gybh_branch_class   0xB
#define gybh_move_class     0x9
#define gybh_math_class     0xA
#define gybh_cast_class     0xC

#define gybh_move           gybh_move_class + 0
#define gybh_load           gybh_move_class + 1
#define gybh_store          gybh_move_class + 2
#define gybh_imm            gybh_move_class + 3
#define gybh_push           gybh_move_class + 4
#define gybh_pop            gybh_move_class + 5

#define gybh_cmp            gybh_math_class + 0
#define gybh_add            gybh_math_class + 1
#define gybh_sub            gybh_math_class + 2
#define gybh_mul            gybh_math_class + 3
#define gybh_div            gybh_math_class + 4
#define gybh_left           gybh_math_class + 5
#define gybh_right          gybh_math_class + 6
#define gybh_and            gybh_math_class + 7
#define gybh_or             gybh_math_class + 8
#define gybh_xor            gybh_math_class + 9
#define gybh_flip           gybh_math_class + 10
#define gybh_fadd           gybh_math_class + 11
#define gybh_fsub           gybh_math_class + 12
#define gybh_fmul           gybh_math_class + 13
#define gybh_fdiv           gybh_math_class + 14

#define gybh_fmov           gybh_cast_class + 0
#define gybh_fcast          gybh_cast_class + 1
#define gybh_imov           gybh_cast_class + 2
#define gybh_icast          gybh_cast_class + 3

#define gybh_goto           gybh_branch_class + 0
#define gybh_if             gybh_branch_class + 1
#define gybh_else           gybh_branch_class + 2
#define gybh_lt             gybh_branch_class + 3
#define gybh_gt             gybh_branch_class + 4
#define gybh_le             gybh_branch_class + 5
#define gybh_ge             gybh_branch_class + 6

#define gybh_sys1           gybh_sys_class + 0
#define gybh_sys2           gybh_sys_class + 1
#define gybh_sys3           gybh_sys_class + 2
#define gybh_sys4           gybh_sys_class + 3

#define gybh_register_ar    0
#define gybh_register_br    1
#define gybh_register_cr    2
#define gybh_register_dr    3
#define gybh_register_sp    4
#define gybh_register_bp    5
unsigned char gyb_bytecode_magic(char *buffer);
gybfile_t gyb_bytecode_new();
gybfile_t gyb_bytecode_load(char *filename);
int gyb_bytecode_save(char *filename, gybfile_t source);
uint32_t symbolhash(char *name);
gyb_symboltable_t *gyb_symtable_load(gyb_symboltable_t *table, char *source, int size);
gyb_symbol_t *gyb_symtable_flatten(gyb_symboltable_t *table);
uint32_t symboltable_count(gyb_symboltable_t *table);
bool gyb_symbol_new(gyb_symboltable_t *table, char *name, unsigned char section, uint32_t offset);
bool gyb_symbol_static(gyb_symbol_t *symbols, int count);
void gyb_symtable_import(gyb_symboltable_t *parent, gyb_symboltable_t *source);
void gyb_symtable_print(gyb_symboltable_t *table);
int file_read(char *filename, char *buffer, int max);
int file_write (char *filename, char *buffer, int max);
bool file_exists (char *filename);
int file_size(char *filename);
void file_route(char *path);
void file_delete(char *path);
const char badmissing[] = "could not find file \"%s\"\n";
const char badtype[] = "file \"%s\" is wrong type\n";
const char badentrysection[] = "error: entry symbol points to invalid section\n";
const char badentryoob[] = "error: entry symbol points outside of executable\n";
const char entrywarning[] = "warning: unable to find entry symbol,\ndefaulting to beginning of executable section\n";void gyb_debug_printheader(gyb_header_t *source);