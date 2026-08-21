#include <stdbool.h>
#include <stdio.h>

#include "../include/shared.h"

void LogError(const char* message)  {
    fprintf(stderr, "[VKK][ERROR]: %s\n", message);
}

void LogWarn(const char* message)  {

    if (logWarnings) {
        fprintf(stderr, "[VKK][WARNING]: %s\n", message);
    }

}