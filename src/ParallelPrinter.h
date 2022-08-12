#ifndef PARALLELPRINTER_H
#define PARALLELPRINTER_H
#include <fstream>
#include <iostream>
#include <ostream>
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

    void reset()
    {
        const std::string filename = "timings.dat";
        remove(filename.c_str());

        std::ofstream fp;
        fp.open(filename.c_str(), std::ios_base::out);

        std::string hyphens;
        hyphens.assign(21, '-');
        fp << hyphens << "Exawind detailed timing" << hyphens;

        fp.close();
    };

    void timing_to_file(const std::string& out)
    {
        if (m_rank == m_io_rank) {
            const std::string filename = "timings.dat";
            std::ofstream fp;
            fp.open(filename.c_str(), std::ios_base::app);
            fp << out << std::endl;
            fp.close();
        }
    };

    void time_step_header()
    {
        if (m_rank == m_io_rank) {
            std::ostringstream outstream;
            const char separator = ' ';
            const int name_width = 25;
            const int num_width = 10;
            const int num_precision = 4;
            std::string hyphens;
            hyphens.assign(65, '-');

            outstream << std::endl << hyphens << std::endl
                      << std::left << std::setw(name_width)
                      << std::setfill(separator) << "Routine"
                      << std::setw(num_width) << std::setfill(separator)
                      << std::fixed << std::setprecision(num_precision)
                      << std::right << "step"
                      << std::setw(num_width) << std::setfill(separator)
                      << std::fixed << std::setprecision(num_precision)
                      << std::right << "min"
                      << std::setw(num_width) << std::setfill(separator)
                      << std::fixed << std::setprecision(num_precision)
                      << std::right << "avg"
                      << std::setw(num_width) << std::setfill(separator)
                      << std::fixed << std::setprecision(num_precision)
                      << std::right << "max"
                      << std::endl << hyphens;

            std::cout << outstream.str() << std::endl;
            timing_to_file(outstream.str());
        }
    };

    int io_rank() { return m_io_rank; };
    bool is_io_rank() { return m_rank == m_io_rank; };
};
} // namespace exawind
#endif /* PARALLELPRINTER_H */
