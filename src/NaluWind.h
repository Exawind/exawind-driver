#ifndef NALUWIND_H
#define NALUWIND_H

#include <vector>
#include <string>

#include "Simulation.h"
#include "stk_util/parallel/Parallel.hpp"
#include "yaml-cpp/yaml.h"

namespace TIOGA {
class tioga;
}

namespace exwsim {

class NaluWind
{
private:
    YAML::Node m_doc;
    sierra::nalu::Simulation m_sim;

public:
    static void initialize();
    static void finalize();

    NaluWind(
        stk::ParallelMachine comm,
        const std::string& inp_file,
        TIOGA::tioga& tg);

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
    void register_solution(const std::vector<std::string>&);
    void update_solution();
};

} // namespace exwsim

#endif /* NALUWIND_H */
