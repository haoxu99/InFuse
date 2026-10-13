/*
 * katana, an experimental stl slicer written in C++0x
 *
 * Usage: katana <input.stl> <output.gcode>
 *
 * This program loads a given .stl (stereolithography data, actually triangle data) file
 * and generates a .gcode (RepRap machine instructions) file that can be printed on a RepRap
 * machine.
 *
 * This process is mostly referred as "slicing", as the printers make objects layer by layer,
 * so the 3d model has to be virtually sliced into thin layers. This however is only the first
 * step, after which perimeters and a crosshatch filling pattern are computed for every layer.
 *
 * The .stl file should describe a clean mesh that is a 2-manifold. That means:
 * * every triangle should touch exactly three triangles along its three edges
 * * on every edge, two triangles share exactly two vertices
 * * every triangle has some positive surface area
 *  a triangle may touch further triangles at its vertices.
 *
 *  currently, many checks are disabled to accept objects breaking any of this rules.
 *  the results however are undefined, albeit sometimes usable.
 *
 * in contrast to other slicers, this one does not need:
 * * triangles don't have to carry valid normals
 * * triangle vertices don't need a defined clockwise or counterclockwise order
 *
 * Based on Schlizzer written by Paul Geisler in 2012.
 * https://github.com/dronus/Schlizzer
 */
#include <stdio.h>
#include <assert.h>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <array>
#include <math.h>
#include <exception>
#include <fstream>

#include "datastructures.h"
#include "config.h"
#include "stl.h"
#include "gcode.h"
#include "slicer.h"
#include "katana.h"
#include"GeneralMesh.h"
#include<string>
#include<ctime>
#include<iomanip>
#include "visual.h"
#include "clipper.hpp"

#define use_int32
using namespace std;

using namespace ClipperLib;

typedef unsigned char boolean;

struct InjectionLayer {
    int type;
    Vec3 center;
};
vector<vector<InjectionLayer>> layer_type;// Layer types and injection centers, indexed by patch and layer.
vector<vector<double>> layer_volume;// Layer volumes: contour area times layer height; -1 means no injection volume.
double calculate_volume(Paths solution);// Compute the layer volume.
vector<vector<Paths>> inner_solution;// Innermost contours for each layer.

double distance(Vec3 a, Vec3 b);
double distance_2(Vec3 a, Vec3 b);
void adjust_gcode(vector<vector<vector<Vec3>>>& ALL_OPP);
void gcode_generation(vector<vector<vector<vector<Vec3>>>> opp);
void gap_optimization(vector<vector<Vec3>>& opp, Paths solution, vector<vector<vector<Vec3>>> ALL_OPP);

void skip_optimization(vector<vector<vector<vector<Vec3>>>>& opp);
vector<vector<Vec3>> Infill_zigzag(vector<vector<Vec3>> contour);
bool JudgePointInside(vector<vector<Point2d>> points, Point2d p);
vector<vector<vector<vector<Vec3>>>> Clipper_offset(vector<vector<vector<Vec3>>> ALL_OPP);
void gcode_header(ofstream& gcode);
void gcode_footer(ofstream& gcode);
void OutputPaths(vector<vector<vector<Vec3>>> opp, const std::string& file_name);

void adjust_gcode(vector<vector<vector<Vec3>>>& ALL_OPP,double enlarge_mul)
{
    // Center the model on the printing platform.
    // Model center.
    Vec3 center_point;
    center_point.m_x = center_point.m_y = 0;
    double min_z = 999999;
    int cont_point = 0;
    // Compute the model center.
    for (int i = 0; i < ALL_OPP.size(); i++) {
        for (int j = 0; j < ALL_OPP[i].size(); j++) {
            for (int k = 0; k < ALL_OPP[i][j].size(); k++)
            {
                center_point.m_x += ALL_OPP[i][j][k].m_x;
                center_point.m_y += ALL_OPP[i][j][k].m_y;
                cont_point++;
                if (ALL_OPP[i][j][k].m_z < min_z)
                    min_z = ALL_OPP[i][j][k].m_z;
            }
        }
    }

    center_point.m_x /= cont_point;
    center_point.m_y /= cont_point;

    // Move the model center to the origin.
    for (int i = 0; i < ALL_OPP.size(); i++) {
        for (int j = 0; j < ALL_OPP[i].size(); j++) {
            for (int k = 0; k < ALL_OPP[i][j].size(); k++)
            {
                ALL_OPP[i][j][k].m_x -= center_point.m_x;
                ALL_OPP[i][j][k].m_y -= center_point.m_y;
                ALL_OPP[i][j][k].m_z -= min_z;
            }
            // Move injection centers to the origin.
            if (layer_type[i][j].type == 1) {
                layer_type[i][j].center.m_x -= center_point.m_x;
                layer_type[i][j].center.m_y -= center_point.m_y;
                layer_type[i][j].center.m_z -= min_z;
            }
        }
    }

    // Move the model to the platform center.
    for (int i = 0; i < ALL_OPP.size(); i++) {
        for (int j = 0; j < ALL_OPP[i].size(); j++) {
            for (int k = 0; k < ALL_OPP[i][j].size(); k++)
            {
                ALL_OPP[i][j][k].m_x += 150;
                ALL_OPP[i][j][k].m_y += 150;
                ALL_OPP[i][j][k].m_z += 0.2;
            }
            // Move injection centers to the platform center.
            if (layer_type[i][j].type == 1) {   // Adjust this offset for the target hardware.
                layer_type[i][j].center.m_x += 150;
                layer_type[i][j].center.m_y += 150;
                layer_type[i][j].center.m_z += 0.2;
            }
        }
    }

}

//We update this progarm, function include: We_update() is main function, Clipper_offset() for the contour offset, gcode_generation() for generation gcode file.
void We_update(int step) {

    // Reset global state before processing the next model.
    layer_type.clear();
    layer_volume.clear();
    inner_solution.clear();

    clock_t start_time, end_time;
    start_time = clock();
    bool open_adjust_z = false;

    int num_block = 0;
    // Read the printing paths.
    ifstream ifile;
    vector<vector<vector<Vec3>>> ALL_OPP;
    vector<vector<vector<double>>> ALL_H;
    vector<Vec3> last_block_start_point;
    while (true) {
        int cont_step = -1;
        string str_num = to_string(num_block);
        ifile.open("..//Blocks_files/" + file_name + "_block" + str_num + ".txt");
        if (!ifile.is_open())
            break;
        num_block++;
        vector<vector<Vec3>> temp_v;
        vector<vector<double>> temp_H;
        ALL_OPP.push_back(temp_v);
        ALL_H.push_back(temp_H);
        int num_seg;
        ifile >> num_seg;
        for (int j = 0; j < num_seg; j++)
        {

            cont_step = (cont_step + 1) % step;

            if (cont_step == 0) {
                vector<Vec3> temp_vv;
                vector<double> temp_hh;
                ALL_OPP[ALL_OPP.size() - 1].push_back(temp_vv);
                ALL_H[ALL_H.size() - 1].push_back(temp_hh);
            }
            int num_points;
            ifile >> num_points;
            for (int k = 0; k < num_points; k++)
            {
                Vec3 the_point;
                double the_H;
                if (cont_step == 0) {
                    ifile >> the_point.m_x >> the_point.m_y >> the_point.m_z >> the_H;
                    ALL_OPP[ALL_OPP.size() - 1][ALL_OPP[ALL_OPP.size() - 1].size() - 1].push_back(the_point);
                    ALL_H[ALL_H.size() - 1][ALL_H[ALL_H.size() - 1].size() - 1].push_back(the_H);
                    if (j == 0 && k == 0)
                        last_block_start_point.push_back(the_point);
                }
                else
                    ifile >> the_point.m_x >> the_point.m_y >> the_point.m_z >> the_H;
            }
        }
        ifile.close();

    }

    // Read injection types and centers from Injection_Order.txt.
    ifstream ifile_injection;
    ifile_injection.open("Injection_Order.txt");
    if (!ifile_injection.is_open()) {
        cout << "No injection file!" << endl;
    }
    int layer_num;
    ifile_injection >> layer_num;	// Read the number of layers.

    vector<InjectionLayer> layer_type_one;// Store layer types and injection centers in a flat sequence.
    for (int i = 0; i < layer_num; i++) {	// Read each layer.
        InjectionLayer buffer;	// Temporarily store layer information.
        ifile_injection >> buffer.type;	// Read the layer type.
        if (buffer.type == 1)	// Read the center for injection layers.
            ifile_injection >> buffer.center.m_x >> buffer.center.m_y >> buffer.center.m_z;

        layer_type_one.push_back(buffer);
    }
    ifile_injection.close();

    // Map the flat sequence to patch and layer indices.
    int layer_num2 = 0;
    for (int i = 0; i < ALL_OPP.size(); i++) {
        vector<InjectionLayer> layer_type_buffer;
        for (int j = 0; j < ALL_OPP[i].size(); j++) {
            layer_type_buffer.push_back(layer_type_one[layer_num2]);
            layer_num2++;
        }
        layer_type.push_back(layer_type_buffer);
        layer_type_buffer.clear();
    }

    // Normalize contour winding.
    for (int i = 0; i < ALL_OPP.size(); i++) {
        for (int j = 0; j < ALL_OPP[i].size(); j++) {
            double s = 0.0;
            int n = ALL_OPP[i][j].size();
            for (int k = 0; k < n; ++k) {
                int m = (k + 1) % n;// Wrap the final point back to the first point.
                s += (ALL_OPP[i][j][k].m_x * ALL_OPP[i][j][m].m_y - ALL_OPP[i][j][m].m_x * ALL_OPP[i][j][k].m_y);
            }
            // Use signed area to determine the contour winding.
            if (s > 0.0) {
                std::reverse(ALL_OPP[i][j].begin(), ALL_OPP[i][j].end());
            }
        }
    }

    //adjust gcode
    double enlarge_mul = 1;// Coordinate scale factor.
    adjust_gcode(ALL_OPP, enlarge_mul);

    vector<vector<vector<vector<Vec3>>>> opp = Clipper_offset(ALL_OPP);

    end_time = clock();
    cout << "&&&&&&& time of post processing: " << double(end_time - start_time) / CLOCKS_PER_SEC << "s &&&&&&&" << endl;
    gcode_generation(opp);
}

void gap_optimization(vector<vector<Vec3>>& opp, Paths solution, vector<vector<vector<Vec3>>> ALL_OPP) {
    Path way;
    Paths subj;
    Paths clip_up;
   Paths clip_down;
   double sh = Extrusion_width * 1000 * (1 - PathOverlap); //path width

    IntPoint p;
    Vec3 p_vec3;
    Point2d p_vec2;
    vector<Point2d> p_buffer1;
    vector<vector<Point2d>> p_buffer2;

    int patch_id, layer_id;// patch_id identifies the patch; layer_id identifies the layer.

    subj = solution;

    int count_iii = 0;

    for (int i = 0; i < ALL_OPP.size(); i++) {
        bool label = false;
        for (int j = 0; j < ALL_OPP[i].size(); j++) {
            count_iii++;
            if (ALL_OPP[i][j] == opp[0]) {
                patch_id = i;
                layer_id = j;
                label = true;
                break;
            }
        }
        if (label) break;
    }

    bool flag = false;
    for (int i = 0; i < ALL_OPP.size(); i++) {
        for (int j = 0; j < ALL_OPP[i].size(); j++) {

            // Upper opening.
            if ((abs(ALL_OPP[i][j][0].m_z - (opp[0][0].m_z + dh)) <= 0.1) && ((layer_type[patch_id][layer_id].type < 0 && layer_type[i][j].type < 0) || (layer_type[patch_id][layer_id].type >= 0 && layer_type[i][j].type >= 0))) {// Tolerate small rounding errors in z coordinates.

                for (int nn = 0; nn < inner_solution[i][j].size(); nn++) {
                    clip_up.push_back(inner_solution[i][j][nn]);
                }

                flag = true;

            }

            // Lower opening.
            if ((abs((ALL_OPP[i][j][0].m_z + dh) - opp[0][0].m_z) <= 0.1) && ((layer_type[patch_id][layer_id].type < 0 && layer_type[i][j].type < 0) || (layer_type[patch_id][layer_id].type >= 0 && layer_type[i][j].type >= 0))) {// Tolerate small rounding errors in z coordinates.
                for (int nn = 0; nn < ALL_OPP[i][j].size(); nn++) {
                    p_vec3 = ALL_OPP[i][j][nn];
                    p.X = p_vec3.m_x * 1000;
                    p.Y = p_vec3.m_y * 1000;
                    way.push_back(p);
                }
                flag = true;
                clip_down.push_back(way);
                way.clear();

            }

        }
    }

    if (flag && layer_type[patch_id][layer_id].type == 0) {
        Clipper Dif;
        Paths poly;
        Paths poly_1;

        Dif.AddPaths(subj, ptSubject, true);
        Dif.AddPaths(clip_up, ptClip, true);
        Dif.Execute(ctDifference, poly, pftNonZero, pftNonZero);

        Dif.Clear();
        Dif.AddPaths(subj, ptSubject, true);
        Dif.AddPaths(clip_down, ptClip, true);
        Dif.Execute(ctDifference, poly_1, pftNonZero, pftNonZero);

        Dif.Clear();
        Dif.AddPaths(poly, ptSubject, true);
        Dif.AddPaths(poly_1, ptClip, true);
        Dif.Execute(ctUnion, poly, pftNonZero, pftNonZero);

        if (!poly.empty()) {

            //draw polygons

            vector<vector<Vec3>> contour;
            vector<Vec3> contour_buffer;
            for (int i = 0; i < poly.size(); i++) {
                for (int j = 0; j < poly[i].size(); j++) {
                    p_vec3.m_x = double(poly[i][j].X) / 1000;
                    p_vec3.m_y = double(poly[i][j].Y) / 1000;
                    p_vec3.m_z = opp[0][0].m_z;
                    contour_buffer.push_back(p_vec3);
                }
                contour.push_back(contour_buffer);
                contour_buffer.clear();
            }

            // Insert contours into the patch.
            for (int j = 0; j < contour.size(); j++) {
                opp.push_back(contour[j]);
            }
            vector<vector<Vec3>> path_zigzag = Infill_zigzag(contour);
            for (int j = 0; j < path_zigzag.size(); j++) {
                opp.push_back(path_zigzag[j]);
            }
        }
    }

    if (layer_type[patch_id][layer_id].type >= 1) {

        if (layer_type[patch_id][layer_id].type == 1) {// Represent an injection layer by a single center point.
            vector<Vec3> injection_point;

            injection_point.push_back(layer_type[patch_id][layer_id].center);
            opp.push_back(injection_point);
        }
        vector<vector<Vec3>> contour;
        vector<Vec3> contour_buffer;
        for (int i = 0; i < subj.size(); i++) {
            for (int j = 0; j < subj[i].size(); j++) {
                p_vec3.m_x = double(subj[i][j].X) / 1000;
                p_vec3.m_y = double(subj[i][j].Y) / 1000;
                p_vec3.m_z = opp[0][0].m_z;
                contour_buffer.push_back(p_vec3);
            }
            contour.push_back(contour_buffer);
            contour_buffer.clear();
        }

        vector<vector<Vec3>> path_zigzag = Infill_zigzag(contour);
        for (int j = 0; j < path_zigzag.size(); j++) {
            opp.push_back(path_zigzag[j]);
        }

    }

    // Compute each layer volume.
    if (layer_type[patch_id][layer_id].type == 0) {
        Clipper c;
        c.AddPaths(subj, ptSubject, true);
        c.AddPaths(clip_up, ptClip, true);
        Paths intersection_buffer;
        Paths intersection;
        c.Execute(ctIntersection, intersection_buffer, pftNonZero, pftNonZero);
        c.Clear();
        c.AddPaths(intersection_buffer, ptSubject, true);
        c.AddPaths(clip_down, ptClip, true);
        c.Execute(ctIntersection, intersection, pftNonZero, pftNonZero);
        c.Clear();
        if (!intersection.empty()) {
            layer_volume[patch_id][layer_id] = calculate_volume(intersection);
        }
    }

    // Generate internal support by intersecting the upper and current layers.
    if (GapOptimization_support) {
        Clipper c;
        c.AddPaths(subj, ptSubject, true);
        c.AddPaths(clip_up, ptClip, true);
        Paths intersection;
        c.Execute(ctIntersection, intersection, pftNonZero, pftNonZero);
        c.Clear();
        if (!intersection.empty()) {
            ClipperOffset co;
            co.AddPaths(intersection, jtRound, etClosedPolygon);
            co.Execute(intersection, sh);
            inner_solution[patch_id][layer_id] = intersection;
        }
    }

    // Seal the highest contour.

}

vector<vector<Vec3>> Infill_zigzag(vector<vector<Vec3>> contour) {
    vector<Vec3> path;  // Zigzag path points.
    vector<vector<Vec3>> result;  // Resulting paths.
    if (contour.empty()) {
        return result;  // Return no paths for empty input.
    }

    // Find the minimum and maximum y coordinates.
    double minY = contour[0][0].m_y;
    double maxY = contour[0][0].m_y;
    double minX = contour[0][0].m_x;
    double maxX = contour[0][0].m_x;

    for (int i = 0; i < contour.size(); i++) {
        for (const auto& point : contour[i]) {
            if (point.m_y < minY) minY = point.m_y;
            if (point.m_y > maxY) maxY = point.m_y;
            if (point.m_x < minX) minX = point.m_x;
            if (point.m_x > maxX) maxX = point.m_x;
        }
    }

    double step = Extrusion_width;  // Fill spacing equals the extrusion width.

    // Convert contour representation.
    vector<Point2d> contour_buffer1;
    vector<vector<Point2d>> contour_buffer;
    for (int i = 0; i < contour.size(); i++) {
        for (const auto& point : contour[i]) {
            contour_buffer1.push_back(Point2d(point.m_x, point.m_y));
        }
        contour_buffer.push_back(contour_buffer1);
        contour_buffer1.clear();
    }
    // Generate alternating zigzag rows.
    int flag111 = 1;  // 1 moves left to right; -1 moves right to left.
    for (double y = minY; y <= maxY; y += step) {
        vector<Vec3> currentRow;  // Points in the current row.
        if (flag111 == 1) {
            flag111 = -1;
            // Fill from left to right.
            for (double x = minX; x <= maxX; x += step / 2) {
                // Check whether the point is inside the contour.
                if (JudgePointInside(contour_buffer, Point2d(x, y))) {
                    currentRow.push_back(Vec3{ x, y, contour[0][0].m_z}); // Use the contour z coordinate.
                }
                else if (!currentRow.empty()) {
                    result.push_back(currentRow);
                    currentRow.clear();
                }
            }
        }
        else if (flag111 == -1) {
            flag111 = 1;
            // Fill from right to left.
            for (double x = maxX; x >= minX; x -= step / 2) {
                // Check whether the point is inside the contour.
                if (JudgePointInside(contour_buffer, Point2d(x, y))) {
                    currentRow.push_back(Vec3{ x, y, contour[0][0].m_z}); // Use the contour z coordinate.
                }
                else if (!currentRow.empty()) {
                    result.push_back(currentRow);
                    currentRow.clear();
                }
            }
        }
        // Append the current row.
        if (!currentRow.empty()) {
            result.push_back(currentRow);
        }
    }

    for (int i = 0; i < result.size(); i++) {
        while (result[i].size() > 2) {
            result[i].erase(result[i].begin() + 1);
        }
    }
    // Remove single-point paths.
    for (int i = 0; i < result.size(); i++) {
        if (result[i].size() == 1) {
            result.erase(result.begin() + i);
            i--;
        }
    }

    // Optimize travel between paths.

    for (int i = 0; i < result.size() - 1; i++) {
        double dis = 9999;
        double index = i + 1;
        for (int j = i + 1; j < result.size(); j++) {
            if (distance_2(result[i][1], result[j][0]) < dis) {
                dis = distance_2(result[i][1], result[j][0]);
                index = j;
            }
        }
        if (index != i + 1) {
            vector<Vec3> temp = result[index];
            result[index] = result[i + 1];
            result[i + 1] = temp;
        }
    }

    return result;  // Return the generated paths.
}

bool JudgePointInside(vector<vector<Point2d>> points, Point2d p)
{
    double minx, maxx, miny, maxy;
    int find = 0;
    if (find == 0) {
        minx = MAX_D;
        maxx = MIN_D;
        miny = MAX_D;
        maxy = MIN_D;
        for (int i = 0; i < points.size(); i++) {
            for (int j = 0; j < points[i].size(); j++) {
                minx = std::min(points[i][j].x, minx);
                maxx = std::max(points[i][j].x, maxx);
                miny = std::min(points[i][j].y, miny);
                maxy = std::max(points[i][j].y, maxy);
            }
        }
        find = 1;
    }
    if (p.x < minx || p.x > maxx || p.y < miny || p.y > maxy) return false;
    int i, j, c = 0;
    for (int m = 0; m < points.size(); m++) {
        for (i = 0, j = points[m].size() - 1; i < points[m].size(); j = i++) {
            if (((points[m][i].y > p.y) != (points[m][j].y > p.y)))
                if (p.x < (points[m][j].x - points[m][i].x) * (p.y - points[m][i].y) / (points[m][j].y - points[m][i].y) + points[m][i].x)
                    c = !c;
        }
    }
    return c;
}

vector<vector<vector<vector<Vec3>>>> Clipper_offset(vector<vector<vector<Vec3>>> ALL_OPP) {
    vector<vector<vector<vector<Vec3>>>> opp;
    vector<vector<vector<Vec3>>> opp_buffer3;
    vector<vector<Vec3>> opp_buffer2;
    vector<Vec3> opp_buffer1;

    Paths solution;
    IntPoint p;
    Vec3 p_vec3;
    vector<Vec3> p_buffer1;

    int sh = Extrusion_width * 1000 * (1 - PathOverlap);

    for (int i = 0; i < ALL_OPP.size(); i++) {
        for (int j = 0; j < ALL_OPP[i].size(); j++) {
            for (int m = 0; m < ALL_OPP[i][j].size(); m++) {
                opp_buffer1.push_back(ALL_OPP[i][j][m]);
            }
            opp_buffer2.push_back(opp_buffer1);
            opp_buffer1.clear();
            opp_buffer3.push_back(opp_buffer2);
            opp_buffer2.clear();
        }
        opp.push_back(opp_buffer3);
        opp_buffer3.clear();
    }

    // Initialize layer volumes.

    for (int i = 0; i < opp.size(); i++) {
        vector<double> volume_buffer;
        for (int j = 0; j < opp[i].size(); j++) {
            volume_buffer.push_back(-1);
        }
        layer_volume.push_back(volume_buffer);
        volume_buffer.clear();
    }

    //Contour offset
    for (int i = 0; i < ALL_OPP.size(); i++) {

        vector<Paths> inner_solution_buffer;
        for (int j = 0; j < ALL_OPP[i].size(); j++) {
            Path way;
            for (int m = 0; m < ALL_OPP[i][j].size(); m++) {
                p.X = ALL_OPP[i][j][m].m_x * 1000;
                p.Y = ALL_OPP[i][j][m].m_y * 1000;
                way.push_back(p);
            }

            ClipperOffset co;
            co.AddPath(way, jtRound, etClosedPolygon);

            for (int a = 0; a < times; a++) {

                solution.clear();
                co.Execute(solution, -sh);
                for (int n1 = 0; n1 < solution.size(); n1++) {// Offsetting may produce multiple contours.

                    for (int n = 0; n < solution[n1].size(); n++) {
                        p_vec3.m_x = double(solution[n1][n].X) / 1000;
                        p_vec3.m_y = double(solution[n1][n].Y) / 1000;
                        p_vec3.m_z = ALL_OPP[i][j][n1].m_z;

                        p_buffer1.push_back(p_vec3);
                    }

                    opp[i][j].push_back(p_buffer1);
                    p_buffer1.clear();
                }

                co.Clear();
                co.AddPaths(solution, jtRound, etClosedPolygon);

            }
            //end offset
            // Store the innermost contours.
            inner_solution_buffer.push_back(solution);

        }
        inner_solution.push_back(inner_solution_buffer);
        inner_solution_buffer.clear();

    }

    // Optimize gaps bottom-up for self-supporting models and molds.
    if (!GapOptimization_support) {
        for (int i = 0; i < opp.size(); i++) {
            for (int j = 0; j < opp[i].size(); j++) {

               gap_optimization(opp[i][j], inner_solution[i][j], ALL_OPP);//opp[i][j] is recendly layer, solution is inner path of the recendly layer, ALL_OPP is the original contour.

            }
        }
    }
    // Optimize gaps top-down with support.
    else {
        for (int i = opp.size() - 1; i >= 0; i--) {
            for (int j = opp[i].size() - 1; j >= 0; j--) {

                gap_optimization(opp[i][j], inner_solution[i][j], ALL_OPP);//opp[i][j] is recendly layer, solution is inner path of the recendly layer, ALL_OPP is the original contour.

            }
        }
    }

    // Export layer volumes.
    double volume_total = 0;
    for (int i = 0; i < layer_volume.size(); i++) {
        for (int j = 0; j < layer_volume[i].size(); j++) {
            if(layer_volume[i][j] >= 0)
                volume_total += layer_volume[i][j];
        }
    }
    cout << "Total Volume:" << volume_total << endl;

    //Contour skip optimization
    skip_optimization(opp);

    return opp;
}

double calculate_volume(Paths solution) {
    double sh = (Extrusion_width / 2) * 1000;// Offset by half of the path width.

    ClipperOffset co;
    co.AddPaths(solution, jtRound, etClosedPolygon);
    solution.clear();
    co.Execute(solution, -sh);
    vector<vector<Point2d>> contour;
    vector<Point2d> contour_buffer;
    Point2d p;
    for (int i = 0; i < solution.size(); i++) {
        for (int j = 0; j < solution[i].size(); j++) {
            p.x = double(solution[i][j].X) / 1000;
            p.y = double(solution[i][j].Y) / 1000;
            contour_buffer.push_back(p);
        }
        contour.push_back(contour_buffer);
        contour_buffer.clear();
    }

    double res = 0;
    // Compute area.
    for (int i = 0; i < contour.size(); i++) {
        if (contour[i].size() < 3) {// Skip contours with fewer than three points.
            continue;
        }
        for (int j = 0, k = contour[i].size() - 1; j < contour[i].size(); j++) {
            res += (contour[i][k].x + contour[i][j].x) * (contour[i][k].y - contour[i][j].y);// Shoelace formula.
            k = j;
        }
    }
    // Export the result.

    return abs(res / 2) * dh;

}

//Skip optimization
void skip_optimization(vector<vector<vector<vector<Vec3>>>>& opp) {
    int index;
    double dis;
    Vec3 p_vec3;
    for (int i = 0; i < opp.size(); i++) {

        p_vec3 = opp[i][0][0][0];
        for (int j = 0; j < opp[i].size(); j++) {

            for (int m = 0; m < opp[i][j].size(); m++) {
                if (opp[i][j][m].size() > 2) {   // Exclude zigzag segments from contour optimization.
                    dis = 1000;   //Init minimum distance
                    index = 0;
                    if ((j == 0 && m == 0))
                        continue;
                    for (int n = 0; n < opp[i][j][m].size(); n++) {
                        double dis_buffer = distance(p_vec3, opp[i][j][m][n]);
                        if (dis_buffer <= dis) {
                            dis = dis_buffer;
                            index = n;
                        }
                    }
                    if (index != 0) {
                        for (int n = 0; n < index; n++) {
                            opp[i][j][m].push_back(opp[i][j][m][n]);
                        }
                        opp[i][j][m].erase(opp[i][j][m].begin(), opp[i][j][m].begin() + index);
                    }
                }
            }
        }
    }
}

void gcode_header(ofstream& gcode) {
    ifstream ifile;
    ifile.open("./slice/gcode_header.txt");
    // Check that the file opened.
    if (!ifile.is_open())
    {
        std::cout << "Cannot open gcode_header.txt!" << std::endl;
        return;
    }
    // Read the template.
    vector<string> lines;
    string line;
    while (getline(ifile, line)) {
        lines.push_back(line);
        line.clear();
    }
    // Write the template.
    for (int i = 0; i < lines.size(); i++) {
        gcode << lines[i] << endl;
    }

}

void gcode_footer(ofstream& gcode) {
    ifstream ifile;
    ifile.open("./slice/gcode_footer.txt");
    // Check that the file opened.
    if (!ifile.is_open())
    {
        std::cout << "Cannot open gcode_footer.txt!" << std::endl;
        return;
    }
    // Read the template.
    vector<string> lines;
    string line;
    while (getline(ifile, line)) {
        lines.push_back(line);
        line.clear();
    }
    // Write the template.
    for (int i = 0; i < lines.size(); i++) {
        gcode << lines[i] << endl;
    }
}

void gcode_generation(vector<vector<vector<vector<Vec3>>>> opp) {

    int G0_speed = 3600;
    int G1_speed = 1200;
    double E_sum = 0;   //Extrusion output
    double E_rate = 0.03327;    //Extrusion rate
    int layer_num = 0;
    double E_injection = 0;// Total injection extrusion.
    double E_injection_buffer = 0;
    double E_injection_rate = 625;// Syringe output per millimeter of plunger travel.

    ofstream gcode("..//Blocks_files/" + file_name + "_path.gcode");
    gcode_header(gcode);

    for (int i = 0; i < opp.size(); i++) {
        gcode << "G1 Z" << opp[i][0][0][0].m_z + 8 << endl;
        for (int j = 0; j < opp[i].size(); j++) {

            gcode << "Layer:" << ++layer_num << endl;
            if (i == 0 && j == 0) {
                G1_speed = 900;
                E_rate = 0.052;
            }
            else {
                G1_speed = 1200;
                E_rate = 0.03427;
            }
            // Retract.

            for (int m = 0; m < opp[i][j].size(); m++) {

                for (int n = 0; n < opp[i][j][m].size();  n++) {

                    if (opp[i][j][m][n] == layer_type[i][j].center) {// This is an injection center.
                        cout << "This is a injection point!" << endl;
                        // Compute total injection extrusion.
                        double injection_volume = 0;
                        for (int i1 = 0; i1 < opp.size(); i1++) {
                            for (int i2 = 0; i2 < opp[i1].size(); i2++) {
                                if (opp[i1][i2][0][0].m_z < layer_type[i][j].center.m_z) {
                                    injection_volume +=  layer_volume[i1][i2];
                                }
                            }
                        }
                        E_injection = injection_volume / E_injection_rate - E_injection_buffer;
                        E_injection_buffer = injection_volume / E_injection_rate;

                        gcode << "G92 E0" << endl;
                        E_sum = 0;
                        gcode << "T1" << endl;
                        gcode << "M109 S200 C5 W0" << endl;
                        gcode << "M105" << endl;
                        gcode << "M107 P1" << endl;
                        gcode << "G92 E0" << endl;
                        gcode << "G92 B0" << endl;
                        gcode << "G0 F" <<" Z" << layer_type[i][j].center.m_z + 8 << endl;
                        gcode << "G0 F" << G0_speed << " X" << setiosflags(ios::fixed) << setprecision(3) << layer_type[i][j].center.m_x - 59 << " Y" << layer_type[i][j].center.m_y - 32.5 << endl;
                        gcode << "G0 F" << " Z" << layer_type[i][j].center.m_z-0.2 << endl;
                        gcode << "G1 F2" << " E" << E_injection * (material_ratio.first / (material_ratio.first + material_ratio.second)) << " B-" << E_injection * (material_ratio.second / (material_ratio.first + material_ratio.second)) << endl;
                        gcode << "G04 P120000" << endl;
                        gcode << "G92 E0" << endl;
                        gcode << "T0" << endl;
                        gcode << "G92 E0" << endl;

                        continue;
                    }
                    if (n == 0) {
                        gcode << "G0 F" << G0_speed << " X" << setiosflags(ios::fixed) << setprecision(3) << opp[i][j][m][n].m_x << " Y" << opp[i][j][m][n].m_y << " Z" << opp[i][j][m][n].m_z << endl;
                    }
                    else {
                        E_sum += distance(opp[i][j][m][n - 1], opp[i][j][m][n]) * E_rate;
                        gcode << "G1 F" << G1_speed << " X" << setiosflags(ios::fixed) << setprecision(3) << opp[i][j][m][n].m_x << " Y" << opp[i][j][m][n].m_y << " Z" << opp[i][j][m][n].m_z << " E" << E_sum << endl;
                    }
                    if (n == opp[i][j][m].size() - 1 && opp[i][j][m].size() > 2) { //building a loop
                        E_sum += distance(opp[i][j][m][0], opp[i][j][m][n]) * E_rate;
                        gcode << "G1 F" << G1_speed << " X" << setiosflags(ios::fixed) << setprecision(3) << opp[i][j][m][0].m_x << " Y" << opp[i][j][m][0].m_y << " Z" << opp[i][j][m][0].m_z << " E" << E_sum << endl;
                    }
                }
            }
            // Retract.

        }
    }

   gcode_footer(gcode);
   gcode.close();

   vector<vector<vector<Vec3>>> paths;
   vector<vector<Vec3>>paths_buffer2;
   vector<Vec3>paths_buffer1;
   for (int i = 0; i < opp.size(); i++) {
       for (int j = 0; j < opp[i].size(); j++) {
           for (int m = 0; m < opp[i][j].size(); m++) {
               for (int n = 0; n < opp[i][j][m].size(); n++) {

                   paths_buffer1.push_back(opp[i][j][m][n]);
               }
               if (opp[i][j][m].size() > 2) {
                   paths_buffer1.push_back(opp[i][j][m][0]);
               }
               paths_buffer2.push_back(paths_buffer1);
               paths_buffer1.clear();

           }
           paths.push_back(paths_buffer2);
           paths_buffer2.clear();
       }
   }

   OutputPaths(paths, file_name + "_paths" + suffix_txt);

   // Export alternating-step visualization.
   vector<vector<vector<Vec3>>> paths_step;
   for (int i = 0; i < opp.size(); i++) {
       for (int j = 0; j < opp[i].size(); j++) {
           for (int m = 0; m < opp[i][j].size(); m++) {
               for (int n = 0; n < opp[i][j][m].size(); n++) {
                   if (opp[i][j][m][n] == layer_type[i][j].center) {
                       paths_step.push_back(paths_buffer2);
                       paths_buffer2.clear();
                   }
                   else
                       paths_buffer1.push_back(opp[i][j][m][n]);

               }
               if (opp[i][j][m].size() > 2) {
                   paths_buffer1.push_back(opp[i][j][m][0]);
               }
               paths_buffer2.push_back(paths_buffer1);
               paths_buffer1.clear();

           }

       }
   }
   paths_step.push_back(paths_buffer2);
   paths_buffer2.clear();

   OutputPaths(paths_step, file_name + "_paths_step" + suffix_txt);

   // Export visualization for nozzle transitions.
   vector<vector<vector<Vec3>>> paths_jump;
   for (int i = 0; i < opp.size(); i++) {
       for (int j = 0; j < opp[i].size(); j++) {
           for (int m = 0; m < opp[i][j].size(); m++) {
               for (int n = 0; n < opp[i][j][m].size(); n++) {
                   paths_buffer1.push_back(opp[i][j][m][n]);
               }
               if (opp[i][j][m].size() > 2) {
                   paths_buffer1.push_back(opp[i][j][m][0]);
               }
               paths_buffer2.push_back(paths_buffer1);
               paths_buffer1.clear();
           }
       }
       paths_jump.push_back(paths_buffer2);
       paths_buffer2.clear();
   }
   OutputPaths(paths_jump, file_name + "_paths_jump" + suffix_txt);

   // Export visualization for the interface.
   vector<vector<vector<Vec3>>> paths_interface;
   for (int i = 0; i < paths.size(); i++) {
       paths_buffer2.push_back(paths[i][0]);
       paths_interface.push_back(paths_buffer2);
       paths_buffer2.clear();
   }
   OutputPaths(paths_interface, file_name + "_paths_interface" + suffix_txt);

}

double distance(Vec3 a, Vec3 b)//have Z
{
    double dis = sqrt(pow(a.m_x - b.m_x, 2) + pow(a.m_y - b.m_y, 2) + pow(a.m_z - b.m_z, 2));
    return dis;
}

double distance_2(Vec3 a, Vec3 b)
{
    double dis = sqrt(pow(a.m_x - b.m_x, 2) + pow(a.m_y - b.m_y, 2));
    return dis;
}

void OutputPaths(vector<vector<vector<Vec3>>> opp, const std::string& file_name)
{
    std::ofstream dstream(file_name.c_str());
    if (!dstream.is_open()) {
        std::cout << "can not open " << file_name << std::endl;
        return;
    }
    dstream << opp.size() << std::endl;
    for (int i = 0; i < opp.size(); i++) {
       dstream <<opp[i].size() << std::endl;

        for (int j = 0; j < opp[i].size(); j++) {
            dstream << opp[i][j].size() << std::endl;

            for (int k = 0; k < opp[i][j].size(); k++) {
                dstream << opp[i][j][k].m_x << " " << opp[i][j][k].m_y << " " << opp[i][j][k].m_z << std::endl;
            }
        }
    }
    dstream.close();  // Close the stream to flush output and release resources.

}
