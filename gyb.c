#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <sys/stat.h>

#include "object.h"         // provides definitions, handles emission
#include "linker.h"         // validates and squashes objects
#include "backend.h"        // hooks in emitters for different platforms
#include "file.h"           // dependency for file i/o
#include "run.h"            // bytecode machine

#include "tmp/help.h"

const char version_info[] = "gyb v 0.0.0, (C) 2025 Modula.dev\n<https://modula.dev>\n";

int main (int argc, char **argv) {
    if ( argc == 1 ) { printf(version_info); printf(help_message); exit(1); }
    char *references[16]; 
    bool run = false; bool link = false;   
}