#ifndef BYTEMACHINE_H
#define BYTEMACHINE_H

    // ============================ Expected External

    // DEPENDS UPON OBJECT_H for:
        // enum registers
        // enum instructions
        // section definition
        // object i/o
        // run.asm functions

    extern uint64_t __run_interrupt(char number);
    extern uint64_t __run_syscall(uint64_t number, uint64_t arg0, uint64_t arg1, uint64_t arg2);

    // ============================ Definitions

    enum { zero_f, carry_f, signed_f };

    typedef struct {
        uint64_t registers[9];
        bool flags[3];
        object_header_t header;
        buffer_t readable;
        buffer_t writable;
        buffer_t executable;
        buffer_t stack;
        symbol_t *symbols;
    } runtime_t;

    #define quotient_register   ar
    #define remainder_register  dr
    #define vaddr_writable      0x10000
    #define vaddr_readable      0xF0000000
    #define vaddr_executable    0xF8000000
    #define vaddr_stacktop      0xFFFFFFF0

    // ============================ Function Signatures

    runtime_t run_byteload(char *filename);
    void run_bytecode(runtime_t *runtime);      // errcode or 0
    bool run_bytestep(runtime_t *runtime);      // errcode or 0
    void *run_deref(runtime_t *runtime, uint64_t address); // converts virt address to address of read memory
    uint64_t run_ref(char section, uint32_t offset);

        // Most of these will be defined through macro expansion

        bool run_add        (runtime_t *runtime, uint32_t instruction);            // Arithmetic Instructions
        bool run_sub        (runtime_t *runtime, uint32_t instruction);
        bool run_mul        (runtime_t *runtime, uint32_t instruction);
        bool run_div        (runtime_t *runtime, uint32_t instruction);        
        bool run_left       (runtime_t *runtime, uint32_t instruction);
        bool run_right      (runtime_t *runtime, uint32_t instruction);
        bool run_and        (runtime_t *runtime, uint32_t instruction);
        bool run_or         (runtime_t *runtime, uint32_t instruction);
        bool run_xor        (runtime_t *runtime, uint32_t instruction);
        bool run_flip       (runtime_t *runtime, uint32_t instruction);
        bool run_inc        (runtime_t *runtime, uint32_t instruction);
        bool run_dec        (runtime_t *runtime, uint32_t instruction);
        bool run_fadd       (runtime_t *runtime, uint32_t instruction);
        bool run_fsub       (runtime_t *runtime, uint32_t instruction);
        bool run_fmul       (runtime_t *runtime, uint32_t instruction);
        bool run_fdiv       (runtime_t *runtime, uint32_t instruction);
        bool run_compare    (runtime_t *runtime, uint32_t instruction);
        bool run_fcompare   (runtime_t *runtime, uint32_t instruction);
        bool run_fmov       (runtime_t *runtime, uint32_t instruction);             // Move Instructions
        bool run_imov       (runtime_t *runtime, uint32_t instruction);
        bool run_fcast      (runtime_t *runtime, uint32_t instruction);
        bool run_icast      (runtime_t *runtime, uint32_t instruction);
        bool run_move       (runtime_t *runtime, uint32_t instruction);
        bool run_immediate  (runtime_t *runtime, uint32_t instruction);
        bool run_load       (runtime_t *runtime, uint32_t instruction);
        bool run_store      (runtime_t *runtime, uint32_t instruction);
        bool run_gotor      (runtime_t *runtime, uint32_t instruction);             // Branch Instructions
        bool run_goto       (runtime_t *runtime, uint32_t instruction);
        bool run_if         (runtime_t *runtime, uint32_t instruction);
        bool run_not        (runtime_t *runtime, uint32_t instruction);
        bool run_lt         (runtime_t *runtime, uint32_t instruction);
        bool run_le         (runtime_t *runtime, uint32_t instruction);
        bool run_gt         (runtime_t *runtime, uint32_t instruction);
        bool run_ge         (runtime_t *runtime, uint32_t instruction);
        bool run_lts        (runtime_t *runtime, uint32_t instruction);
        bool run_les        (runtime_t *runtime, uint32_t instruction);
        bool run_gts        (runtime_t *runtime, uint32_t instruction);
        bool run_ges        (runtime_t *runtime, uint32_t instruction);
        bool run_interrupt  (runtime_t *runtime, uint32_t instruction);             // System Call Instructions
        bool run_syscall    (runtime_t *runtime, uint32_t instruction);
        bool run_open       (runtime_t *runtime, uint32_t instruction);
        bool run_close      (runtime_t *runtime, uint32_t instruction);
        bool run_read       (runtime_t *runtime, uint32_t instruction);
        bool run_write      (runtime_t *runtime, uint32_t instruction);
        bool run_seek       (runtime_t *runtime, uint32_t instruction);
        bool run_brk        (runtime_t *runtime, uint32_t instruction);
        bool run_fork       (runtime_t *runtime, uint32_t instruction);
        bool run_exec       (runtime_t *runtime, uint32_t instruction);
        bool run_exit       (runtime_t *runtime, uint32_t instruction);
        bool run_push       (runtime_t *runtime, uint32_t instruction);             // Stack Instructions
        bool run_pop        (runtime_t *runtime, uint32_t instruction);
        
    // ============================ Function Definitions

    runtime_t run_byteload(char *filename) {
        runtime_t self;
        // Load the file
        int size = file_size(filename); if ( size <= 0 ) { fprintf(stderr, "loader error, unable to sys/stat file \"%s\"\n", filename); exit(1); }
        char filecontent[size+1]; filecontent[size] = '\0';
        int r = file_read(filename, filecontent, size); if ( r <= 0 ) { fprintf(stderr, "loader error, unable to read file \"%s\"\n", filename); exit(1); }
        buffer_t content = { .size = size, .content = filecontent };
        
        // Get it processed
        object_header_t *header = header_read(&content); if ( header == NULL ) { fprintf(stderr, "loader error, error parsing file header\n", filename); exit(1); }
        symbol_t *table = symboltable_read(&content); if ( table == NULL ) { fprintf(stderr, "loader error, error parsing symbol table\n", filename); exit(1); }
        
        // Init our memory
        self.readable.size =        header->readable;
        self.writable.size =        header->writable;
        self.executable.size =      header->executable;
        self.stack.size =           0xFFFF;
        int headskip = sizeof(object_header_t) + sizeof(symbol_t) * header->symbols;
        self.readable.content =     malloc(header->readable); memcpy(self.readable.content, (char*)(filecontent + headskip), header->readable); headskip += header->readable;
        self.writable.content =     malloc(header->readable); memcpy(self.writable.content, (char*)(filecontent + headskip), header->writable); headskip += header->readable;
        self.executable.content =   malloc(header->readable); memcpy(self.executable.content, (char*)(filecontent + headskip), header->executable);
        self.stack.content =        malloc(0xFFFF);

        // TODO: initalize argv onto the stack
        // Init registers for the ip, sp, bp
        self.registers[ip] = vaddr_writable + header->entry;
        self.registers[sp] = vaddr_stacktop;
        self.registers[bp] = vaddr_stacktop;
        return self;
    }

    void run_bytecode(runtime_t *runtime) {
        while(true) {
            if ( run_bytestep(runtime) == false ) { exit( runtime->registers[ar] ); }
        }
    }

    bool run_bytestep(runtime_t *runtime) {
        uint64_t address = runtime->registers[ip];
        if (address < vaddr_executable || address >= ( vaddr_executable + runtime->header.executable ) ||
            address >= vaddr_stacktop ) { fprintf(stderr, "runtime error [%lx], instruction pointer out-of-bounds\n"); return false; }
        uint32_t instruction =  *(uint32_t*)(run_deref(runtime, address));
        uint8_t  opcode =       *(uint8_t*)(&instruction);
        switch(opcode) {
            case move_i:        return run_move         (runtime, instruction); break;
            case immediate_i:   return run_immediate    (runtime, instruction); break;
            case load8_i:       
            case load16_i:
            case load32_i:
            case load64_i:      return run_load         (runtime, instruction); break;
            case store8_i:       
            case store16_i:
            case store32_i:
            case store64_i:     return run_store        (runtime, instruction); break;
            case add_i:         return run_add          (runtime, instruction); break;
            case sub_i:         return run_sub          (runtime, instruction); break;
            case mul_i:         return run_mul          (runtime, instruction); break;
            case div_i:         return run_div          (runtime, instruction); break;
            case left_i:        return run_left         (runtime, instruction); break;
            case right_i:       return run_right        (runtime, instruction); break;
            case and_i:         return run_and          (runtime, instruction); break;
            case or_i:          return run_or           (runtime, instruction); break;
            case xor_i:         return run_xor          (runtime, instruction); break;
            case flip_i:        return run_flip         (runtime, instruction); break;
            case inc_i:         return run_inc          (runtime, instruction); break;
            case dec_i:         return run_dec          (runtime, instruction); break;
            case fmov_i:        return run_fmov         (runtime, instruction); break;
            case fcast_i:       return run_fcast        (runtime, instruction); break;
            case imov_i:        return run_imov         (runtime, instruction); break;
            case icast_i:       return run_icast        (runtime, instruction); break;
            case push_i:        return run_push         (runtime, instruction); break;
            case pop_i:         return run_pop          (runtime, instruction); break;
            case gotor_i:       return run_gotor        (runtime, instruction); break;
            case goto_i:        return run_goto         (runtime, instruction); break;
            case if_i:          return run_if           (runtime, instruction); break;
            case not_i:         return run_not          (runtime, instruction); break;
            case lt_i:          return run_lt           (runtime, instruction); break;
            case le_i:          return run_le           (runtime, instruction); break;
            case gt_i:          return run_gt           (runtime, instruction); break;
            case ge_i:          return run_ge           (runtime, instruction); break;
            case lts_i:         return run_lts          (runtime, instruction); break;
            case les_i:         return run_les          (runtime, instruction); break;
            case gts_i:         return run_gts          (runtime, instruction); break;
            case ges_i:         return run_ges          (runtime, instruction); break;
            case cmp_i:         return run_compare      (runtime, instruction); break;
            case fcmp_i:        return run_fcompare     (runtime, instruction); break;
            case interrupt_i:   return run_interrupt    (runtime, instruction); break;
            case syscall_i:     return run_syscall      (runtime, instruction); break;
            case sysopen_i:     return run_open         (runtime, instruction); break;
            case sysclose_i:    return run_close        (runtime, instruction); break;
            case sysread_i:     return run_read         (runtime, instruction); break;
            case syswrite_i:    return run_write        (runtime, instruction); break;
            case sysseek_i:     return run_seek         (runtime, instruction); break;
            case sysbrk_i:      return run_brk          (runtime, instruction); break;
            case sysfork_i:     return run_fork         (runtime, instruction); break;
            case sysexec_i:     return run_exec         (runtime, instruction); break;
            case sysexit_i:     return run_exit         (runtime, instruction); break;
        }
    }

    void *run_deref(runtime_t *runtime, uint64_t address) {
        if     (address >= vaddr_writable && 
                address < (vaddr_writable + runtime->header.writable) )  
                { return (void*)(runtime->writable.content + address - vaddr_writable); }
        else if(address >= vaddr_readable && 
                address < (vaddr_readable + runtime->header.readable) )
                { return (void*)(runtime->readable.content + address - vaddr_readable); }
        else if(address >= vaddr_executable && 
                address < (vaddr_executable + runtime->header.executable) )
                { return (void*)(runtime->executable.content + address - vaddr_executable); }
        else if(address <= vaddr_stacktop && 
                address >= runtime->registers[sp] )
                { return (void*)(runtime->stack.content + address - vaddr_stacktop + 0xFFFF); }
        return NULL;   
    }

    uint64_t run_ref(char section, uint32_t offset) {
        switch(section) {
            case read_s:    return vaddr_readable   + offset;
            case write_s:   return vaddr_writable   + offset;
            case exec_s:    return vaddr_executable + offset;
        }
    }

    #define run_binop(operation) \
    (runtime_t *runtime, uint32_t instruction) { \
        char *iparts =  (char*)(&instruction); \
        if ( iparts[1] > bp || iparts[2] > bp ) { fprintf(stderr, "runtime error [%lx], invalid register pair\n", runtime->registers[ip]); return false; } \
        uint64_t *base =    (uint64_t*)(&runtime->registers[1]); \
        uint64_t *adder =   (uint64_t*)(&runtime->registers[2]); \
        *base = *base operation *adder; \
        return true; \
    }

    #define run_floatop(operation) \
    (runtime_t *runtime, uint32_t instruction) { \
        char *iparts =  (char*)(&instruction); \
        if (iparts[1] < x0 || iparts[2] < x0 | \
            iparts[1] > x1 || iparts[2] > x1 ) { fprintf(stderr, "runtime error [%lx], invalid register pair\n", runtime->registers[ip]); return false; } \
        double *base =    (double*)(&runtime->registers[1]); \
        double *adder =   (double*)(&runtime->registers[2]); \
        *base = *base operation *adder; \
        return true; \
    }

    #define run_unop(operation) \
    (runtime_t *runtime, uint32_t instruction) { \
        char *iparts =  (char*)(&instruction); \
        if ( iparts[1] >= bp ) { fprintf(stderr, "runtime error [%lx], invalid register\n", runtime->registers[ip]); return false; } \
        uint64_t *base =    (uint64_t*)(&runtime->registers[1]); \
        *base = operation *base; \
        return true; \
    }

    // Stack Instructions -- Manually Defined 

    bool run_push       (runtime_t *runtime, uint32_t instruction) {
        char *iparts =  (char*)(&instruction);
        char basr =     iparts[1]; if ( basr > ip ) { fprintf(stderr, "runtime error [%lx], invalid register\n", runtime->registers[ip]); return false; }
        uint64_t *sv = &runtime->registers[sp];
        if ( *sv <= (vaddr_stacktop - 0xFFFF) ) { fprintf(stderr, "runtime error [%lx], stack overflow error\n", runtime->registers[ip]); return false; }
        runtime->stack.content[*sv] = runtime->registers[basr]; *sv-=sizeof(uint32_t);
        return true;
    }

    bool run_pop        (runtime_t *runtime, uint32_t instruction) {
        char *iparts =  (char*)(&instruction);
        char basr =     iparts[1]; if ( basr > ip ) { fprintf(stderr, "runtime error [%lx], invalid register\n", runtime->registers[ip]); return false; }
        uint64_t *sv = &runtime->registers[sp];
        if ( *sv >= (vaddr_stacktop) ) { fprintf(stderr, "runtime error [%lx], stack underflow error\n", runtime->registers[ip]); return false; }
        *sv+=sizeof(uint32_t); runtime->stack.content[*sv] = runtime->registers[basr];
        return true;
    }

    // Arithmetic -- Macro Defined

    bool run_add        run_binop(+);
    bool run_sub        run_binop(-);
    bool run_mul        run_binop(*);
    bool run_left       run_binop(<<);
    bool run_right      run_binop(>>);
    bool run_and        run_binop(&);
    bool run_or         run_binop(|);
    bool run_xor        run_binop(^);
    bool run_flip       run_unop(~);
    bool run_inc        run_unop(++);
    bool run_dec        run_unop(--);
    bool run_fadd       run_floatop(+);
    bool run_fsub       run_floatop(-);
    bool run_fmul       run_floatop(/);

    // Arithmetic -- Manually defined

    bool run_div        (runtime_t *runtime, uint32_t instruction)  {
        char *iparts =              (char*)&instruction;
        char basreg =               iparts[1];
        char addreg =               iparts[2];
        if ( basreg > bp || addreg > bp ) { fprintf(stderr, "runtime error [%lx], invalid register pair\n", runtime->registers[ip]); return false; }
        uint64_t *base =            (uint64_t*)(&runtime->registers[basreg]);
        uint64_t *adder =           (uint64_t*)(&runtime->registers[addreg]);
        runtime->registers[quotient_register] =  *base / *adder;
        runtime->registers[remainder_register] = *base % *adder;
        return true;
    }

    bool run_fdiv       (runtime_t *runtime, uint32_t instruction)  {
        char *iparts =              (char*)&instruction;
        char basreg =               iparts[1];
        char addreg =               iparts[2];
        if ( basreg > bp || addreg > bp ) { fprintf(stderr, "runtime error [%lx], invalid register pair\n", runtime->registers[ip]); return false; }
        double *base =            (double*)(&runtime->registers[basreg]);
        double *adder =           (double*)(&runtime->registers[addreg]);
        runtime->registers[quotient_register] =  *base / *adder;
        return true;
    }

    bool run_compare    (runtime_t *runtime, uint32_t instruction)  {
        char *iparts =              (char*)&instruction;
        char basreg =               iparts[1];
        char addreg =               iparts[2];
        if ( basreg > bp || addreg > bp ) { fprintf(stderr, "runtime error [%lx], invalid register pair\n", runtime->registers[ip]); return false; }
        uint64_t *base =            (uint64_t*)(&runtime->registers[basreg]);
        uint64_t *adder =           (uint64_t*)(&runtime->registers[addreg]);
        runtime->flags[zero_f] =    ( *base == *adder );
        runtime->flags[carry_f] =   ( *base > *adder );
        runtime->flags[signed_f] =  ( *(int64_t*)base > *(int64_t*)adder );
        return true;
    }

    bool run_fcompare   (runtime_t *runtime, uint32_t instruction)  {
        char *iparts =              (char*)&instruction;
        char basreg =               iparts[1];
        char addreg =               iparts[2];
        if ( basreg < x0 || addreg < x1 ) { fprintf(stderr, "runtime error [%lx], invalid register pair\n", runtime->registers[ip]); return false; }
        double *base =              (double*)(&runtime->registers[basreg]);
        double *adder =             (double*)(&runtime->registers[addreg]);
        runtime->flags[zero_f] =    ( *base == *adder );
        runtime->flags[carry_f] =   ( *base > *adder );
        runtime->flags[signed_f] =  runtime->flags[carry_f];
        return true;
    }


    // Move Instructions -- Manually Defined
    
    bool run_move       (runtime_t *runtime, uint32_t instruction)  {
        char *iparts =              (char*)&instruction;
        char basreg =               iparts[1];
        char addreg =               iparts[2];
        if ( basreg > bp || addreg > bp ) { fprintf(stderr, "runtime error [%lx], invalid register pair\n", runtime->registers[ip]); return false; }
        uint64_t *base =            (uint64_t*)(&runtime->registers[basreg]);
        *base = runtime->registers[addreg];
        return true;
    }

    bool run_immediate  (runtime_t *runtime, uint32_t instruction) {
        char *iparts =              (char*)&instruction;
        char basreg =               iparts[1];
        uint16_t reference =        *(uint16_t*)(&iparts[2]);
        if ( basreg > bp )          { fprintf(stderr, "runtime error [%lx], invalid register\n", runtime->registers[ip]); return false; }
        if ( reference >= runtime->header.symbols ) { fprintf(stderr, "runtime error [%lx], invalid symbol index\n", runtime->registers[ip]); return false; }
        uint64_t *base =            (uint64_t*)(&runtime->registers[basreg]);
        symbol_t ref =              runtime->symbols[reference]; // skip typechecking, symbol table checked during loading
        switch(ref.section)         {
            case read_s:            
            case write_s:           
            case exec_s:            *base = run_ref(read_s, ref.value); break;
            case const_s:           *base = ref.value; break;
        }
        return true;
    }

    bool run_load   (runtime_t *runtime, uint32_t instruction) {
        char *iparts =              (char*)&instruction;
        char width =                iparts[0];
        char basreg =               iparts[1];
        uint16_t reference =        *(uint16_t*)(&iparts[2]);
        if ( basreg > bp ) { fprintf(stderr, "runtime error [%lx], invalid register\n", runtime->registers[ip]); return false; }
        if ( reference >= runtime->header.symbols ) { fprintf(stderr, "runtime error [%lx], invalid symbol index\n", runtime->registers[ip]); return false; }
        // translate symbol to address
        uint64_t vaddr;
        symbol_t self = runtime->symbols[reference];
        switch(self.section) {
            default: fprintf(stderr, "runtime error [%lx], invalid symbol for load instruction %lx\n", runtime->registers[ip]); return false; break;
            case read_s:    
            case write_s:   
            case exec_s:    vaddr = run_ref(self.section, self.value); break;
        }   if ( vaddr == -1 ) { fprintf(stderr, "runtime error [%lx], address is invalid or out-of-bounds\n", runtime->registers[ip]); return false; }
        // dereference and move the value
        uint64_t *base =            (uint64_t*)(&runtime->registers[basreg]);
        void *source;
        switch(width) {
            default: fprintf(stderr, "runtime error [%lx], invalid load instruction width %lx\n", runtime->registers[ip]); return false;
            case load8_i:   *base = *(uint8_t*)(source);  break;
            case load16_i:  *base = *(uint16_t*)(source); break;
            case load32_i:  *base = *(uint32_t*)(source); break;
            case load64_i:  *base = *(uint64_t*)(source); break;
        }
        return true;
    }

    bool run_store  (runtime_t *runtime, uint32_t instruction) {
        char *iparts =              (char*)&instruction;
        char width =                iparts[0];
        char basreg =               iparts[1];
        uint16_t reference =        *(uint16_t*)(&iparts[2]);
        if ( basreg > bp ) { fprintf(stderr, "runtime error [%lx], invalid register\n", runtime->registers[ip]); return false; }
        if ( reference >= runtime->header.symbols ) { fprintf(stderr, "runtime error [%lx], invalid symbol index\n", runtime->registers[ip]); return false; }
        // translate symbol to address
        uint64_t vaddr;
        symbol_t self = runtime->symbols[reference];
        switch(self.section) {
            default: fprintf(stderr, "runtime error [%lx], invalid symbol for store instruction %lx\n", runtime->registers[ip]); return false; break;
            case read_s:    
            case write_s:   
            case exec_s:    vaddr = run_ref(self.section, self.value); break;
        }   if ( vaddr == -1 ) { fprintf(stderr, "runtime error [%lx], address is invalid or out-of-bounds\n", runtime->registers[ip]); return false; }
        // dereference and move the value
        uint64_t *base =            (uint64_t*)(&runtime->registers[basreg]);
        void *source;
        switch(width) {
            default: fprintf(stderr, "runtime error [%lx], invalid store instruction width %lx\n", runtime->registers[ip]); return false;
            case store8_i:   *(uint8_t*)(source) = *base;  break;
            case store16_i:  *(uint16_t*)(source) = *base; break;
            case store32_i:  *(uint32_t*)(source) = *base; break;
            case store64_i:  *(uint64_t*)(source) = *base; break;
        }
        return true;
    }

    bool run_fmov   (runtime_t *runtime, uint32_t instruction) {
        char *iparts =              (char*)&instruction;
        char basreg =               iparts[1];
        char addreg =               iparts[2];
        if (basreg > bp ||
            addreg < x0 || addreg > x1 ) { fprintf(stderr, "runtime error [%lx], invalid register pair\n", runtime->registers[ip]); return false; }
        uint64_t *base =            &runtime->registers[basreg];
        uint64_t *adder =           &runtime->registers[addreg];
        *base = *adder;
        return true;
    }

    bool run_imov   (runtime_t *runtime, uint32_t instruction) {
        char *iparts =              (char*)&instruction;
        char basreg =               iparts[1];
        char addreg =               iparts[2];
        if (addreg > bp ||
            basreg < x0 || basreg > x1) { fprintf(stderr, "runtime error [%lx], invalid register pair\n", runtime->registers[ip]); return false; }
        uint64_t *base =            &runtime->registers[basreg];
        uint64_t *adder =           &runtime->registers[addreg];
        *base = *adder;
        return true;
    }

    bool run_fcast  (runtime_t *runtime, uint32_t instruction) {
        char *iparts =              (char*)&instruction;
        char basreg =               iparts[1];
        char addreg =               iparts[2];
        if (basreg > bp ||
            addreg < x0 || addreg > x1 ) { fprintf(stderr, "runtime error [%lx], invalid register pair\n", runtime->registers[ip]); return false; }
        double *base =            (double*)&runtime->registers[basreg];
        double adder =           (double)(runtime->registers[addreg]);
        *base = adder;
        return true;
    }

    bool run_icast  (runtime_t *runtime, uint32_t instruction) {
        char *iparts =              (char*)&instruction;
        char basreg =               iparts[1];
        char addreg =               iparts[2];
        if (addreg > bp ||
            basreg < x0 || basreg > x1) { fprintf(stderr, "runtime error [%lx], invalid register pair\n", runtime->registers[ip]); return false; }
        uint64_t *base =            &runtime->registers[basreg];
        uint64_t adder =            (runtime->registers[addreg]);
        *base = adder;
        return true;
    }

    // Branch Instructions -- Manually Defined

    bool run_gotor      (runtime_t *runtime, uint32_t instruction) {
        char *iparts =              (char*)&instruction;
        char basreg =               iparts[1];
        if ( basreg > dr )          { fprintf(stderr, "runtime error [%lx], invalid register\n", runtime->registers[ip]); return false; }
        uint64_t *base =            &runtime->registers[basreg];
        runtime->registers[ip] = *base;
        return true;
    }

    bool run_goto       (runtime_t *runtime, uint32_t instruction) {
        char *iparts =              (char*)&instruction;
        uint16_t reference =        *(uint16_t*)(iparts + 1);
        if ( reference >= runtime->header.symbols ) { fprintf(stderr, "runtime error [%lx], invalid symbol index\n", runtime->registers[ip]); return false; }
        symbol_t self = runtime->symbols[reference];
        uint64_t vaddr = self.value; if ( !run_deref(runtime, vaddr) ) { fprintf(stderr, "runtime error [%lx], invalid or out-of-bounds address %lx\n", runtime->registers[ip], vaddr); return false; }
        runtime->registers[ip] = vaddr;
        return true;
    }

    bool run_if         (runtime_t *runtime, uint32_t instruction) {  if ( runtime->flags[zero_f] == true ) { return run_goto(runtime, instruction); } return true; }

    bool run_not        (runtime_t *runtime, uint32_t instruction) {  if ( runtime->flags[zero_f] == false ) { return run_goto(runtime, instruction); } return true; }

    bool run_lt         (runtime_t *runtime, uint32_t instruction) {  if ( runtime->flags[zero_f] == false && runtime->flags[carry_f] == false ) { return run_goto(runtime, instruction); } return true; }

    bool run_le         (runtime_t *runtime, uint32_t instruction) {  if ( runtime->flags[carry_f] == false ) { return run_goto(runtime, instruction); } return true; }

    bool run_gt         (runtime_t *runtime, uint32_t instruction) {  if ( runtime->flags[zero_f] == false && runtime->flags[carry_f] == true ) { return run_goto(runtime, instruction); } return true; }

    bool run_ge         (runtime_t *runtime, uint32_t instruction) {  if ( runtime->flags[carry_f] == true ) { return run_goto(runtime, instruction); } return true; }

    bool run_lts        (runtime_t *runtime, uint32_t instruction) {  if ( runtime->flags[zero_f] == false && runtime->flags[signed_f] == false ) { return run_goto(runtime, instruction); } return true; }

    bool run_les        (runtime_t *runtime, uint32_t instruction) {  if ( runtime->flags[signed_f] == false ) { return run_goto(runtime, instruction); } return true; }

    bool run_gts        (runtime_t *runtime, uint32_t instruction) {  if ( runtime->flags[zero_f] == false && runtime->flags[signed_f] == true ) { return run_goto(runtime, instruction); } return true; }

    bool run_ges        (runtime_t *runtime, uint32_t instruction) {  if ( runtime->flags[signed_f] == true ) { return run_goto(runtime, instruction); } return true; }

    // Syscall Instructions -- Manually Defined
    
    bool run_interrupt  (runtime_t *runtime, uint32_t instruction) {
        fprintf(stderr, "runtime error [%lx], not implementing non-syscall system interrupts\n", runtime->registers[ip]); return false;
    }

    #define run_syscall_generic(call) \
    (runtime_t *runtime, uint32_t instruction) { \
            uint64_t result = __run_syscall( \
            call, \
            runtime->registers[br], \
            runtime->registers[cr], \
            runtime->registers[dr] \
        ); \
        runtime->registers[ar] = result; \
        return true; \
    }

    // https://x64.syscall.sh/
    bool run_syscall    run_syscall_generic (runtime->registers[ar]);
    bool run_open       run_syscall_generic (2);
    bool run_close      run_syscall_generic (3);
    bool run_read       run_syscall_generic (0);
    bool run_write      run_syscall_generic (1);
    bool run_seek       run_syscall_generic (8);
    bool run_fork       run_syscall_generic (57);
    bool run_exec       run_syscall_generic (59);
    bool run_exit       run_syscall_generic (60);

    bool run_brk        (runtime_t *runtime, uint32_t instruction) {
        char *iparts =  (char*)&instruction;
        uint64_t vbrk = runtime->registers[ar];
        if ( vbrk < vaddr_writable || vbrk >= vaddr_readable ) { fprintf(stderr, "runtime error [%lx], invalid address for breakpoint\n", runtime->registers[ip]); return false; }
        runtime->writable.size = vbrk - vaddr_writable;
        runtime->writable.content = realloc(runtime->writable.content, runtime->writable.size);
        if ( !runtime->writable.content ) { fprintf(stderr, "runtime error [%lx], failed to reallocate memory\n", runtime->registers[ip]); return false; }
        return true;
    }
    
#endif