#pragma once
#ifndef VISUAL_H_
#define VISUAL_H_

#include "config.h"
#include <vector>

#include <exception>
#include <fstream>

#include "slicer.h"
#include"GeneralMesh.h"
#include<string>
#include <time.h>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
using namespace cv;
class Visual
{
public:
    void insert_Line_Cube(General_Mesh& mesh, std::vector<Vec3> points, float radius);
    void generatePatchOBJ(const string& _file_name, const string& output_folder);
};
#endif
