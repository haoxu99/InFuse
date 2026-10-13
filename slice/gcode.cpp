#define _CRT_SECURE_NO_WARNINGS
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
#include "katana.h"
#include "stl.h"
#include "gcode.h"
#include "../opp/polygon.h"
using namespace std;

void adjust_gcode(vector<vector<vector<Vertex>>>& ALL_OPP)
{
    Vertex center_point;
    center_point.x = center_point.y = center_point.z = 0;
    int cont_point = 0;
    for (int i = 0; i < ALL_OPP.size(); i++)
        for (int j = 0; j < ALL_OPP[i].size(); j++)
            for (int k = 0; k < ALL_OPP[i][j].size(); k++)
            {
                center_point.x += ALL_OPP[i][j][k].x;
                center_point.y += ALL_OPP[i][j][k].y;
                center_point.z += ALL_OPP[i][j][k].z;
                cont_point++;
            }
    center_point.x /= cont_point;
    center_point.y /= cont_point;
    center_point.z /= cont_point;
    for (int i = 0; i < ALL_OPP.size(); i++)
        for (int j = 0; j < ALL_OPP[i].size(); j++)
            for (int k = 0; k < ALL_OPP[i][j].size(); k++)
            {
                ALL_OPP[i][j][k].x -= center_point.x;
                ALL_OPP[i][j][k].y -= center_point.y;
                ALL_OPP[i][j][k].z -= center_point.z;
            }
    for (int i = 0; i < ALL_OPP.size(); i++)
        for (int j = 0; j < ALL_OPP[i].size(); j++)
            for (int k = 0; k < ALL_OPP[i][j].size(); k++)
            {
                double temp = ALL_OPP[i][j][k].y;
                ALL_OPP[i][j][k].y = ALL_OPP[i][j][k].z;
                ALL_OPP[i][j][k].z = -temp;
            }
    for (int i = 0; i < ALL_OPP.size(); i++)
        for (int j = 0; j < ALL_OPP[i].size(); j++)
            for (int k = 0; k < ALL_OPP[i][j].size(); k++)
            {
                ALL_OPP[i][j][k].x += 100;
                ALL_OPP[i][j][k].y += 100;
                ALL_OPP[i][j][k].z += 6.5;
            }
}

double distance(Vertex a, Vertex b)
{
    double dis = sqrt(pow(a.x - b.x, 2) + pow(a.y - b.y, 2) + pow(a.z - b.z, 2));
    return dis;
}

// save Gcode
// iterates over the previously generated layers and emit gcode for every segment
// uses some configuration values to decide when to retract the filament, how much
// to extrude and so on.
void GCodeWriter::write(const char* filename)
{
    vector<vector<vector<Vertex>>> out_slicer_layers;

    // segments shorter than this are ignored
    float skipDistance = .0001;

    // compute extrusion factor, that is the amount of filament feed over extrusion length
    float dia = Katana::Instance().config.get("filament_diameter");
    float filamentArea = 3.14159f * dia * dia / 4;
    float extrusionVolume = Katana::Instance().config.get("nozzle_diameter") * Katana::Instance().config.get("layer_height");
    float extrusionFactor = extrusionVolume / filamentArea * Katana::Instance().config.get("extrusion_multiplier");

    // retract filament if traveling
    float retract_length = Katana::Instance().config.get("retract_length");
    float retract_before_travel = Katana::Instance().config.get("retract_before_travel");

    // statistical values shown to the user
    int travels = 0, longTravels = 0, extrusions = 0;
    int travelsSkipped = 0, extrusionsSkipped = 0;
    float travelled = 0, extruded = 0;

    // offset of the emitted Gcode coordinates to the .stl ones

    Vertex offset = { 0,0,0 };
    Vertex position = { 0,0,0 };

    for (unsigned int i = 0; i < Katana::Instance().layers.size(); i++)
    {
        vector<vector<Vertex>> temp;
        out_slicer_layers.push_back(temp);
        Layer& l = Katana::Instance().layers[i];

        float feedrate = (i == 0) ? 500.f : 1200.f;

        float extrusion = (i == 0) ? 1 : 0; // extrusion axis position

        for (unsigned int j = 0; j < l.segments.size(); j++)
        {
            Vertex& v0 = l.segments[j].vertices[0];
            Vertex& v1 = l.segments[j].vertices[1];

            // skip segment with NaN or Infinity caused by numeric instablities
            if (v0.z != l.z) continue;
            if (v1.z != l.z) continue;

            // reorder segment for shorter or zero traveling

            if (v1.distance(position) < v0.distance(position))
                std::swap(v0, v1);

            if (j != l.segments.size() - 1)
            {
                if (v0 == l.segments[j + 1].vertices[0] || v0 == l.segments[j + 1].vertices[1])
                    swap(v0, v1);
            }

            // check distance to decide if we need to travel

            float d = v0.distance(position);
            if (d > skipDistance)
            {

                // the sements are not connected, so travel without extrusion
                if (d > retract_before_travel)
                {
                    // we travel some time, do retraction
                    extrusion -= retract_length;

                    //G92 E0

                }
                vector<Vertex> temp2;
                out_slicer_layers[out_slicer_layers.size() - 1].push_back(temp2);
                // emit G1 travel command

                Vertex VV;
                VV.x = v0.x + offset.x; VV.y = v0.y + offset.y; VV.z = l.z + offset.z;
                out_slicer_layers[out_slicer_layers.size() - 1][out_slicer_layers[out_slicer_layers.size() - 1].size() - 1].push_back(VV);
                if (d > retract_before_travel)
                {
                    // we travelled some time, undo retraction
                    extrusion += retract_length;

                    longTravels++;
                }
                travels++;
                travelled += v0.distance(position);
                position = v0;
            }
            else   // the segments where connected or not far away
                travelsSkipped++;

            extrusion += extrusionFactor * v0.distance(v1) * 0.0065; // compute extrusion by segment length
            if (v1.distance(position) > skipDistance)
            {
                // emit G1 extrusion command

                Vertex VV;
                VV.x = v1.x + offset.x; VV.y = v1.y + offset.y; VV.z = l.z + offset.z;
                out_slicer_layers[out_slicer_layers.size() - 1][out_slicer_layers[out_slicer_layers.size() - 1].size() - 1].push_back(VV);
                extrusions++;
                extruded += v1.distance(position);
                position = v1;
            }
            else   // the segment is to short to do extrusion
                extrusionsSkipped++;
        }
    }

    ////////////////////////find flatten layers/////////////////////////////

    //// print some statisitcs
    //printf("Saving complete. %ld bytes written. %d travels %.0f mm, %d long travels, %d extrusions %.0f mm, %d travel skips, %d extrusion skips\n",

    ofstream ofile(filename);
    ofstream ofile_blocks("../Blocks_files/" + file_name + ".txt");
    ofile_blocks << out_slicer_layers.size() << endl;
    ofile << out_slicer_layers.size() << endl;
    std::vector<std::vector<bool>> jud_connected(out_slicer_layers.size());

    double MIN_SEGMENT = 3; //3
    double MIN_CONNECT_DISTANCE = 5; //5

    for (int i = 0; i < out_slicer_layers.size(); i++)
    {
        jud_connected[i].assign(out_slicer_layers[i].size(), false);
        for (int j = 0; j < out_slicer_layers[i].size() - 1; j++)
        {
            if (jud_connected[i][j] == false)
            {
                for (int k = j + 1; k < out_slicer_layers[i].size(); k++)
                {
                    if (jud_connected[i][k] == false)
                    {
                        if (distance(out_slicer_layers[i][j][out_slicer_layers[i][j].size() - 1], out_slicer_layers[i][k][0]) < MIN_CONNECT_DISTANCE)
                        {
                            for (int m = 0; m < out_slicer_layers[i][k].size(); m++)
                                out_slicer_layers[i][j].push_back(out_slicer_layers[i][k][m]);
                            out_slicer_layers[i][k].clear();
                            jud_connected[i][k] = true;
                            k = j + 1;
                        }
                        else if (distance(out_slicer_layers[i][j][out_slicer_layers[i][j].size() - 1], out_slicer_layers[i][k][out_slicer_layers[i][k].size() - 1]) < MIN_CONNECT_DISTANCE)
                        {
                            for (int m = out_slicer_layers[i][k].size() - 1; m >= 0; m--)
                                out_slicer_layers[i][j].push_back(out_slicer_layers[i][k][m]);
                            out_slicer_layers[i][k].clear();
                            jud_connected[i][k] = true;
                            k = j + 1;
                        }
                        else if (distance(out_slicer_layers[i][j][0], out_slicer_layers[i][k][out_slicer_layers[i][k].size() - 1]) < MIN_CONNECT_DISTANCE)
                        {
                            vector<Vertex> temp_v;
                            temp_v.clear();
                            for (int m = out_slicer_layers[i][j].size() - 1; m >= 0; m--)
                                temp_v.push_back(out_slicer_layers[i][j][m]);
                            out_slicer_layers[i][j] = temp_v;
                            for (int m = out_slicer_layers[i][k].size() - 1; m >= 0; m--)
                                out_slicer_layers[i][j].push_back(out_slicer_layers[i][k][m]);
                            out_slicer_layers[i][k].clear();
                            jud_connected[i][k] = true;
                            k = j + 1;
                        }
                        else if (distance(out_slicer_layers[i][j][0], out_slicer_layers[i][k][0]) < MIN_CONNECT_DISTANCE)
                        {
                            vector<Vertex> temp_v;
                            temp_v.clear();
                            for (int m = out_slicer_layers[i][j].size() - 1; m >= 0; m--)
                                temp_v.push_back(out_slicer_layers[i][j][m]);
                            out_slicer_layers[i][j] = temp_v;
                            for (int m = 0; m < out_slicer_layers[i][k].size(); m++)
                                out_slicer_layers[i][j].push_back(out_slicer_layers[i][k][m]);
                            out_slicer_layers[i][k].clear();
                            jud_connected[i][k] = true;
                            k = j + 1;
                        }
                    }
                }
            }
        }
    }

    for (int i = 0; i < out_slicer_layers.size(); i++)
    {
        for (int j = 0; j < out_slicer_layers[i].size(); j++)
        {
            if (jud_connected[i][j] == false)
            {
                double sum_distance = 0;
                for (int k = 0; k < out_slicer_layers[i][j].size() - 1; k++)
                {
                    sum_distance += distance(out_slicer_layers[i][j][k], out_slicer_layers[i][j][k + 1]);
                }
                if (sum_distance < MIN_SEGMENT)
                {
                    out_slicer_layers[i][j].clear();
                }
            }
        }
        vector<vector<vector<Vertex>>> real_out_slicer_layers = out_slicer_layers;
        int cont_real_segment = 0;
        for (int j = 0; j < out_slicer_layers[i].size(); j++)
            if (out_slicer_layers[i][j].size() != 0 && jud_connected[i][j] == false)
                cont_real_segment++;
        ofile << cont_real_segment << endl;
        for (int j = 0; j < out_slicer_layers[i].size(); j++)
        {
            if (out_slicer_layers[i][j].size() != 0 && jud_connected[i][j] == false)
            {
                ofile << out_slicer_layers[i][j].size() << endl;
                for (int k = 0; k < out_slicer_layers[i][j].size(); k++)
                {
                    if (k != 0 && k != out_slicer_layers[i][j].size()-1) {
                        Vertex diff = out_slicer_layers[i][j][k + 1] - out_slicer_layers[i][j][k];
                        real_out_slicer_layers[i][j][k].x = out_slicer_layers[i][j][k].x + diff.x / MAX_D;
                        real_out_slicer_layers[i][j][k].y = out_slicer_layers[i][j][k].y + diff.y / MAX_D;
                        real_out_slicer_layers[i][j][k].z = out_slicer_layers[i][j][k].z + diff.z / MAX_D;
                    }
                     ofile << real_out_slicer_layers[i][j][k].x << " " << real_out_slicer_layers[i][j][k].y << " " << real_out_slicer_layers[i][j][k].z << endl;
                }
            }
        }

        //output blocks_files
        if (real_out_slicer_layers[i][0].size() != 0 && jud_connected[i][0] == false)
        {
            ofile_blocks << real_out_slicer_layers[i][0].size() << endl;
            for (int k = 0; k < real_out_slicer_layers[i][0].size(); k++)
            {
                ofile_blocks << real_out_slicer_layers[i][0][k].x << " " << real_out_slicer_layers[i][0][k].y << " " << real_out_slicer_layers[i][0][k].z << " "<< dh <<endl;
            }
        }
    }
}
