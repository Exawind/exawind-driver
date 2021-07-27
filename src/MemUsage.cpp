#include "OversetSimulation.h"
#include "ExawindSolver.h"
#include <fstream>

#ifdef __linux__
#include <sys/resource.h>

namespace exawind {

long OversetSimulation::mem_usage_all(const int step)
{
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    // convert to MB
    const long mem = (long) ((double) usage.ru_maxrss)/1024.0;

    int psize, prank;
    MPI_Comm_size(m_comm, &psize);
    MPI_Comm_rank(m_comm, &prank);

    if(psize < 1) return -1;

    // gather all memory usage to proc 0
    std::vector<long> memall(psize);
    MPI_Gather(&mem,1,MPI_LONG,memall.data(),1,MPI_LONG,0,m_comm);
   
    // FIXME: once we can distinguish between different instances 
    // of same solver we will move to separate output files 
    // and put in ExawindSolver
    if (prank == 0) {
        std::string filename = "memusage.dat";
        std::ofstream fp;

        if(step == m_last_timestep + 1){ 
            fp.open(filename.c_str(), std::ios_base::out);
            fp << "# time step, memory usage in MBs" << std::endl;
        } else {
            fp.open(filename.c_str(), std::ios_base::app);
        }
        
        fp << std::to_string(step);
        for(int i = 0; i < psize; ++i) {
            fp << ' ' <<  memall[i];
        }
        fp << std::endl;
        fp.close();
    }
    return mem;
}

long ExawindSolver::mem_usage()
{
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
        
    // convert to MB
    const long mem = (long) ((double) usage.ru_maxrss)/1024.0;

    int myrank, psize, minrank;
    MPI_Comm_rank(comm(), &myrank);
    MPI_Comm_size(comm(), &psize);

    MPI_Allreduce(&myrank, &minrank, 1, MPI_INT, MPI_MIN, comm());
        
    // gather all memory usage to proc 0
    long minmem = -1;
    long totmem = -1;
    long maxmem = -1;
    MPI_Reduce(&mem, &minmem, 1, MPI_LONG, MPI_MIN, minrank, comm());
    MPI_Reduce(&mem, &totmem, 1, MPI_LONG, MPI_SUM, minrank, comm());
    MPI_Reduce(&mem, &maxmem, 1, MPI_LONG, MPI_MAX, minrank, comm());

    ParallelPrinter printer(comm(), minrank);
    const std::string out =
        identifier() + " Memory Usage" 
         + " min: " + std::to_string(minmem) 
         + " avg: " + std::to_string((long) ((double) totmem/(double) psize)) 
         + " max: " + std::to_string(maxmem)
         + " total: " + std::to_string(totmem);
    printer.echo(out);

    return mem;
}

}

#else
long ExawindSolver::mem_usage() {return -1;}
long OversetSimulation::mem_usage_all(const int ) {return -1;}
#endif


