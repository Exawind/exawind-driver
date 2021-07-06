#ifndef EXAWINDSOLVER_H
#define EXAWINDSOLVER_H

namespace exawind {

class ExawindSolver
{
public:
    virtual void init_prolog(bool multi_solver_mode = true){};
    virtual void init_epilog(){};

    virtual void prepare_solver_prolog(){};
    virtual void prepare_solver_epilog(){};

    virtual void pre_advance_stage1(){};
    virtual void pre_advance_stage2(){};
    virtual void advance_timestep(){};
    virtual void post_advance(){};

    virtual void pre_overset_conn_work(){};
    virtual void post_overset_conn_work(){};
    virtual void register_solution(){};
    virtual void update_solution(){};
    virtual bool is_unstructured() { return false; };
    virtual bool is_amr() { return false; };
    virtual int overset_update_interval() { return 100000000; };
    virtual std::string identifier() { return "ExawindSolver"; }
};

} // namespace exawind
#endif /* EXAWINDSOLVER_H */
