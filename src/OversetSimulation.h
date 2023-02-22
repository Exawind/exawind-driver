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
    //! List of start ranks for all nalu-wind instances
    int m_num_nw_solvers;
    std::vector<int> m_nw_start_rank;
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
    const std::vector<std::string> m_names_exa{"TimeStep"};
    const std::vector<std::string> m_names_tg{"Connectivity", "SolExchange"};
    //! Timer
    Timers m_timers_exa;
    Timers m_timers_tg;

public:
    OversetSimulation(MPI_Comm comm);
    ~OversetSimulation();

    //! Register a solver
    template <class Solver, class... Args>
    void register_solver(Args... args)
    {
        m_solvers.emplace_back(
            std::make_unique<Solver>(std::forward<Args>(args)..., m_tg));
    }

    //! Delete solvers
    void delete_solvers()
    {
        for (auto& ss : m_solvers) {
            ss.reset();
        }
    }

    //! Initialize all solvers
    void initialize();

    //! Determine field, fringe, hole information
    void perform_overset_connectivity();

    //! Exchange solution between solvers
    void exchange_solution(bool increment_time = false);

    //! Run prescribed number of timesteps
    void run_timesteps(const int add_pic_its, const int nsteps = 1);

    //! Print something
    void echo(const std::string& out) { m_printer.echo(out); }

    //! track detailed timing and print to file
    void print_timing(const int nt);

    //! track memory usage and print to file
    long mem_usage_all(const int step);

    //! set number of nalu-wind instances
    void set_nw_start_rank(const std::vector<int>& start_ranks)
    {
        m_nw_start_rank = start_ranks;
        m_num_nw_solvers = m_nw_start_rank.size();
    }
};

} // namespace exawind
#endif /* OVERSETSIMULATION_H */
