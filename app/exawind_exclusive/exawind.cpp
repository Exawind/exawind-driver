#include "AMRWind.h"
#include "NaluWind.h"
#include "OversetSimulation.h"
#include "mpi.h"
#include <filesystem>

#include "yaml-cpp/yaml.h"
#include "tioga.h"

int main(int argc, char** argv)
{
    if (argc != 2) {
        throw std::runtime_error("Usage: nalu_amr <inpfile>");
    }

    MPI_Init(&argc, &argv);
    int psize, prank;
    MPI_Comm_size(MPI_COMM_WORLD, &psize);
    MPI_Comm_rank(MPI_COMM_WORLD, &prank);

    // Temporarily only testing 2 MPI ranks
    if (psize != 2) {
        printf("This application is meant to be run with 2 processes.\n");
        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
    }

    const int num_nalu_ranks = 1;
    const int num_amr_ranks = psize - num_nalu_ranks;

    int amr_range[1][3] = {0, num_amr_ranks - 1, 1};
    int nalu_range[1][3] = {num_amr_ranks, psize - 1, 1};

    MPI_Group world_group, nalu_group, amr_group;
    MPI_Comm_group(MPI_COMM_WORLD, &world_group);
    MPI_Group_range_incl(world_group, 1, amr_range, &amr_group);
    MPI_Group_range_incl(world_group, 1, nalu_range, &nalu_group);

    MPI_Comm amr_comm, nalu_comm;
    MPI_Comm_create(MPI_COMM_WORLD, amr_group, &amr_comm);
    MPI_Comm_create(MPI_COMM_WORLD, nalu_group, &nalu_comm);

    const std::string inpfile(argv[1]);
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
        "Initializing AMR-Wind on " + std::to_string(num_amr_ranks) +
        " MPI ranks");
    if (amr_comm != MPI_COMM_NULL)
        exawind::AMRWind::initialize(amr_comm, amr_inp, out);
    sim.echo(
        "Initializing Nalu-Wind on " + std::to_string(num_nalu_ranks) +
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
