#ifndef TIMERS_H
#define TIMERS_H

#include "mpi.h"
#include <vector>
#include <algorithm>
#include <chrono>
#include <numeric>
#include <cassert>
#include <iomanip>
#include <sstream>
#include <string>

namespace exawind {

class Timer
{
    using ClockT = std::chrono::steady_clock;
    using TimePt = std::chrono::time_point<ClockT>;
    using TimeT = std::chrono::milliseconds;

    TimePt _start = ClockT::now(), _end = {};
    TimeT _increment;

public:
    void tick(const bool incremental = true)
    {
        _start = ClockT::now();
        if ((incremental) && (_end != TimePt{})) {
            _start -= _increment;
        }
        _end = TimePt{};
        _increment = duration();
    }

    void tock()
    {
        _end = ClockT::now();
        _increment = duration();
    }

    TimeT duration() const
    {
        return std::chrono::duration_cast<TimeT>(_end - _start);
    }
};

struct Timers
{
    std::vector<Timer> m_timers;
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

    void addTimer(std::string name)
    {
        m_names.push_back(name);
        Timer clock;
        m_timers.push_back(clock);
    }

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

    std::string get_timings(MPI_Comm comm, int root = 0)
    {
        const auto len = m_timers.size();
        const auto times = counts();
        std::vector<double> maxtimes(len, 0.0);
        std::vector<double> mintimes(len, 0.0);
        std::vector<double> avgtimes(len, 0.0);
        MPI_Reduce(
            times.data(), maxtimes.data(), len, MPI_DOUBLE, MPI_MAX, root,
            comm);
        MPI_Reduce(
            times.data(), mintimes.data(), len, MPI_DOUBLE, MPI_MIN, root,
            comm);
        MPI_Reduce(
            times.data(), avgtimes.data(), len, MPI_DOUBLE, MPI_SUM, root,
            comm);

        int psize;
        MPI_Comm_size(comm, &psize);
        for (auto& elem : avgtimes) {
            elem /= psize;
        }

        MPI_Barrier(comm);
        std::ostringstream outstream;
        const double ms2s = 1000.0;
        const char separator = ' ';
        const int name_width = 10;
        const int num_width = 8;
        for (int i = 0; i < len; i++) {
            outstream << "  " << std::left << std::setw(name_width)
                      << std::setfill(separator) << m_names.at(i) + ":"
                      << std::setw(num_width) << std::setfill(separator)
                      << std::fixed << std::setprecision(4) << std::right
                      << (mintimes.at(i) / ms2s) << std::setw(num_width)
                      << std::setfill(separator) << std::fixed
                      << std::setprecision(4) << std::right
                      << (avgtimes.at(i) / ms2s) << std::setw(num_width)
                      << std::setfill(separator) << std::fixed
                      << std::setprecision(4) << std::right
                      << (maxtimes.at(i) / ms2s) << std::endl;
        }
        const double total = std::accumulate(
            avgtimes.begin(), avgtimes.end(),
            decltype(avgtimes)::value_type(0.0));
        outstream << "  " << std::left << std::setw(name_width)
                  << std::setfill(separator) << "Total:" << std::setw(num_width)
                  << std::setfill(separator) << std::fixed
                  << std::setprecision(4) << std::right << (total / ms2s);
        std::string out(outstream.str());
        return out;
    };
};
} // namespace exawind
#endif /* TIMERS_H */
