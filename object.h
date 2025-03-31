#ifndef OBJECT_H
#define OBJECT_H

    // ============================ Expected External

    int file_append (char *filename, char *buffer, int max);

    // ============================ Structures & Definitions

    typedef struct {
        char magic[3];          // 3
        char type;              // 4
        uint32_t standard;      // 8
        uint32_t entry;         // 12
        uint32_t symbols;       // 16
        uint32_t readable;      // 20
        uint32_t writable;      // 24
        uint32_t executable;    // 28
        char padding[4];        // 32
    } object_header_t;

    typedef struct {
        int size; char *content;
    } buffer_t;

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
    enum { static_o = 0, relocatable_o = 'R' };
    enum {
        move_i = 0x10, immediate_i, load8_i, load16_i, load32_i, load64_i,
        store8_i, store16_i, store32_i, store64_i,
        add_i = 0x30, sub_i, mul_i, div_i, left_i, right_i, and_i, or_i, xor_i,
        flip_i, inc_i, dec_i, fadd_i, fsub_i, fmul_i, fdiv_i,
        fmov_i, fcast_i, imov_i, icast_i,
        push_i = 0x50, pop_i,
        gotor_i = 0x60, goto_i, if_i, not_i, lt_i, le_i, gt_i, ge_i,
        lts_i, les_i, gts_i, ges_i, cmp_i, fcmp_i, call_i, ret_i,
        interrupt_i = 0x80, syscall_i,
        sysopen_i, sysclose_i, sysread_i, syswrite_i,
        sysseek_i, sysbrk_i, sysfork_i, sysexec_i, sysexit_i
    };

    #define symbol_max          67108864 / sizeof(hotsymbol_t) // 64MB
    #define standard_max        1

    // ============================ Function Signatures

    buffer_t *section_push          (char section, char *data, int size);
    bool instruction_push           (char section, char opcode, char arg0, char arg1, char arg2);
    bool section_write              (char section, char *filename);

    uint32_t __symbol_hash          (char *name); // does not touch files
    uint32_t symbol_set             (char *name, char section, uint64_t value); // overwrites
    uint32_t symbol_touch           (char *name, char section, uint64_t value); // does not
    bool symbol_defined             (char *name);
    bool symbol_write               (char *filename);
    bool header_write               (char *filename);

    object_header_t *header_read    (buffer_t *content); // These do some validation, then return void * +offset within content
    symbol_t *symboltable_read      (buffer_t *content); // These do some validation, then return void * +offset within content

    // ============================ Shared References

        hotsymbol_t symbol_table     [symbol_max] = { 0 };
        buffer_t section_table       [count_s] = { 0 };
        uint32_t symbol_count        = 0;

    // ============================ Implementations

    buffer_t *section_push     (char section, char *data, int size) {
        if ( section >= count_s || section <= 0 ) { return NULL; }
        buffer_t *self = &section_table[section-1];
        self->content = realloc(self->content, self->size + size);
        char *dest = (self->content) + self->size;
        memcpy(dest, data, size); self->size += size;
        return self;
    }

    bool instruction_push   (char section, char opcode, char arg0, char arg1, char arg2) {
        uint32_t instruction = *(uint32_t*) (char[]){ opcode, arg0, arg1, arg2 };
        buffer_t *check = section_push(section, (char*)&instruction, sizeof(instruction));
        if ( check == NULL ) { return false; } return true;
    }
    
    bool section_write          (char section, char *filename) {
        if ( section >= count_s || section <= 0 ) { return NULL; }
        buffer_t *self = &section_table[section-1];
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

    uint32_t symbol_touch      (char *name, char section, uint64_t value) {
        uint32_t hash = __symbol_hash(name);
        hotsymbol_t *self = &symbol_table[hash];
        if ( self->section == 0 ) {
        self->section = section; self->value = value;
        self->index = ++symbol_count; }
        return hash;
    }

    uint32_t symbol_set        (char *name, char section, uint64_t value) {
        uint32_t hash = __symbol_hash(name);
        hotsymbol_t *self = &symbol_table[hash];
        if ( self->section == 0 ) { return symbol_touch(name, section, value); }
        self->section = section; self->value = value; // don't mod value if defined
        return hash;
    }

    bool symbol_write           (char *filename) {
        // remember if any extern
        bool external = false;
        // count the symbols
        int count; int i; for (i=0;i<symbol_max;i++) {
            hotsymbol_t *ref = &symbol_table[i]; 
            if ( ref->section == 0 ) { continue; }
            count++;
        }
        // then stack allocate room to *sort* them into
        symbol_t sort[count];
        for(i=0;i<count;i++) {
            hotsymbol_t *ref = &symbol_table[i];
            if ( ref->section != 0 ) { 
                int index = ref->index;
                sort[index] = (symbol_t) {
                    ref->section,
                    ref->value
                };  strncpy(sort[index].name, ref->name, 59);
            }
        }
        // once flattened, we can append it out
        int w = file_append (filename, (char*)&sort, sizeof(symbol_t)*count);
        if ( w <= 0 ) { return false; } return true;
    }

    bool header_write           (char *filename) {
        uint32_t entry = __symbol_hash("entry");
        hotsymbol_t *self = &symbol_table[entry];
        if ( self->section != '0' ) { entry = self->index; } else { entry = -1; }
        object_header_t yield = {
            .magic = "GYB",
            .type = 0,
            .standard = standard_max,
            .entry = entry,
            .symbols = symbol_count,
            .readable = section_table[read_s].size,
            .writable = section_table[write_s].size,
            .executable = section_table[exec_s].size
        };
        int w = file_append(filename, (char*)&yield, sizeof(object_header_t));
        if ( w <= 0 ) { return false; } return true;
    }

    object_header_t *header_read (buffer_t *content) {
        if ( content->size <= sizeof(object_header_t) )     { fprintf(stderr, "File too short to be a valid object\n"); return NULL; }
        object_header_t *self = (object_header_t *)(content);
        if ( strncmp(self->magic, "GYB", 3)!=0 )            { fprintf(stderr, "File header contains invalid bytemagic\n"); return NULL; }
        if (self->type != 0 && 
            self->type != 'R' )                             { fprintf(stderr, "File header contains invalid bytemagic subcode\n"); return NULL; }
        if (self->standard == 0 || 
            self->standard > standard_max )                 { fprintf(stderr, "File header contains invalid or unsupported standard\n"); return NULL; }
        int expected = sizeof(object_header_t) + \
            self->symbols * sizeof(symbol_t) + \
            self->readable + self->writable + self->executable;
        if ( content->size !=  expected )                   { fprintf(stderr, "File size was expected to be %d, actually %d\n", expected, content->size); return NULL; }
        return self;
    }

    symbol_t *symboltable_read  (buffer_t *content) {
        object_header_t *header = (object_header_t *)(content);
        symbol_t *self = (symbol_t *)(content + sizeof(object_header_t));
        int count = header->symbols;
        // validate the symbols
        int i; for (i=0;i<count;i++) {
            symbol_t ref = self[i];
            switch(ref.section) {
                default:            { fprintf(stderr, "Invalid section data for symbol #%d\n", i); return NULL; }
                case read_s:        if ( ref.value >= header->readable )    { fprintf(stderr, "Reference out of bounds for symbol #%d\n", i); return NULL; } break;
                case write_s:       if ( ref.value >= header->writable )    { fprintf(stderr, "Reference out of bounds for symbol #%d\n", i); return NULL; } break;
                case exec_s:        if ( ref.value >= header->executable )  { fprintf(stderr, "Reference out of bounds for symbol #%d\n", i); return NULL; } break;
                case ext_s:         if ( header->type != 'R' )              { fprintf(stderr, "Unresolved external symbol #%d in object marked as static\n", i); return NULL; } break;
                case const_s:       break;
            }
        }
        return self;
    }

#endif