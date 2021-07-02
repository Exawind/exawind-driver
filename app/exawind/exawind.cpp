#include "AMRWind.h"
#include "NaluWind.h"
#include "Timers.h"
#include "mpi.h"
#include <filesystem>

#include "yaml-cpp/yaml.h"
#include "tioga.h"

int main(int argc, char** argv)
{
    MPI_Init(&argc, &argv);
    int psize, prank;
    MPI_Comm_size(MPI_COMM_WORLD, &psize);
    MPI_Comm_rank(MPI_COMM_WORLD, &prank);

    if (argc != 2) {
        throw std::runtime_error("Usage: exawind <inpfile>");
    }

    const std::string inpfile(argv[1]);
    const YAML::Node doc(YAML::LoadFile(inpfile));
    const YAML::Node node = doc["exawind"];

    const std::string amr_inp = node["amr_wind_inp"].as<std::string>();
    const std::string nalu_inp = node["nalu_wind_inp"].as<std::string>();

    std::filesystem::path fpath_amr_inp(amr_inp);
    const std::string amr_log =
        fpath_amr_inp.replace_extension(".log").string();
    std::ofstream out(amr_log);

    exawind::NaluWind::initialize();
    exawind::AMRWind::initialize(MPI_COMM_WORLD, amr_inp, out);

    {
        const auto nalu_vars = node["nalu_vars"].as<std::vector<std::string>>();
        const auto amr_cvars =
            node["amr_cell_vars"].as<std::vector<std::string>>();
        const auto amr_nvars =
            node["amr_node_vars"].as<std::vector<std::string>>();
        const int nsteps = node["num_timesteps"].as<int>();

        TIOGA::tioga tg;
        tg.setCommunicator(MPI_COMM_WORLD, prank, psize);

        exawind::NaluWind nalu(MPI_COMM_WORLD, nalu_inp, tg);
        exawind::AMRWind awind(tg);

        awind.init_prolog();
        nalu.init_prolog();
        nalu.pre_overset_conn_work();
        awind.pre_overset_conn_work();
        tg.profile();
        tg.performConnectivity();
        tg.performConnectivityAMR();
        nalu.post_overset_conn_work();
        awind.post_overset_conn_work();
        nalu.init_epilog();

        nalu.prepare_solver_prolog();
        awind.prepare_solver_prolog();

        nalu.register_solution(nalu_vars);
        awind.register_solution(amr_cvars, amr_nvars);
        tg.dataUpdate_AMR();
        nalu.update_solution();
        awind.update_solution();

        nalu.prepare_solver_epilog();
        awind.prepare_solver_epilog();

        MPI_Barrier(MPI_COMM_WORLD);

        amrex::Print(std::cout)
            << "Running " << nsteps << " timesteps" << std::endl;

        const std::vector<std::string> names{"Pre", "Conn", "Solve", "Post"};
        exawind::Timers nalu_timers(names);
        exawind::Timers awind_timers(names);
        exawind::Timers tg_timers({"Conn"});

        const int root = 0;
        for (int i = 0; i < nsteps; ++i) {
            amrex::Print(std::cout) << "Timestep " << i << std::endl;
            nalu_timers.tick("Pre");
            nalu.pre_advance_stage1();
            nalu_timers.tock("Pre");
            awind_timers.tick("Pre");
            awind.pre_advance_stage1();
            awind_timers.tock("Pre");

            nalu_timers.tick("Conn");
            nalu.pre_overset_conn_work();
            nalu_timers.tock("Conn");
            awind_timers.tick("Conn");
            awind.pre_overset_conn_work();
            awind_timers.tock("Conn");
            tg_timers.tick("Conn");
            tg.profile();
            tg.performConnectivity();
            tg.performConnectivityAMR();
            tg_timers.tock("Conn");
            nalu_timers.tick("Conn", true);
            nalu.post_overset_conn_work();
            nalu_timers.tock("Conn");
            awind_timers.tick("Conn");
            awind.post_overset_conn_work();
            awind_timers.tock("Conn");
            MPI_Barrier(MPI_COMM_WORLD);

            nalu_timers.tick("Pre", true);
            nalu.pre_advance_stage2();
            nalu_timers.tock("Pre");
            awind_timers.tick("Pre");
            awind.pre_advance_stage2();
            awind_timers.tock("Pre");

            nalu_timers.tick("Conn", true);
            nalu.register_solution(nalu_vars);
            nalu_timers.tock("Conn");
            awind_timers.tick("Conn");
            awind.register_solution(amr_cvars, amr_nvars);
            awind_timers.tock("Conn");
            tg_timers.tick("Conn");
            tg.dataUpdate_AMR();
            tg_timers.tock("Conn");
            nalu_timers.tick("Conn", true);
            nalu.update_solution();
            nalu_timers.tock("Conn");
            awind_timers.tick("Conn");
            awind.update_solution();
            awind_timers.tock("Conn");
            MPI_Barrier(MPI_COMM_WORLD);

            nalu_timers.tick("Solve");
            nalu.advance_timestep();
            nalu_timers.tock("Solve");
            awind_timers.tick("Solve");
            awind.advance_timestep();
            awind_timers.tock("Solve");
            MPI_Barrier(MPI_COMM_WORLD);

            nalu_timers.tick("Post");
            nalu.post_advance();
            nalu_timers.tock("Post");
            awind_timers.tick("Post");
            awind.post_advance();
            awind_timers.tock("Post");
            MPI_Barrier(MPI_COMM_WORLD);

            amrex::Print(std::cout)
                << "WallClockTime [s] at step " << i << std::endl;
            exawind::summarize_timers("Nalu-Wind", nalu_timers, MPI_COMM_WORLD);
            exawind::summarize_timers("AMR-Wind", awind_timers, MPI_COMM_WORLD);
            exawind::summarize_timers("Tioga", tg_timers, MPI_COMM_WORLD);
        }
    }

    exawind::AMRWind::finalize();
    exawind::NaluWind::finalize();
    MPI_Finalize();

    return 0;
}
