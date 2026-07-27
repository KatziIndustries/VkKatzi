#include <stdbool.h>
#include <stdio.h>

void Log(const char* message, bool error)  {
    if (error) {
        fprintf(stderr, "[VKK][ERROR]: %s\n", message);
    } else {
        fprintf(stdout, "[VKK][INFO]: %s\n", message);
    }
}