#ifndef OBJECT_H
#define OBJECT_H

    // ============================ Expected External

    int file_append (char *filename, char *buffer, int max);

    // ============================ Structures & Definitions

    typedef struct {
        char magic[4];          // 4
        uint32_t standard;      // 8
        uint32_t entry;         // 12
        uint32_t symbols;       // 16
        uint32_t readable;      // 20
        uint32_t writable;      // 24
        uint32_t executable;    // 28
        char padding[6];        // 32
    } object_header_t;

    typedef struct {
        int size; char *content;
    } section_t;

    typedef struct {
        char section; uint32_t index; char name[59];
        uint64_t value; // size doesn't matter
    } hotsymbol_t;

    typedef struct {
        char section; char name[63];
        uint64_t value; // 128 bytes
    } symbol_t;

    enum { read_s = 1, write_s, exec_s, const_s, ext_s, count_s }; // section enums
    enum { ar = 0xa, br, cr, dr, sp, bp, ip, x0, x1 }; // register enums
    enum {
        add_i = 0x30, sub_i, mul_i, div_i, left_i, right_i, and_i, or_i, xor_i,
        flip_i, inc_i, dec_i, fadd_i, fsub_i, fmul_i, fdiv_i,
        fmov_i, fcast_i, imov_i, icast_i,
        push_i = 0x50, pop_i,
        gotor_i = 0x60, goto_i, if_i, not_i, lt_i, le_i, gt_i, ge_i,
        lts_i, les_i, gts_i, ges_i, cmp_i, fcmp_i, call_i, ret_i,
        interrupt_i = 0x80, syscall_i
    };

    #define symbol_max          67108864 / sizeof(hotsymbol_t) // 64MB

    // ============================ Function Signatures

    section_t *section_push     (char section, char *data, int size);
    bool instruction_push       (char section, char opcode, char arg0, char arg1, char arg2);
    bool section_write          (char section, char *filename);

    uint32_t __symbol_hash      (char *name); // does not touch files
    hotsymbol_t *symbol_set     (char *name, char section, uint64_t value); // overwrites
    hotsymbol_t *symbol_touch   (char *name, char section, uint64_t value); // does not
    bool symbol_defined         (char *name);
    bool symbol_write           (char *filename);
    bool header_write           (char *filename);

    // ============================ Shared References

        hotsymbol_t symbol_table        [symbol_max] = { 0 };
        section_t   section_table       [count_s] = { 0 };
        uint32_t    symbol_count        = 0;

    // ============================ Implementations

    section_t *section_push     (char section, char *data, int size) {
        if ( section >= count_s || section <= 0 ) { return NULL; }
        section_t *self = &section_table[section-1];
        self->content = realloc(self->content, self->size + size);
        char *dest = (self->content) + self->size;
        memcpy(dest, data, size); self->size += size;
        return self;
    }

    bool instruction_push   (char section, char opcode, char arg0, char arg1, char arg2) {
        uint32_t instruction = *(uint32_t*) (char[]){ opcode, arg0, arg1, arg2 };
        section_t *check = section_push(section, (char*)&instruction, sizeof(instruction));
        if ( check == NULL ) { return false; } return true;
    }
    
    bool section_write          (char section, char *filename) {
        if ( section >= count_s || section <= 0 ) { return NULL; }
        section_t *self = &section_table[section-1];
        char *buffer = self->content;
        int max = self->size;
        int w = file_append (filename, buffer, max);
        if ( w <= 0 ) { return false; } return true;
    }

    uint32_t __symbol_hash      (char *name) {
        uint32_t hash;
        char p3 = 0; char p5 = 0; char p7 = 0;
        int i; for(i=0;i<63;i++) {
            if ( name[i] == '\0' ) { break; }
            p3 += (name[i])%3; p5 += (name[i])%5; p7 += (name[i])%7;
        }   hash = *(uint32_t*)(char[]){ p3, p5, p7, (char)i };
        return hash % symbol_max;
    }

    bool symbol_defined         (char *name) {
        uint32_t hash = __symbol_hash(name);
        hotsymbol_t *self = &symbol_table[hash];
        if ( self->section == 0 ) { return false; } return true;
    }

    hotsymbol_t *symbol_touch      (char *name, char section, uint64_t value) {
        uint32_t hash = __symbol_hash(name);
        hotsymbol_t *self = &symbol_table[hash];
        if ( self->section == 0 ) {
        self->section = section; self->value = value;
        self->index = ++symbol_count; }
        return self;
    }

    hotsymbol_t *symbol_set        (char *name, char section, uint64_t value) {
        uint32_t hash = __symbol_hash(name);
        hotsymbol_t *self = &symbol_table[hash];
        if ( self->section == 0 ) { return symbol_touch(name, section, value); }
        self->section = section; self->value = value; // don't mod value if defined
        return self;
    }

    bool symbol_write           (char *filename) {
        // count the symbols
        int count; int i; for (i=0;i<symbol_max;i++) {
            hotsymbol_t *ref = &symbol_table[i]; 
            if ( ref->section != 0 ) { count++; }
        }
        // stack allocate room to clone them into
        hotsymbol_t flatten[count];
        int out=0; for(i=0;out<symbol_max;i++) {
            hotsymbol_t *ref = &symbol_table[i];
            if ( ref->section != 0 ) { memcpy( &flatten[out], (char*)&symbol_table[i], sizeof(hotsymbol_t) ); out++;}
        }

        // then stack allocate room to *sort* them into
        // TODO: replace with better algorithm. this impl is O(n**2)
        symbol_t sort[count];
        for(out=0;out<count;out++) {
        for(i=0;i<count;i++) {
            if ( flatten[i].index == out ) {
                strncpy(sort[out].name, flatten[i].name, 59);
                sort[out].section = flatten[i].section;
                sort[out].value = flatten[i].value;
                continue;
            }
        }}

        // once flattened, we can append it out
        int w = file_append (filename, (char*)&sort, sizeof(symbol_t)*count);
        if ( w <= 0 ) { return false; } return true;
    }

    bool header_write           (char *filename) {
        uint32_t entry = __symbol_hash("entry");
        hotsymbol_t *self = &symbol_table[entry];
        if ( self->section != '0' ) { entry = self->index; } else { entry = -1; }
        object_header_t yield = {
            .magic = "GYB\0",
            .standard = 0,
            .entry = entry,
            .symbols = symbol_count,
            .readable = section_table[read_s].size,
            .writable = section_table[write_s].size,
            .executable = section_table[exec_s].size
        };
        int w = file_append(filename, (char*)&yield, sizeof(object_header_t));
        if ( w <= 0 ) { return false; } return true;
    }

#endif