#ifndef TIMERS_H
#define TIMERS_H

#include "Timer.h"
#include "mpi.h"

namespace exawind {

struct Timers
{
    std::vector<Timer<>> m_timers;
    std::vector<std::string> m_names;

    Timers(const std::vector<std::string>& names) : m_names(names)
    {
        Timer clock;
        int cnt = 0;
        for (const auto& name : names) {
            m_timers.push_back(clock);
            cnt++;
        }
    };

    const std::vector<double> counts()
    {
        std::vector<double> counts;
        for (auto const& timer : m_timers) {
            counts.push_back(timer.duration().count());
        }
        return counts;
    };

    void tick(const std::string name, const bool incremental = false)
    {
        m_timers.at(idx(name)).tick(incremental);
    };

    void tock(const std::string name) { m_timers.at(idx(name)).tock(); };

    int idx(const std::string key)
    {
        std::vector<std::string>::iterator itr =
            std::find(m_names.begin(), m_names.end(), key);

        assert(itr != m_names.cend());
        return std::distance(m_names.begin(), itr);
    }
};

void summarize_timers(const std::string name, Timers timers, MPI_Comm comm)
{
    const int root = 0;
    int psize;
    MPI_Comm_size(comm, &psize);

    const auto len = timers.m_timers.size();
    const auto times = timers.counts();
    std::vector<double> maxtimes(len, 0.0);
    std::vector<double> mintimes(len, 0.0);
    std::vector<double> avgtimes(len, 0.0);
    MPI_Reduce(
        times.data(), maxtimes.data(), len, MPI_DOUBLE, MPI_MAX, root, comm);
    MPI_Reduce(
        times.data(), mintimes.data(), len, MPI_DOUBLE, MPI_MIN, root, comm);
    MPI_Reduce(
        times.data(), avgtimes.data(), len, MPI_DOUBLE, MPI_SUM, root, comm);
    for (auto& elem : avgtimes) {
        elem /= psize;
    }

    MPI_Barrier(MPI_COMM_WORLD);
    amrex::Print(std::cout) << name << std::endl;
    const double ms2s = 1000.0;
    for (int i = 0; i < len; i++) {
        amrex::Print(std::cout)
            << "   " << timers.m_names.at(i) << ": " << avgtimes.at(i) / ms2s
            << " (min: " << mintimes.at(i) / ms2s
            << ", max: " << maxtimes.at(i) / ms2s << ")" << std::endl;
    }
};

} // namespace exawind
#endif /* TIMERS_H */
