#include "OversetSimulation.h"
#include <algorithm>

namespace exawind {

OversetSimulation::OversetSimulation(MPI_Comm comm)
    : m_comm(comm), m_printer(comm), m_timers(m_names)
{
    int psize, prank;
    MPI_Comm_size(m_comm, &psize);
    MPI_Comm_rank(m_comm, &prank);
    m_tg.setCommunicator(MPI_COMM_WORLD, prank, psize);
}

void OversetSimulation::check_solver_types()
{
    bool flag[2] = {false, false};
    bool gflag[2] = {false, false};
    flag[0] =
        std::any_of(m_solvers.begin(), m_solvers.end(), [](const auto& ss) {
            return ss->is_amr();
        });
    flag[1] =
        std::any_of(m_solvers.begin(), m_solvers.end(), [](const auto& ss) {
            return ss->is_unstructured();
        });
    MPI_Allreduce(flag, gflag, 2, MPI_CXX_BOOL, MPI_LOR, m_comm);
    m_has_amr = gflag[0];
    m_has_unstructured = gflag[1];
}

void OversetSimulation::determine_overset_interval()
{
    int flag[1] = {m_overset_update_interval};
    int gflag[1] = {m_overset_update_interval};
    for (auto& ss : m_solvers) {
        flag[0] = std::min(flag[0], ss->overset_update_interval());
    }
    MPI_Allreduce(flag, gflag, 1, MPI_INT, MPI_MIN, m_comm);
    m_overset_update_interval = gflag[0];
    m_printer.echo(
        "Overset update interval = " +
        std::to_string(m_overset_update_interval));
}

void OversetSimulation::initialize()
{
    check_solver_types();
    if (!m_has_unstructured) {
        throw std::runtime_error(
            "OversetSimulationulation requires at least one unstructured "
            "solver");
    }

    for (auto& ss : m_solvers) ss->call_init_prolog(true);

    determine_overset_interval();

    perform_overset_connectivity();

    for (auto& ss : m_solvers) {
        ss->call_init_epilog();
        ss->call_prepare_solver_prolog();
    }

    exchange_solution();

    for (auto& ss : m_solvers) ss->call_prepare_solver_epilog();

    MPI_Barrier(m_comm);
    m_initialized = true;
}

void OversetSimulation::perform_overset_connectivity()
{
    for (auto& ss : m_solvers) ss->call_pre_overset_conn_work();

    m_timers.tick("TGConn");
    if (m_has_amr) m_tg.preprocess_amr_data();

    m_tg.profile();
    m_tg.performConnectivity();
    if (m_has_amr) m_tg.performConnectivityAMR();
    m_timers.tock("TGConn");

    for (auto& ss : m_solvers) ss->call_post_overset_conn_work();
}

void OversetSimulation::exchange_solution()
{

    for (auto& ss : m_solvers) ss->call_register_solution();

    m_timers.tick("TGConn");
    if (m_has_amr) {
        m_tg.dataUpdate_AMR();
    } else {
        throw std::runtime_error("Invalid overset exchange");
    }
    m_timers.tock("TGConn");

    for (auto& ss : m_solvers) ss->call_update_solution();
}

void OversetSimulation::run_timesteps(int nsteps)
{

    if (!m_initialized) {
        throw std::runtime_error("OversetSimulation has not been initialized");
    }

    const int tstart = m_last_timestep + 1;
    const int tend = m_last_timestep + 1 + nsteps;
    m_printer.echo(
        "Running " + std::to_string(nsteps) + " timesteps starting from " +
        std::to_string(tstart));

    for (int nt = tstart; nt < tend; ++nt) {
        for (auto& ss : m_solvers) ss->call_pre_advance_stage1();

        if (do_connectivity(nt)) perform_overset_connectivity();

        for (auto& ss : m_solvers) ss->call_pre_advance_stage2();

        exchange_solution();

        for (auto& ss : m_solvers) ss->call_advance_timestep();

        for (auto& ss : m_solvers) ss->call_post_advance();

        MPI_Barrier(m_comm);
        const auto timings = m_timers.get_timings(m_comm, m_printer.io_rank());
        m_printer.echo("WCTime: " + std::to_string(nt));
        m_printer.echo(timings);
        for (auto& ss : m_solvers) {
            const auto ss_timings =
                ss->m_timers.get_timings(m_comm, m_printer.io_rank());
            m_printer.echo(
                "WCTime for " + ss->identifier() + ": " + std::to_string(nt));
            m_printer.echo(timings);
        }
    }

    m_last_timestep = tend;
}

bool OversetSimulation::do_connectivity(const int tstep)
{
    return (tstep > 0) && (tstep % m_overset_update_interval) == 0;
}

} // namespace exawind
