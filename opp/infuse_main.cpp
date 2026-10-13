#include "data.h"
#include "fusion.h"
#include "input.h"
#include "layer_graph.h"
#include "Merge_Graph.h"
#include "TRA_Graph.h"
#include "../slice/visual.h"
#include <algorithm>
#include <chrono>
#include <cctype>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace fs = std::filesystem;
std::string file_name;

namespace {
struct Options {
    fs::path input;
    fs::path output = "results";
    fs::path config;
    fs::path templates;
    std::string mode = "shell";
};

void usage() {
    std::cout << "Usage: infuse --input <ASCII STL file or directory> [options]\n"
              << "  --output <directory>   Output root (default: results)\n"
              << "  --mode shell|fusion    Processing mode (default: shell)\n"
              << "  --config <file>        Slicing configuration\n"
              << "  --templates <dir>      Directory containing G-code header/footer\n"
              << "  --help                 Show this message\n";
}

Options parse(int argc, char** argv) {
    Options options;
    const fs::path executable_dir = fs::absolute(argv[0]).parent_path();
    options.config = fs::exists("config.ini") ? fs::path("config.ini") : executable_dir / "config.ini";
    options.templates = fs::exists("slice/gcode_header.txt") ? fs::path("slice") : executable_dir / "slice";
    for (int i = 1; i < argc; ++i) {
        const std::string key = argv[i];
        if (i + 1 >= argc) throw std::runtime_error("Missing value for " + key);
        const std::string value = argv[++i];
        if (key == "--input") options.input = value;
        else if (key == "--output") options.output = value;
        else if (key == "--mode") options.mode = value;
        else if (key == "--config") options.config = value;
        else if (key == "--templates") options.templates = value;
        else throw std::runtime_error("Unknown option: " + key);
    }
    if (options.input.empty()) throw std::runtime_error("--input is required.");
    if (options.mode != "shell" && options.mode != "fusion") throw std::runtime_error("--mode must be shell or fusion.");
    options.input = fs::absolute(options.input);
    options.output = fs::absolute(options.output);
    options.config = fs::absolute(options.config);
    options.templates = fs::absolute(options.templates);
    if (!fs::is_regular_file(options.config)) throw std::runtime_error("Configuration file does not exist: " + options.config.string());
    for (const auto* name : {"gcode_header.txt", "gcode_footer.txt"}) {
        if (!fs::is_regular_file(options.templates / name)) throw std::runtime_error("Missing G-code template: " + (options.templates / name).string());
    }
    return options;
}

bool is_stl(const fs::path& path) {
    std::string extension = path.extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return extension == ".stl";
}

std::vector<fs::path> models(const fs::path& input) {
    std::vector<fs::path> paths;
    if (fs::is_regular_file(input) && is_stl(input)) paths.push_back(input);
    else if (fs::is_directory(input)) {
        for (const auto& entry : fs::directory_iterator(input)) {
            if (entry.is_regular_file() && is_stl(entry.path())) paths.push_back(entry.path());
        }
    }
    std::sort(paths.begin(), paths.end());
    if (paths.empty()) throw std::runtime_error("No STL files found: " + input.string());
    return paths;
}

void generate_paths(Data& data, const std::vector<std::vector<int>>& patches,
                    std::vector<std::pair<int, int>>& dependencies,
                    std::vector<std::pair<int, int>>& constraints) {
    TRA_Graph travel_graph(data, patches, dependencies, constraints);
    travel_graph.BuildTRAGraph();
    travel_graph.generate_dot();
    travel_graph.TopologicalSort_BeamSearch();
    travel_graph.GeneratePath();
}

void process(const fs::path& model, const Options& options) {
    file_name = model.stem().string();
    const fs::path model_output = options.output / file_name;
    // Reject existing output: stale block files would silently change subsequent runs.
    if (fs::exists(model_output)) throw std::runtime_error("Output already exists; choose a new --output directory: " + model_output.string());
    fs::create_directories(model_output / "work" / "slice");
    fs::create_directories(model_output / "Blocks_files");
    fs::create_directories(model_output / "patches");
    for (const auto* name : {"gcode_header.txt", "gcode_footer.txt"}) {
        fs::copy_file(options.templates / name, model_output / "work" / "slice" / name);
    }
    fs::current_path(model_output / "work");
    const std::string model_path = model.string();
    const std::string config_path = options.config.string();
    nozzle printer_nozzle{4.5, 1.0, 8.0, 2.5, 0.6};
    if (options.mode == "shell") {
        Input input;
        input.config_path = config_path.c_str();
        input.model_path = model_path.c_str();
        input.DefaultPrintingDirectionSlice(0);
        Data data;
        data.ReadData("slice_layers.txt");
        Layer_Graph layer_graph(data);
        layer_graph.BuildLayerGraph(printer_nozzle);
        layer_graph.GetInitialOPP();
        Merge_Graph merge_graph(data, layer_graph.dependency_edges, layer_graph.constraint_edges);
        merge_graph.Merge();
        generate_paths(data, merge_graph.initial_opp_info, layer_graph.dependency_edges, layer_graph.constraint_edges);
    } else {
        Input outer, inner;
        outer.config_path = inner.config_path = config_path.c_str();
        outer.model_path = inner.model_path = model_path.c_str();
        outer.DefaultPrintingDirectionSlice(1);
        Data outer_data, inner_data;
        outer_data.ReadData("slice_layers_outer.txt");
        inner.DefaultPrintingDirectionSlice(2);
        inner_data.ReadData("slice_layers_inner.txt");
        Fusion fusion(outer_data, inner_data);
        fusion.BuildGraph(printer_nozzle);
        fusion.Fusion_Graph(printer_nozzle);
        Merge_Graph merge_graph(fusion.data_fusion[0], fusion.dependency_edges, fusion.constraint_edges);
        merge_graph.Merge();
        generate_paths(fusion.data_fusion[0], merge_graph.initial_opp_info, fusion.dependency_edges, fusion.constraint_edges);
    }
    We_update(1);
    Visual visual;
    visual.generatePatchOBJ(file_name + "_paths_step", (model_output / "patches").string());
}

void reset_slicer() {
    auto& katana = Katana::Instance();
    katana.layers.clear();
    katana.triangles.clear();
    katana.vertices.clear();
    katana.stl.f_triangles.clear();
    katana.mappint_para_point_index.clear();
    katana.min_z = 0;
}
} // namespace

int main(int argc, char** argv) {
    if (argc == 2 && std::string(argv[1]) == "--help") { usage(); return 0; }
    const fs::path launch_dir = fs::current_path();
    try {
        const Options options = parse(argc, argv);
        const auto paths = models(options.input);
        int failures = 0;
        for (std::size_t i = 0; i < paths.size(); ++i) {
            std::cout << "[" << i + 1 << "/" << paths.size() << "] Processing: " << paths[i].filename().string() << std::endl;
            const auto start = std::chrono::steady_clock::now();
            try {
                reset_slicer();
                process(paths[i], options);
                const auto seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
                std::cout << "Completed in " << seconds << " s." << std::endl;
            } catch (const std::exception& error) {
                ++failures;
                std::cerr << "Failed: " << paths[i].filename().string() << ": " << error.what() << std::endl;
            }
            fs::current_path(launch_dir);
            reset_slicer();
        }
        std::cout << "Processed " << paths.size() << " models; failures: " << failures << std::endl;
        return failures == 0 ? 0 : 1;
    } catch (const std::exception& error) {
        fs::current_path(launch_dir);
        std::cerr << "Error: " << error.what() << '\n';
        usage();
        return 1;
    }
}
