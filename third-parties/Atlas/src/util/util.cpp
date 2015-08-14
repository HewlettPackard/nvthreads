#include <execinfo.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

#include "util.h"

const char *NVM_GetLogDir()
{
#ifdef PMM_OS
    return "/dev/pmmfs";
#else
    return "/dev/shm/regions";
#endif
}

void NVM_CreateLogDir()
{
#ifdef PMM_OS
    system("mkdir -p /dev/pmmfs");
#else
    system("mkdir -p /dev/shm/regions");
#endif
}

#ifdef PMM_OS
char *NVM_GetFullyQualifiedRegionName(const char * name)
{
    assert(strlen("/dev/pmmfs/")+strlen(name) < 2*MAXLEN+1);
    char *s = (char*) malloc(2*MAXLEN*sizeof(char));
    sprintf(s, "/dev/pmmfs/%s", name);
    return s;
}
#else
char *NVM_GetFullyQualifiedRegionName(const char * name)
{
    assert(strlen("/dev/shm/regions/")+strlen(name) < 2*MAXLEN+1);
    char *s = (char*) malloc(2*MAXLEN*sizeof(char));
    sprintf(s, "/dev/shm/regions/%s", name);
    return s;
}
#endif

char *NVM_GetLogRegionName()
{
    extern const char *__progname;
    assert(strlen("logs_")+strlen(__progname) <= MAXLEN);
    char *s = (char*) malloc(MAXLEN*sizeof(char));
    sprintf(s, "logs_%s", __progname);
    return s;
}

char *NVM_GetLogRegionName(const char * name)
{
    assert(strlen("logs_")+strlen(name) <= MAXLEN);
    char *s = (char*) malloc(MAXLEN*sizeof(char));
    sprintf(s, "logs_%s", name);
    return s;
}

bool NVM_doesLogExist(const char *log_path_name)
{
    assert(log_path_name);
    struct stat buf;
    int status = stat(log_path_name, &buf);
    return status == 0;
}

void NVM_qualifyPathName(char *s, const char *name)
{
#ifdef PMM_OS
    sprintf(s, "/dev/pmmfs/%s", name);
#else
    sprintf(s, "/dev/shm/regions/%s", name);
#endif
}

// The following assumes interference-freedom, i.e. a lock must be held
void NVM_CowAddToMapInterval(MapInterval * volatile *ptr,
                             uint64_t e1, uint64_t e2, uint32_t e3)
{
    assert(ptr);
    // since only writer, no need to use ALAR below
    if (!*ptr) ASRW(*ptr, new MapInterval);
    MapInterval *new_map = new MapInterval;
    const MapInterval & old_map = **ptr;
    MapInterval::const_iterator ci_end = old_map.end();
    for (MapInterval::const_iterator ci = old_map.begin(); ci!=ci_end; ++ci)
        InsertToMapInterval(
            new_map, ci->first.first, ci->first.second, ci->second);
    InsertToMapInterval(new_map, e1, e2, e3);
    ASRW(*ptr, new_map);
}

// The following assumes interference-freedom, i.e. a lock must be held
void NVM_CowDeleteFromMapInterval(MapInterval * volatile *ptr,
                                  uint64_t e1, uint64_t e2, uint32_t e3)
{
    assert(ptr);
    assert(*ptr);
    MapInterval *new_map = new MapInterval;
    const MapInterval & old_map = **ptr;
    MapInterval::const_iterator ci_end = old_map.end();
    for (MapInterval::const_iterator ci = old_map.begin(); ci!=ci_end; ++ci)
    {
        if (ci->first.first != e1)
            InsertToMapInterval(
                new_map, ci->first.first, ci->first.second, ci->second);
        else
        {
            assert(ci->first.second == e2);
            assert(ci->second == e3);
        }
    }
    ASRW(*ptr, new_map);
}

//template <class T> uint32_t SimpleHashTable<T>::Size_ = 1024;
template<> uint32_t SimpleHashTable<SetOfInts>::Size_ = 1024;

// Obtained from
// http://www.gnu.org/software/libc/manual/html_node/Backtraces.html
void PrintBackTrace()
{
    void *array[10];
    size_t size;
    char **strings;
    size_t i;
     
    size = backtrace(array, 10);
    strings = backtrace_symbols(array, size);
     
    printf ("Obtained %zd stack frames.\n", size);
     
    for (i = 0; i < size; i++)
        printf ("%s\n", strings[i]);
     
    free (strings);
}

void AtlasTrace(FILE *fp, const char *fmt, ...)
{
    va_list ap;
    const char *p;
    int ival;
    long lval;
    char *s;
    void *v;
    char c;

    va_start(ap, fmt);
    
    for (p = fmt; *p; ++p)
    {
        if (*p != '%')
        {
            fputc(*p, fp);
            continue;
        }
        ++p;
        if (!*p) break;
        
        switch(*p)
        {
            case 'd':
                ival = va_arg(ap, int);
                fprintf(fp, "%d", ival);
                break;
            case 'l':
                ++p;
                if (!*p || (*p != 'd')) assert(0);
                lval = va_arg(ap, long);
                fprintf(fp, "%ld", lval);
                break;
            case 'c':
                c = va_arg(ap, int);
                fprintf(fp, "%c", c);
                break;
            case 's':
                s = va_arg(ap, char*);
                fprintf(fp, "%s", s);
                break;
            case 'p':
                v = va_arg(ap, void*);
                fprintf(fp, "%p", v);
                break;
            default:
                assert(0);
                break;
        }
    }
    va_end(ap);
}

#if defined(_NVM_TRACE) || defined(_NVM_VERBOSE_TRACE)
void InitializeTLFile(FILE **p)
{
    assert(!*p);
    char buf[256];
    sprintf(buf, "log_%ld.txt", pthread_self());
    *p = fopen(buf, "w");
    assert(*p);
}
#endif
