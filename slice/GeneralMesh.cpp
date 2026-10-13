#include "GeneralMesh.h"

General_Mesh::General_Mesh()
{
	genCube();
}

General_Mesh::~General_Mesh()
{
}

void General_Mesh::genCube()
{
	Vec3 temp(-0.5, -0.5, 0.5);
	cube_vertex.push_back(temp);
	temp.m_x = 0.5;
	cube_vertex.push_back(temp);
	temp.m_y = 0.5;
	cube_vertex.push_back(temp);
	temp.m_x = -0.5;
	cube_vertex.push_back(temp);

	temp.m_x = -0.5, temp.m_y = -0.5, temp.m_z = -0.5;
	cube_vertex.push_back(temp);
	temp.m_x = 0.5;
	cube_vertex.push_back(temp);
	temp.m_y = 0.5;
	cube_vertex.push_back(temp);
	temp.m_x = -0.5;
	cube_vertex.push_back(temp);

	v_int v_temp;
	v_temp.push_back(0);
	v_temp.push_back(1);
	v_temp.push_back(2);
	v_temp.push_back(3);
	cube_faces.push_back(v_temp);

	v_temp.clear();
	v_temp.push_back(7);
	v_temp.push_back(6);
	v_temp.push_back(5);
	v_temp.push_back(4);
	cube_faces.push_back(v_temp);

	v_temp.clear();
	v_temp.push_back(1);
	v_temp.push_back(0);
	v_temp.push_back(4);
	v_temp.push_back(5);
	cube_faces.push_back(v_temp);

	v_temp.clear();
	v_temp.push_back(2);
	v_temp.push_back(6);
	v_temp.push_back(7);
	v_temp.push_back(3);
	cube_faces.push_back(v_temp);

	v_temp.clear();
	v_temp.push_back(2);
	v_temp.push_back(1);
	v_temp.push_back(5);
	v_temp.push_back(6);
	cube_faces.push_back(v_temp);

	v_temp.clear();
	v_temp.push_back(0);
	v_temp.push_back(3);
	v_temp.push_back(7);
	v_temp.push_back(4);
	cube_faces.push_back(v_temp);

	ofstream fout;
	fout.open("cube.obj");
	for (int i = 0; i < cube_vertex.size(); i++)
	{
		fout << "v " << cube_vertex[i].m_x << " " << cube_vertex[i].m_y << " " << cube_vertex[i].m_z << endl;
	}
	for (int i = 0; i < cube_faces.size(); i++)
	{
		int facet_count = cube_faces[i].size();
		fout << "f";
		for (int j = 0; j < facet_count; j++)
		{
			fout << " " << cube_faces[i][j] + 1;
		}
		fout << endl;
	}
}

void General_Mesh::insert(vector<Vec3>& origin_vertexs, vector<v_int>& origin_faces,
    vector<Vec3> vertexs, vector<v_int> faces)
{
    const int offset = static_cast<int>(origin_vertexs.size());
    origin_vertexs.insert(origin_vertexs.end(), vertexs.begin(), vertexs.end());
    for (auto face : faces) {
        for (auto& index : face) index += offset;
        origin_faces.push_back(face);
    }
}

void General_Mesh::insert(vector<Vec3> vertexs, vector<v_int> faces)
{
	insert(result_vertex, result_faces, vertexs, faces);
}

void General_Mesh::genResultMesh(const char* filename)
{
	ofstream fout;
	fout.open(filename);

	if (r != 0 || g != 0 || b != 0)
		for (int i = 0; i < result_vertex.size(); i++)
		{
			fout << "v " << result_vertex[i].m_x << " " << result_vertex[i].m_y << " " << result_vertex[i].m_z << " " << r << " " << g << " " << b << " " << endl;
		}
	else
		for (int i = 0; i < result_vertex.size(); i++)
		{
			fout << "v " << result_vertex[i].m_x << " " << result_vertex[i].m_y << " " << result_vertex[i].m_z << endl;
		}

	for (int i = 0; i < result_faces.size(); i++)
	{
		int facet_count = result_faces[i].size();
		fout << "f";
		for (int j = 0; j < facet_count; j++)
		{
			fout << " " << result_faces[i][j] + 1;
		}
		fout << endl;
	}
}

vector<Vec3> General_Mesh::meshTrans(Vec3 dire, vector<Vec3> point)
{
	vector<Vec3> result;
	int count = point.size();
	for (int i = 0; i < count; i++)
	{
		result.push_back(point[i] + dire);
	}
	return result;
}

vector<Vec3> General_Mesh::meshScale(Vec3 scale, vector<Vec3> point)
{
	vector<Vec3> result;
	int count = point.size();
	for (int i = 0; i < count; i++)
	{
		result.push_back(point[i] * scale);
	}
	return result;
}

vector<Vec3> General_Mesh::meshRotate(Vec3 z_dire, vector<Vec3> point)
{
	z_dire.Normalized();

	Vec3 x_dire, y_dire;

	float t = powf(z_dire.m_x, 2) + powf(z_dire.m_y, 2);

	if (t == 0)
	{
		x_dire.m_x = z_dire.m_z;
	}
	else
	{
		x_dire.m_x = -z_dire.m_y;
		x_dire.m_y = z_dire.m_x;
		x_dire.Normalized();
	}

	y_dire = Cross(z_dire, x_dire);

	vector<Vec3> result;
	for (int i = 0; i < point.size(); i++)
	{
		Vec3 temp = point[i].m_x * x_dire + point[i].m_y * y_dire + point[i].m_z * z_dire;
		result.push_back(temp);
	}

	return result;
}
