#include "ExawindSolver.h"
#include "MemoryUsage.h"

namespace exawind {

long ExawindSolver::mem_usage()
{
    const long mem = memory_usage();

    ParallelPrinter printer(comm());

    // gather all memory usage
    long minmem = -1, totmem = -1, maxmem = -1;
    MPI_Reduce(&mem, &minmem, 1, MPI_LONG, MPI_MIN, printer.io_rank(), comm());
    MPI_Reduce(&mem, &totmem, 1, MPI_LONG, MPI_SUM, printer.io_rank(), comm());
    MPI_Reduce(&mem, &maxmem, 1, MPI_LONG, MPI_MAX, printer.io_rank(), comm());

    int psize;
    MPI_Comm_size(comm(), &psize);
    const std::string out =
        identifier() + " memory --" + " min: " + std::to_string(minmem) +
        " avg: " + std::to_string((long)((double)totmem / (double)psize)) +
        " max: " + std::to_string(maxmem) + " total: " + std::to_string(totmem);
    printer.echo(out);

    return mem;
}

} // namespace exawind
