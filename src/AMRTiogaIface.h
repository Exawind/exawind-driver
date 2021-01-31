#ifndef AMRTIOGAIFACE_H
#define AMRTIOGAIFACE_H

#include <memory>
#include <vector>

namespace amr_wind {
class CFDSim;
}

namespace TIOGA {
class tioga;
struct AMRMeshInfo;
}

namespace exwsim {

class AMRTiogaIface
{
public:
    AMRTiogaIface(amr_wind::CFDSim&, TIOGA::tioga& tg);

    void pre_overset_conn_work();

    void post_overset_conn_work();

    void register_mesh();

    void register_solution(
        const std::vector<std::string>& cell_vars,
        const std::vector<std::string>& node_vars);

    void update_solution();

private:
    amr_wind::CFDSim& m_sim;
    TIOGA::tioga& m_tg;

    std::unique_ptr<TIOGA::AMRMeshInfo> m_info;
};

} // namespace exwsim

#endif /* AMRTIOGAIFACE_H */
