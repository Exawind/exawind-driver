#include "AMRWind.h"
#include "NaluWind.h"
#include "OversetSimulation.h"
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

    exawind::OversetSimulation sim(MPI_COMM_WORLD);

    {
        const auto nalu_vars = node["nalu_vars"].as<std::vector<std::string>>();
        const auto amr_cvars =
            node["amr_cell_vars"].as<std::vector<std::string>>();
        const auto amr_nvars =
            node["amr_node_vars"].as<std::vector<std::string>>();
        const int num_timesteps = node["num_timesteps"].as<int>();

        sim.register_solver<exawind::NaluWind>(
            MPI_COMM_WORLD, nalu_inp, nalu_vars);
        sim.register_solver<exawind::AMRWind>(amr_cvars, amr_nvars);

        sim.echo("Initializing overset simulation");
        sim.initialize();
        sim.echo("Initialization successful");

        sim.run_timesteps(num_timesteps);
    }

    exawind::AMRWind::finalize();
    exawind::NaluWind::finalize();
    MPI_Finalize();

    return 0;
}
