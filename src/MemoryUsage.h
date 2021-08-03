#ifndef MEMORYUSAGE_H
#define MEMORYUSAGE_H

#ifdef __linux__
#include <sys/resource.h>
#endif

namespace exawind {

//! Get memory usage in MB
long memory_usage();

} // namespace exawind
#endif /* MEMORYUSAGE_H */
