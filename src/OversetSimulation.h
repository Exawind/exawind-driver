#ifndef OVERSETSIMULATION_H
#define OVERSETSIMULATION_H
#include "mpi.h"
#include "tioga.h"
#include "ExawindSolver.h"
#include "ParallelPrinter.h"
#include "Timers.h"

namespace TIOGA {
class tioga;
}

namespace exawind {

class OversetSimulation
{
private:
    //! World communicator instance
    MPI_Comm m_comm;
    //! List of solvers active in this overset simulation
    std::vector<std::unique_ptr<ExawindSolver>> m_solvers;
    //! Flag indicating whether an AMR solver is active
    bool m_has_amr{false};
    //! Flag indicating whether an unstructured solver is active
    bool m_has_unstructured{false};
    //! Interval for overset updates during timestepping
    int m_overset_update_interval{100000000};
    //! Last timestep run during this simulation
    int m_last_timestep{0};
    //! Flag indicating whether initialization tasks have been performed
    bool m_initialized{false};
    //! Tioga instance
    TIOGA::tioga m_tg;
    //! Determine unstructured and structured solver types
    void check_solver_types();
    //! Determine if we should update connectivity during time integration
    void determine_overset_interval();
    //! Return True if connectivity must be updated at a given timestep
    bool do_connectivity(const int tstep);
    //! Parallel printer utility
    ParallelPrinter m_printer;
    //! Timer names
    const std::vector<std::string> m_names{
        "Init", "Pre", "Conn", "Solve", "Post"};
    //! Timers
    Timers m_timers;

public:
    OversetSimulation(MPI_Comm comm);

    //! Register a solver
    template <class Solver, class... Args>
    void register_solver(Args... args)
    {
        m_solvers.emplace_back(
            std::make_unique<Solver>(std::forward<Args>(args)..., m_tg));
    }

    //! Initialize all solvers
    void initialize();

    //! Determine field, fringe, hole information
    void perform_overset_connectivity();

    //! Exchange solution between solvers
    void exchange_solution();

    //! Run prescribed number of timesteps
    void run_timesteps(int nsteps = 1);

    //! Print something
    void echo(const std::string& out) { m_printer.echo(out); }
};

} // namespace exawind
#endif /* OVERSETSIMULATION_H */
