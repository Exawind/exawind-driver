#ifndef PARALLELPRINTER_H
#define PARALLELPRINTER_H
#include "mpi.h"

namespace exawind {

class ParallelPrinter
{
private:
    MPI_Comm m_comm;
    int m_rank;
    int m_io_rank;

public:
    ParallelPrinter(MPI_Comm comm, const int io_rank = 0)
        : m_comm(comm), m_io_rank(io_rank)
    {
        MPI_Comm_rank(m_comm, &m_rank);
    };

    void echo(const std::string& out)
    {
        if (m_rank == m_io_rank) std::cout << out << std::endl;
    };

    int io_rank() { return m_io_rank; };
    bool is_io_rank() { return m_rank == m_io_rank; };
};
} // namespace exawind
#endif /* PARALLELPRINTER_H */
