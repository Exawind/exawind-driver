#include "amr-wind/incflo.H"
#include "AMRTiogaIface.h"

namespace TIOGA {
class tioga;
}

namespace exwsim {

class AMRWind
{
private:
    incflo m_incflo;
    AMRTiogaIface m_tgiface;

public:
    static void initialize(MPI_Comm comm, const std::string& inpfile);
    static void finalize();

    AMRWind(TIOGA::tioga&);

    void init_prolog(bool multi_solver_mode = true);
    void init_epilog();

    void prepare_solver_prolog();
    void prepare_solver_epilog();

    void pre_advance_stage1();
    void pre_advance_stage2();
    void advance_timestep();
    void post_advance();

    void pre_overset_conn_work();
    void post_overset_conn_work();
    void register_solution(
        const std::vector<std::string>&, const std::vector<std::string>&);
    void update_solution();
};

} // namespace exwsim
