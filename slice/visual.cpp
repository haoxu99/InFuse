#include <filesystem>
#include "visual.h"
void Visual::generatePatchOBJ(const string& _file_name, const string& output_folder)
{
    ifstream ifile2(_file_name + ".txt");
    if (!ifile2.is_open()) {
        std::cout << "can not open " << _file_name + ".txt" << std::endl;
        return;
    }

    // Create output directories.
    std::filesystem::create_directories(output_folder.c_str());

    // Use fixed colors for the first four patches, then random colors.
    // #B0F3CD, #FFD3B6, #FFB3BB, #F38181
    const double preset_colors[][3] = {
        { 0xB0 / 255.0, 0xF3 / 255.0, 0xCD / 255.0 },
        { 0xFF / 255.0, 0xD3 / 255.0, 0xB6 / 255.0 },
        { 0xFF / 255.0, 0xB3 / 255.0, 0xBB / 255.0 },
        { 0xF3 / 255.0, 0x81 / 255.0, 0x81 / 255.0 }
    };
    const int num_preset = 4;

    float radius = 0.2;
    srand((int)time(0));

    int num_block;
    ifile2 >> num_block;
    for (int i = 0; i < num_block; i++)
    {
        General_Mesh mesh1;
        if (i < num_preset) {
            mesh1.r = preset_colors[i][0];
            mesh1.g = preset_colors[i][1];
            mesh1.b = preset_colors[i][2];
        } else {
            mesh1.r = rand() / double(RAND_MAX);
            mesh1.g = rand() / double(RAND_MAX);
            mesh1.b = rand() / double(RAND_MAX);
        }
        std::vector<Vec3> points;

        int num_seg;
        ifile2 >> num_seg;
        for (int j = 0; j < num_seg; j++)
        {
            int num_points;
            ifile2 >> num_points;
            for (int k = 0; k < num_points; k++)
            {
                Vec3 the_point;
                ifile2 >> the_point.m_x >> the_point.m_y >> the_point.m_z;
                points.push_back(the_point);
            }
            insert_Line_Cube(mesh1, points, radius);
            points.clear();
        }

        string patch_file = output_folder + "/patch_" + to_string(i) + ".obj";
        mesh1.genResultMesh(patch_file.c_str());
        std::cout << "Output: " << patch_file << std::endl;
    }
    ifile2.close();
}

void Visual::insert_Line_Cube(General_Mesh& mesh, std::vector<Vec3> points, float radius)
{
    if (points.size() < 2)
        return;

    for (int i = 0; i < points.size() - 1; i++)
    {
        Vec3 v0 = points[i];
        Vec3 v1 = points[i + 1];

        Vec3 t_dir = v1 - v0;

        Vec3 scale(radius, radius / 2, t_dir.Length());
        Vec3 trans = (v0 + v1) / 2;

        vector<Vec3> c_vertex = mesh.meshScale(scale, mesh.cube_vertex);
        c_vertex = mesh.meshRotate(t_dir, c_vertex);
        c_vertex = mesh.meshTrans(trans, c_vertex);
        mesh.insert(c_vertex, mesh.cube_faces);
    }
}
