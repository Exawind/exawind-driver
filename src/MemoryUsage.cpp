#include "MemoryUsage.h"

namespace exawind {

#ifdef __linux__
long memory_usage()
{
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);

    // convert to MB
    return (long)((double)usage.ru_maxrss) / 1024.0;
}
#else
long memory_usage() { return -1; }
#endif

} // namespace exawind
