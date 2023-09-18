#include "AMRWind.h"
#include "NaluWind.h"
#include "OversetSimulation.h"
#include "MPIUtilities.h"
#include "mpi.h"
#include "yaml-editor.h"
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

std::string
replace_extension(const std::string& filepath, const std::string& newExt)
{
    size_t lastDotPos = filepath.find_last_of(".");

    if (lastDotPos != std::string::npos && lastDotPos != 0) {
        return filepath.substr(0, lastDotPos) + newExt;
    } else {
        return filepath + newExt;
    }
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
    const std::string amr_log = replace_extension(amr_inp, ".log");
#endif
    std::ofstream out;

    YAML::Node nalu_node = node["nalu_wind_inp"];
    // make sure it is a list for now
    assert(nalu_node.IsSequence());
    const int num_nwsolvers = nalu_node.size();
    if (num_nwind_ranks < num_nwsolvers) {
        throw std::runtime_error(
            "Number of Nalu-Wind ranks is less than the number of Nalu-Wind "
            "solvers. Please have at least one rank per solver.");
    }
    std::vector<int> num_nw_solver_ranks;
    if (node["nalu_wind_procs"]) {
        num_nw_solver_ranks = node["nalu_wind_procs"].as<std::vector<int>>();
        if (num_nw_solver_ranks.size() != num_nwsolvers) {
            throw std::runtime_error(
                "Number of Nalu-Wind rank specifications is less than the "
                " number of Nalu-Wind solvers. Please have one rank count "
                "specification per solver");
        }
        const int tot_num_nw_ranks = std::accumulate(
            num_nw_solver_ranks.begin(), num_nw_solver_ranks.end(), 0);
        if (tot_num_nw_ranks != num_nwind_ranks) {
            throw std::runtime_error(
                "Total number of Nalu-Wind ranks does not "
                "match that given in the command line. Please ensure "
                "they match");
        }
    } else {
        const int ranks_per_nw_solver = num_nwind_ranks / num_nwsolvers;
        num_nw_solver_ranks =
            std::vector<int>(num_nwsolvers, ranks_per_nw_solver);
        const int remainder = num_nwind_ranks % num_nwsolvers;
        if (remainder != 0) {
            std::fill(
                num_nw_solver_ranks.begin() + num_nwsolvers - remainder,
                num_nw_solver_ranks.end(), ranks_per_nw_solver + 1);
        }
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

    MPI_Comm amr_comm =
        use_amr_wind
            ? exawind::create_subcomm(MPI_COMM_WORLD, num_awind_ranks, 0)
            : MPI_COMM_NULL;

    std::vector<MPI_Comm> nalu_comms;
    std::vector<int> nalu_start_rank;
    int start = psize - num_nwind_ranks;
    for (const auto& nr : num_nw_solver_ranks) {
        nalu_start_rank.push_back(start);
        nalu_comms.push_back(
            exawind::create_subcomm(MPI_COMM_WORLD, nr, start));
        start += nr;
    }

    exawind::OversetSimulation sim(MPI_COMM_WORLD);
    if (amr_comm != MPI_COMM_NULL) {
        sim.echo(
            "Initializing AMR-Wind on " + std::to_string(num_awind_ranks) +
            " MPI ranks");
        out.open(amr_log);
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
    sim.set_nw_start_rank(nalu_start_rank);

    const auto nalu_vars = node["nalu_vars"].as<std::vector<std::string>>();
    const int num_timesteps = node["num_timesteps"].as<int>();
    const int additional_picard_its =
        node["additional_picard_iterations"]
            ? node["additional_picard_iterations"].as<int>()
            : 0;
    const int nonlinear_its = node["nonlinear_iterations"]
                                  ? node["nonlinear_iterations"].as<int>()
                                  : 1;

    const YAML::Node yaml_replace_all = node["nalu_replace_all"];
    for (int i = 0; i < num_nwsolvers; i++) {
        if (nalu_comms.at(i) != MPI_COMM_NULL) {
            YAML::Node yaml_replace_instance;
            YAML::Node this_instance = nalu_node[i];

            std::string nalu_inpfile, logfile;
            bool write_final_yaml_to_disk = false;
            if (this_instance.IsMap()) {
                yaml_replace_instance = this_instance["replace"];
                nalu_inpfile =
                    this_instance["base_input_file"].as<std::string>();
                // deal with the logfile name
                if (this_instance["logfile"]) {
                    logfile = this_instance["logfile"].as<std::string>();
                } else {
                    logfile = exawind::NaluWind::change_file_name_suffix(
                        nalu_inpfile, ".log", i);
                }
                if (this_instance["write_final_yaml_to_disk"]) {
                    write_final_yaml_to_disk =
                        this_instance["write_final_yaml_to_disk"].as<bool>();
                }

            } else {
                nalu_inpfile = this_instance.as<std::string>();
                logfile = exawind::NaluWind::change_file_name_suffix(
                    nalu_inpfile, ".log");
            }

            YAML::Node nalu_yaml = YAML::LoadFile(nalu_inpfile);
            // replace in order so instance can overwrite all
            if (yaml_replace_all) {
                YEDIT::find_and_replace(nalu_yaml, yaml_replace_all);
            }
            if (yaml_replace_instance) {
                YEDIT::find_and_replace(nalu_yaml, yaml_replace_instance);
            }

            // only the first rank of the comm should write the file
            int comm_rank = -1;
            MPI_Comm_rank(nalu_comms.at(i), &comm_rank);
            if (write_final_yaml_to_disk && comm_rank == 0) {
                auto new_ifile_name =
                    exawind::NaluWind::change_file_name_suffix(
                        logfile, ".yaml");
                std::ofstream fout(new_ifile_name);
                fout << nalu_yaml;
                fout.close();
            }

            sim.register_solver<exawind::NaluWind>(
                i + 1, nalu_comms.at(i), nalu_yaml, logfile, nalu_vars);
        }
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
    sim.run_timesteps(additional_picard_its, nonlinear_its, num_timesteps);
    sim.delete_solvers();

    if (amr_comm != MPI_COMM_NULL) {
        exawind::AMRWind::finalize();
        out.close();
    }
    if (std::any_of(nalu_comms.begin(), nalu_comms.end(), [](const auto& comm) {
            return comm != MPI_COMM_NULL;
        })) {
        exawind::NaluWind::finalize();
    }
    MPI_Finalize();

    return 0;
}
