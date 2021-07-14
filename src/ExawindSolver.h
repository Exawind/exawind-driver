#ifndef EXAWINDSOLVER_H
#define EXAWINDSOLVER_H

namespace exawind {

class ExawindSolver
{
public:
    void call_init_prolog(bool multi_solver_mode = true)
    {
        init_prolog(multi_solver_mode);
    };
    void call_init_epilog() { init_epilog(); };
    void call_prepare_solver_prolog() { prepare_solver_prolog(); };
    void call_prepare_solver_epilog() { prepare_solver_epilog(); };
    void call_pre_advance_stage1() { pre_advance_stage1(); };
    void call_pre_advance_stage2() { pre_advance_stage2(); };
    void call_advance_timestep() { advance_timestep(); };
    void call_post_advance() { post_advance(); };
    void call_pre_overset_conn_work() { pre_overset_conn_work(); };
    void call_post_overset_conn_work() { post_overset_conn_work(); };
    void call_register_solution() { register_solution(); };
    void call_update_solution() { update_solution(); };
    virtual bool is_unstructured() { return false; };
    virtual bool is_amr() { return false; };
    virtual int overset_update_interval() { return 100000000; };
    virtual std::string identifier() { return "ExawindSolver"; }

protected:
    virtual void init_prolog(bool multi_solver_mode = true) = 0;
    virtual void init_epilog() = 0;
    virtual void prepare_solver_prolog() = 0;
    virtual void prepare_solver_epilog() = 0;
    virtual void pre_advance_stage1() = 0;
    virtual void pre_advance_stage2() = 0;
    virtual void advance_timestep() = 0;
    virtual void post_advance() = 0;
    virtual void pre_overset_conn_work() = 0;
    virtual void post_overset_conn_work() = 0;
    virtual void register_solution() = 0;
    virtual void update_solution() = 0;
};

} // namespace exawind
#endif /* EXAWINDSOLVER_H */
