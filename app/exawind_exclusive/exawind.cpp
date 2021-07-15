#include "AMRWind.h"
#include "NaluWind.h"
#include "OversetSimulation.h"
#include "MPIUtilities.h"
#include "mpi.h"
#include <filesystem>

#include "yaml-cpp/yaml.h"
#include "tioga.h"

static std::string usage(std::string name)
{
    return "usage: " + name +
           " [--awind NPROCS] [--nwind NPROCS] input_file\n" +
           "\t-h,--help\t\tShow this help message\n" +
           "\t--awind NPROCS\t\tNumber of ranks for AMR-Wind (default = all "
           "ranks)\n" +
           "\t--nwind NPROCS\t\tNumber of ranks for Nalu-Wind (default = all "
           "ranks)\n";
}

int main(int argc, char** argv)
{
    MPI_Init(&argc, &argv);
    int psize, prank;
    MPI_Comm_size(MPI_COMM_WORLD, &psize);
    MPI_Comm_rank(MPI_COMM_WORLD, &prank);

    if ((argc != 2) && (argc != 4) && (argc != 6)) {
        throw std::runtime_error(usage(argv[0]));
    }

    int num_nwind_ranks = psize;
    int num_awind_ranks = psize;
    std::string inpfile = "";

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if ((arg == "-h") || (arg == "--help")) {
            if (prank == 0) std::cout << usage(argv[0]);
            return 0;
        } else if (arg == "--awind") {
            if (i + 1 < argc) {
                std::string opt = argv[++i];
                num_awind_ranks = std::stoi(opt);
                if (num_awind_ranks > psize) {
                    throw std::runtime_error(
                        "--awind option requesting more ranks than available.");
                }
            } else {
                throw std::runtime_error(
                    "--awind option requires one argument.");
            }
        } else if (arg == "--nwind") {
            if (i + 1 < argc) {
                std::string opt = argv[++i];
                num_nwind_ranks = std::stoi(opt);
                if (num_nwind_ranks > psize) {
                    throw std::runtime_error(
                        "--nwind option requesting more ranks than available.");
                }
            } else {
                throw std::runtime_error(
                    "--nwind option requires one argument.");
            }
        } else {
            inpfile = argv[i];
        }
    }

    if (num_awind_ranks + num_nwind_ranks < psize) {
        if (prank == 0)
            std::cout << "Warning: using fewer ranks than available ranks: MPI "
                         "size = " +
                             std::to_string(psize) + "; Num ranks used = " +
                             std::to_string(num_awind_ranks + num_nwind_ranks)
                      << std::endl;
    }

    MPI_Comm amr_comm =
        exawind::create_subcomm(MPI_COMM_WORLD, num_awind_ranks, 0);
    MPI_Comm nalu_comm = exawind::create_subcomm(
        MPI_COMM_WORLD, num_nwind_ranks, psize - num_nwind_ranks);

    const YAML::Node doc(YAML::LoadFile(inpfile));
    const YAML::Node node = doc["exawind"];
    const std::string amr_inp = node["amr_wind_inp"].as<std::string>();
    const std::string nalu_inp = node["nalu_wind_inp"].as<std::string>();
    std::filesystem::path fpath_amr_inp(amr_inp);
    const std::string amr_log =
        fpath_amr_inp.replace_extension(".log").string();
    std::ofstream out(amr_log);

    exawind::OversetSimulation sim(MPI_COMM_WORLD);
    sim.echo(
        "Initializing AMR-Wind on " + std::to_string(num_awind_ranks) +
        " MPI ranks");
    if (amr_comm != MPI_COMM_NULL)
        exawind::AMRWind::initialize(amr_comm, amr_inp, out);
    sim.echo(
        "Initializing Nalu-Wind on " + std::to_string(num_nwind_ranks) +
        " MPI ranks");
    if (nalu_comm != MPI_COMM_NULL) exawind::NaluWind::initialize();

    {
        const auto nalu_vars = node["nalu_vars"].as<std::vector<std::string>>();
        const auto amr_cvars =
            node["amr_cell_vars"].as<std::vector<std::string>>();
        const auto amr_nvars =
            node["amr_node_vars"].as<std::vector<std::string>>();
        const int num_timesteps = node["num_timesteps"].as<int>();

        if (nalu_comm != MPI_COMM_NULL)
            sim.register_solver<exawind::NaluWind>(
                nalu_comm, nalu_inp, nalu_vars);
        if (amr_comm != MPI_COMM_NULL)
            sim.register_solver<exawind::AMRWind>(amr_cvars, amr_nvars);

        sim.echo("Initializing overset simulation");
        sim.initialize();
        sim.echo("Initialization successful");

        sim.run_timesteps(num_timesteps);
    }

    if (amr_comm != MPI_COMM_NULL) exawind::AMRWind::finalize();
    if (nalu_comm != MPI_COMM_NULL) exawind::NaluWind::finalize();
    MPI_Finalize();

    return 0;
}
