#include "amr-wind/incflo.H"
#include "AMRTiogaIface.h"
#include "ExawindSolver.h"

namespace TIOGA {
class tioga;
}

namespace exawind {

class AMRWind : public ExawindSolver
{
private:
    incflo m_incflo;
    AMRTiogaIface m_tgiface;
    std::vector<std::string> m_cell_vars;
    std::vector<std::string> m_node_vars;

public:
    static void
    initialize(MPI_Comm comm, const std::string& inpfile, std::ofstream& out);
    static void finalize();
    explicit AMRWind(
        const std::vector<std::string>&,
        const std::vector<std::string>&,
        TIOGA::tioga&);
    ~AMRWind();
    bool is_unstructured() override { return false; }
    bool is_amr() override { return true; }
    int overset_update_interval() override;
    int time_index() override;
    std::string identifier() override { return "AMR-Wind"; }
    MPI_Comm comm() override { return m_comm; }

protected:
    void init_prolog(bool multi_solver_mode = true) override;
    void init_epilog() override;
    void prepare_solver_prolog() override;
    void prepare_solver_epilog() override;
    void pre_advance_stage1() override;
    void pre_advance_stage2() override;
    void advance_timestep() override;
    void additional_picard_iteration() override;
    void post_advance() override;
    void pre_overset_conn_work() override;
    void post_overset_conn_work() override;
    void register_solution() override;
    void update_solution() override;
    void dump_simulation_time() override{};
    MPI_Comm m_comm;
};

} // namespace exawind
