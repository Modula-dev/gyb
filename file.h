int file_read(char *filename, char *buffer, int max) {
    FILE *fp = fopen(filename, "rb");
    if ( fp == NULL ) { return false; }
    int cursor = 0; char c; while(cursor < max -1) {
      c = fgetc(fp);
      if( feof(fp) ) { break; }
      buffer[cursor] = c; cursor++;
   } fclose(fp); return cursor;
}

int file_write (char *filename, char *buffer, int max) {
    FILE *fp = fopen(filename, "w+");
    if ( fp == NULL ) { return false; }
    int cursor; for (cursor = 0; cursor < max; cursor++) {
        if (fputc(buffer[cursor], fp) == EOF) { fclose(fp); return false; }
    } fclose(fp); return cursor;
}

int file_append (char *filename, char *buffer, int max) {
    FILE *fp = fopen(filename, "a+");
    if (fp == NULL) { return false; }
    int cursor; for (cursor = 0; cursor < max; cursor++) {
        if (fputc(buffer[cursor], fp) == EOF) { fclose(fp); return false; }
    } fclose(fp); return cursor;
}

/* REQUIRES: <sys/stat.h> */
bool    file_exists (char *filename) { struct stat buffer; return (stat(filename, &buffer) == 0); }
int     file_size(char *filename) { struct stat buffer; return (stat(filename, &buffer) == 0) ? buffer.st_size : -1; }

void file_route(char *path) {
    #ifdef _WIN_32
        int len = strlen(path); int i;
        for(i=0;i<len;i++) { if (path[i]=='/') { path[i]='\\'; } }
    #endif
}

void file_delete(char *path) {
    char command_path[1024]; file_route(path);
    #ifdef _WIN_32
        sprintf(command_path, "del %s", path);
    #else
        sprintf(command_path, "rm %s", path);
    #endif
    system(command_path);
}