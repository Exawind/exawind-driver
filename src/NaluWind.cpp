#include "NaluWind.h"
#include "NaluEnv.h"
#include "Realm.h"
#include "TimeIntegrator.h"
#include "overset/ExtOverset.h"
#include "overset/TiogaRef.h"

#include "Kokkos_Core.hpp"
#include "tioga.h"
#include "HypreNGP.h"

namespace exawind {

void NaluWind::initialize()
{
    Kokkos::initialize();
    // Hypre initialization
    nalu_hypre::hypre_initialize();
    nalu_hypre::hypre_set_params();
}

void NaluWind::finalize()
{
    // Hypre cleanup
    nalu_hypre::hypre_finalize();

    if (Kokkos::is_initialized()) {
        Kokkos::finalize();
    }
}

NaluWind::NaluWind(
    int id,
    stk::ParallelMachine comm,
    const std::string& inpfile,
    const std::vector<std::string>& fnames,
    TIOGA::tioga& tg)
    : m_id(id)
    , m_comm(comm)
    , m_doc(YAML::LoadFile(inpfile))
    , m_fnames(fnames)
    , m_sim(m_doc)
{
    auto& env = sierra::nalu::NaluEnv::self();
    env.parallelCommunicator_ = comm;
    MPI_Comm_size(comm, &env.pSize_);
    MPI_Comm_rank(comm, &env.pRank_);

    ::tioga_nalu::TiogaRef::self(&tg);

    int extloc = inpfile.rfind(".");
    std::string logfile = inpfile;
    if (extloc != std::string::npos) {
        logfile = inpfile.substr(0, extloc) + ".log";
    }
    env.set_log_file_stream(logfile);
}

NaluWind::~NaluWind() = default;

void NaluWind::init_prolog(bool multi_solver_mode)
{
    m_sim.load(m_doc);
    if (m_sim.timeIntegrator_->overset_ != nullptr)
        m_sim.timeIntegrator_->overset_->set_multi_solver_mode(
            multi_solver_mode);
    m_sim.breadboard();
    m_sim.init_prolog();
}

void NaluWind::init_epilog() { m_sim.init_epilog(); }

void NaluWind::prepare_solver_prolog()
{
    m_sim.timeIntegrator_->prepare_for_time_integration();
}

void NaluWind::prepare_solver_epilog()
{
    for (auto* realm : m_sim.timeIntegrator_->realmVec_)
        realm->output_converged_results();
}

void NaluWind::pre_advance_stage1()
{
    m_sim.timeIntegrator_->pre_realm_advance_stage1();
}

void NaluWind::pre_advance_stage2()
{
    m_sim.timeIntegrator_->pre_realm_advance_stage2();
}

void NaluWind::advance_timestep()
{
    for (auto* realm : m_sim.timeIntegrator_->realmVec_)
        realm->advance_time_step();
}

void NaluWind::additional_picard_iterations(const int n)
{
    for (auto* realm : m_sim.timeIntegrator_->realmVec_)
        realm->nonlinear_iterations(n);
}

void NaluWind::post_advance() { m_sim.timeIntegrator_->post_realm_advance(); }

void NaluWind::pre_overset_conn_work()
{
    m_sim.timeIntegrator_->overset_->pre_overset_conn_work();
}

void NaluWind::post_overset_conn_work()
{
    m_sim.timeIntegrator_->overset_->post_overset_conn_work();
}

void NaluWind::register_solution()
{
    m_ncomps = m_sim.timeIntegrator_->overset_->register_solution(m_fnames);
}

void NaluWind::update_solution()
{
    m_sim.timeIntegrator_->overset_->update_solution();
}

int NaluWind::overset_update_interval()
{
    for (auto& realm : m_sim.timeIntegrator_->realmVec_) {
        if (realm->does_mesh_move()) {
            return 1;
        }
    }
    return 100000000;
}

int NaluWind::time_index() { return m_sim.timeIntegrator_->timeStepCount_; }

void NaluWind::dump_simulation_time()
{
    for (auto& realm : m_sim.timeIntegrator_->realmVec_) {
        realm->dump_simulation_time();
    }
}

} // namespace exawind
