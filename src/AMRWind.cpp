#include "AMRWind.h"
#include "amr-wind/incflo.H"
#include "amr-wind/CFDSim.H"
#include "amr-wind/core/SimTime.H"
#include "amr-wind/utilities/console_io.H"
#include "AMReX.H"
#include "AMReX_ParmParse.H"

#include "tioga.h"

namespace exwsim {

void AMRWind::initialize(
    MPI_Comm comm, const std::string& inpfile, std::ofstream& out)
{
    int argc = 2;
    char** argv = new char*[argc];

    const char* exename = "amr_wind";
    argv[0] = const_cast<char*>(exename);
    argv[1] = const_cast<char*>(inpfile.c_str());

    amrex::Initialize(
        argc, argv, true, comm,
        []() {
            amrex::ParmParse pp("amrex");
            // Set the defaults so that we throw an exception instead of
            // attempting to generate backtrace files. However, if the user has
            // explicitly set these options in their input files respect those
            // settings.
            if (!pp.contains("throw_exception")) pp.add("throw_exception", 1);
            if (!pp.contains("signal_handling")) pp.add("signal_handling", 0);
        },
        out, out);

    amr_wind::io::print_banner(comm, amrex::OutStream());

    delete[] argv;
}

void AMRWind::finalize() { amrex::Finalize(); }

AMRWind::AMRWind(TIOGA::tioga& tg) : m_incflo(), m_tgiface(m_incflo.sim(), tg)
{
    m_incflo.sim().activate_overset();
}

void AMRWind::init_prolog(bool)
{
    m_incflo.init_mesh();
    m_incflo.init_amr_wind_modules();
}

void AMRWind::init_epilog() {}

void AMRWind::prepare_solver_prolog() {}

void AMRWind::prepare_solver_epilog()
{
    m_incflo.prepare_for_time_integration();
}

void AMRWind::pre_advance_stage1()
{
    m_incflo.sim().time().new_timestep();
    m_incflo.regrid_and_update();
    m_incflo.pre_advance_stage1();
}

void AMRWind::pre_advance_stage2() { m_incflo.pre_advance_stage2(); }

void AMRWind::advance_timestep() { m_incflo.advance(); }

void AMRWind::post_advance() { m_incflo.post_advance_work(); }

void AMRWind::pre_overset_conn_work() { m_tgiface.pre_overset_conn_work(); }

void AMRWind::post_overset_conn_work() { m_tgiface.post_overset_conn_work(); }

void AMRWind::register_solution(
    const std::vector<std::string>& cell_vars,
    const std::vector<std::string>& node_vars)
{
    m_tgiface.register_solution(cell_vars, node_vars);
}

void AMRWind::update_solution() { m_tgiface.update_solution(); }

} // namespace exwsim
