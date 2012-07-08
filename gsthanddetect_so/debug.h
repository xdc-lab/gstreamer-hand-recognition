#include <execinfo.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

void dump(int signo)
{
    void *array[30];
    size_t size;
    char **strings;
    size_t i;

    size = backtrace (array, 30);
    strings = backtrace_symbols (array, size);

    fprintf (stderr,"Obtained %zd stack frames.nm\n", size);

    for (i = 0; i < size; i++)
        fprintf (stderr,"%s\n", strings[i]);

    free (strings);

    exit(0);
}

Debug_Printf_FrameInfos()
{
    signal(SIGSEGV, &dump);
}
