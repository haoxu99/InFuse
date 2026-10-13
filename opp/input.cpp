#include "input.h"

void Input::DefaultPrintingDirectionSlice(int type)
{
	Katana::Instance().config.loadConfig(this->config_path);
	Katana::Instance().stl.loadStl(this->model_path);
	Katana::Instance().slicer.buildLayers();
	Katana::Instance().slicer.buildSegments();
	if(type == 0) //Shell mode
		Katana::Instance().gcode.write("slice_layers.txt");
	else if(type == 1) //Outer mode
		Katana::Instance().gcode.write("slice_layers_outer.txt");
	else if(type == 2) //inner mode
		Katana::Instance().gcode.write("slice_layers_inner.txt");
	this->KanataClear();
}

void Input::KanataClear()
{
	Katana::Instance().triangles.clear();
	Katana::Instance().vertices.clear();
	Katana::Instance().layers.clear();
	Katana::Instance().stl.f_triangles.clear();
	Katana::Instance().min_z = 0;
}
