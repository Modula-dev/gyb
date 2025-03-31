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

int main (int argc, char **argv) {

}