#ifndef TIMER_H
#define TIMER_H
#include <chrono>

namespace exawind {
// Almost straight from: https://stackoverflow.com/a/21995693
template <
    class TimeT = std::chrono::milliseconds,
    class ClockT = std::chrono::steady_clock>
class Timer
{
    using timep_t = typename ClockT::time_point;
    timep_t _start = ClockT::now(), _end = {};
    TimeT _increment;

public:
    void tick(const bool incremental = false)
    {
        _start = ClockT::now();
        if ((incremental) && (_end != timep_t{})) {
            _start -= _increment;
        }
        _end = timep_t{};
        _increment = duration();
    }

    void tock()
    {
        _end = ClockT::now();
        _increment = duration();
    }

    template <class TT = TimeT>
    TT duration() const
    {
        return std::chrono::duration_cast<TT>(_end - _start);
    }
};
} // namespace exawind
#endif /* TIMER_H */
