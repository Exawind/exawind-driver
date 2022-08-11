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
        for (const auto& name : names) {
            m_timers.push_back(clock);
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

    std::string get_timings(
        std::string solver,
        int step,
        MPI_Comm comm,
        int root = 0,
        bool print_total = false)
    {
        const auto len = m_timers.size();
        const auto times = counts();
        std::vector<double> mintimes(len, 0.0);
        std::vector<double> avgtimes(len, 0.0);
        std::vector<double> maxtimes(len, 0.0);
        MPI_Reduce(
            times.data(), mintimes.data(), len, MPI_DOUBLE, MPI_MIN, root,
            comm);
        MPI_Reduce(
            times.data(), avgtimes.data(), len, MPI_DOUBLE, MPI_SUM, root,
            comm);
        MPI_Reduce(
            times.data(), maxtimes.data(), len, MPI_DOUBLE, MPI_MAX, root,
            comm);

        int psize;
        MPI_Comm_size(comm, &psize);
        for (auto& elem : avgtimes) {
            elem /= psize;
        }

        std::ostringstream outstream;
        const double ms2s = 1000.0;
        const char separator = ' ';
        const int name_width = 25;
        const int num_width = 10;
        const int num_precision = 4;
        for (int i = 0; i < len; ++i) {
            outstream << std::left << std::setw(name_width)
                      << std::setfill(separator) << solver + "::" + m_names.at(i)
                      << std::setw(num_width) << std::setfill(separator)
                      << std::fixed << std::setprecision(num_precision)
                      << std::right << step
                      << std::setw(num_width) << std::setfill(separator)
                      << std::fixed << std::setprecision(num_precision)
                      << std::right << (mintimes.at(i) / ms2s)
                      << std::setw(num_width) << std::setfill(separator)
                      << std::fixed << std::setprecision(num_precision)
                      << std::right << (avgtimes.at(i) / ms2s)
                      << std::setw(num_width) << std::setfill(separator)
                      << std::fixed << std::setprecision(num_precision)
                      << std::right << (maxtimes.at(i) / ms2s);
            if(i < len-1) outstream << std::endl;
        }

        if(print_total)
        {
            const double total_min = std::accumulate(
                mintimes.begin(), mintimes.end(),
                decltype(mintimes)::value_type(0.0));
            const double total_avg = std::accumulate(
                avgtimes.begin(), avgtimes.end(),
                decltype(avgtimes)::value_type(0.0));
            const double total_max = std::accumulate(
                maxtimes.begin(), maxtimes.end(),
                decltype(maxtimes)::value_type(0.0));
            outstream << std::endl << std::left << std::setw(name_width)
                      << std::setfill(separator) << solver + "::Total"
                      << std::setw(num_width) << std::setfill(separator)
                      << std::fixed << std::setprecision(num_precision)
                      << std::right << step
                      << std::setw(num_width) << std::setfill(separator)
                      << std::fixed << std::setprecision(num_precision)
                      << std::right << (total_min / ms2s)
                      << std::setw(num_width) << std::setfill(separator)
                      << std::fixed << std::setprecision(num_precision)
                      << std::right << (total_avg / ms2s)
                      << std::setw(num_width) << std::setfill(separator)
                      << std::fixed << std::setprecision(num_precision)
                      << std::right << (total_max / ms2s);
        }

        std::string out(outstream.str());
        return out;
    };
};
} // namespace exawind
#endif /* TIMERS_H */
