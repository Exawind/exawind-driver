#include "AMRWind.h"
#include "NaluWind.h"
#include "OversetSimulation.h"
#include "MPIUtilities.h"
#include "mpi.h"
#ifdef EXAWIND_HAS_STD_FILESYSTEM
#include <filesystem>
#endif

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

    const YAML::Node doc(YAML::LoadFile(inpfile));
    const YAML::Node node = doc["exawind"];
    std::string amr_inp = "dummy";
    bool use_amr_wind = false;
    if (node["amr_wind_inp"]) {
        amr_inp = node["amr_wind_inp"].as<std::string>();
        use_amr_wind = true;
    }

#ifdef EXAWIND_HAS_STD_FILESYSTEM
    std::filesystem::path fpath_amr_inp(amr_inp);
    const std::string amr_log =
        fpath_amr_inp.replace_extension(".log").string();
#else
    const std::string amr_log = "amr-wind.log";
#endif
    std::ofstream out;

    if (use_amr_wind) {
        out.open(amr_log);
    }

    const auto nalu_inps = node["nalu_wind_inp"].as<std::vector<std::string>>();
    const int num_nwsolvers = nalu_inps.size();
    if (num_nwind_ranks < num_nwsolvers) {
        throw std::runtime_error(
            "Number of Nalu-Wind ranks is less than the number of Nalu-Wind "
            "solvers. Please have at least one rank per solver.");
    }
    const int ranks_per_nw_solver = num_nwind_ranks / num_nwsolvers;
    std::vector<int> num_nw_solver_ranks(num_nwsolvers, ranks_per_nw_solver);
    const int remainder = num_nwind_ranks % num_nwsolvers;
    if (remainder != 0) {
        std::fill(
            num_nw_solver_ranks.begin() + num_nwsolvers - remainder,
            num_nw_solver_ranks.end(), ranks_per_nw_solver + 1);
    }

    if (!use_amr_wind) {
        num_awind_ranks = 0;
    }
    
    if (num_awind_ranks + num_nwind_ranks < psize) {
        if (prank == 0)
            throw std::runtime_error(
                "Abort: using fewer ranks than available ranks: MPI "
                "size = " +
                std::to_string(psize) + "; Num ranks used = " +
                std::to_string(num_awind_ranks + num_nwind_ranks));
    }

    MPI_Comm amr_comm = use_amr_wind ? exawind::create_subcomm(MPI_COMM_WORLD, num_awind_ranks, 0) : MPI_COMM_NULL;

    std::vector<MPI_Comm> nalu_comms;
    int start = psize - num_nwind_ranks;
    for (const auto& nr : num_nw_solver_ranks) {
        nalu_comms.push_back(
            exawind::create_subcomm(MPI_COMM_WORLD, nr, start));
        start += nr;
    }

    exawind::OversetSimulation sim(MPI_COMM_WORLD);
    if (amr_comm != MPI_COMM_NULL) {
        sim.echo(
            "Initializing AMR-Wind on " + std::to_string(num_awind_ranks) +
            " MPI ranks");
        exawind::AMRWind::initialize(amr_comm, amr_inp, out);
    }
    sim.echo(
        "Initializing " + std::to_string(num_nwsolvers) +
        " Nalu-Wind solvers, equally partitioned on a total of " +
        std::to_string(num_nwind_ranks) + " MPI ranks");
    if (std::any_of(nalu_comms.begin(), nalu_comms.end(), [](const auto& comm) {
            return comm != MPI_COMM_NULL;
        })) {
        exawind::NaluWind::initialize();
    }

    {
        const auto nalu_vars = node["nalu_vars"].as<std::vector<std::string>>();
        const int num_timesteps = node["num_timesteps"].as<int>();
        for (int i = 0; i < num_nwsolvers; i++) {
            if (nalu_comms.at(i) != MPI_COMM_NULL)
                sim.register_solver<exawind::NaluWind>(
                    nalu_comms.at(i), nalu_inps.at(i), nalu_vars);
        }

        if (amr_comm != MPI_COMM_NULL) {
            const auto amr_cvars =
                node["amr_cell_vars"].as<std::vector<std::string>>();
            const auto amr_nvars =
                node["amr_node_vars"].as<std::vector<std::string>>();
    
            sim.register_solver<exawind::AMRWind>(amr_cvars, amr_nvars);
        }

        sim.echo("Initializing overset simulation");
        sim.initialize();
        sim.echo("Initialization successful");
        sim.run_timesteps(num_timesteps);
    }

    if (amr_comm != MPI_COMM_NULL) {
        out.close();
        exawind::AMRWind::finalize();
    }
    if (std::any_of(nalu_comms.begin(), nalu_comms.end(), [](const auto& comm) {
            return comm != MPI_COMM_NULL;
        })) {
        exawind::NaluWind::finalize();
    }
    MPI_Finalize();

    return 0;
}
