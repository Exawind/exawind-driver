#include "AMRWind.h"
#include "NaluWind.h"
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
        throw std::runtime_error("Usage: nalu_amr <inpfile>");
    }

    const std::string inpfile(argv[1]);
    const YAML::Node doc(YAML::LoadFile(inpfile));
    const YAML::Node node = doc["exwsim"];

    const std::string amr_inp = node["amr_wind_inp"].as<std::string>();
    const std::string nalu_inp = node["nalu_wind_inp"].as<std::string>();

    std::filesystem::path fpath_amr_inp(amr_inp);
    const std::string amr_log =
        fpath_amr_inp.replace_extension(".log").string();
    std::ofstream out(amr_log);

    exwsim::NaluWind::initialize();
    exwsim::AMRWind::initialize(MPI_COMM_WORLD, amr_inp, out);

    {
        const auto nalu_vars = node["nalu_vars"].as<std::vector<std::string>>();
        const auto amr_cvars =
            node["amr_cell_vars"].as<std::vector<std::string>>();
        const auto amr_nvars =
            node["amr_node_vars"].as<std::vector<std::string>>();
        const int nsteps = node["num_timesteps"].as<int>();

        TIOGA::tioga tg;
        tg.setCommunicator(MPI_COMM_WORLD, prank, psize);

        exwsim::NaluWind nalu(MPI_COMM_WORLD, nalu_inp, tg);
        exwsim::AMRWind awind(tg);

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
        for (int i = 0; i < nsteps; ++i) {
            nalu.pre_advance_stage1();
            awind.pre_advance_stage1();

            nalu.pre_overset_conn_work();
            awind.pre_overset_conn_work();
            tg.profile();
            tg.performConnectivity();
            tg.performConnectivityAMR();
            nalu.post_overset_conn_work();
            awind.post_overset_conn_work();
            MPI_Barrier(MPI_COMM_WORLD);

            nalu.pre_advance_stage2();
            awind.pre_advance_stage2();

            nalu.register_solution(nalu_vars);
            awind.register_solution(amr_cvars, amr_nvars);
            tg.dataUpdate_AMR();
            nalu.update_solution();
            awind.update_solution();
            MPI_Barrier(MPI_COMM_WORLD);

            nalu.advance_timestep();
            awind.advance_timestep();
            MPI_Barrier(MPI_COMM_WORLD);

            nalu.post_advance();
            awind.post_advance();
            MPI_Barrier(MPI_COMM_WORLD);
        }
    }

    exwsim::AMRWind::finalize();
    exwsim::NaluWind::finalize();
    MPI_Finalize();

    return 0;
}
