
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
#include"visual.h"
#include <stdexcept>

using namespace std;
float layer_height;

// create initialized layers and assign triangles to them
void Slicer::buildLayers()
{
    // we compute the infill by using a 'plane sweep'.
    // see http://en.wikipedia.org/wiki/Sweep_line_algorithm
    // for that we create an index of vertices sorted by z and iterate it while keeping a heap of
    // triangles currently touching the sweep plane. As there is no topological change between the
    // sweep locations, any layer inbetween can be initialized with triangles intersected by that layer.

    // create a index into the vertices sorted by z
    std::vector<VertexIndex> by_z;
    for (unsigned int i = 0; i < Katana::Instance().triangles.size(); i++)
        for (int j = 0; j < 3; j++) {
            VertexIndex vi = {
              Katana::Instance().triangles[i].vertices[j]->z,
              &Katana::Instance().triangles[i]
            };
            by_z.push_back(vi);
        }
    std::sort(by_z.begin(), by_z.end());

    for (unsigned int i = 0; i < by_z.size(); i++)
    {
        DPRINTF("Vertex %d Z: %f (%f, %f, %f), (%f, %f, %f), (%f, %f, %f)\n", i, by_z[i].value, by_z[i].triangle->vertices[0]->x, by_z[i].triangle->vertices[0]->y, by_z[i].triangle->vertices[0]->z
            , by_z[i].triangle->vertices[1]->x, by_z[i].triangle->vertices[1]->y, by_z[i].triangle->vertices[1]->z
            , by_z[i].triangle->vertices[2]->x, by_z[i].triangle->vertices[2]->y, by_z[i].triangle->vertices[2]->z);
    }

    // sweep heap: updated list of triangles touched by the current sweep plane
    // and the number of triangle vertices already passed by the sweep plane
    std::map<Triangle*, int> activeTriangles;
    int cont_layer = 0;
    // print geometric height
    Katana::Instance().min_z = by_z.front().value;
    float max_z = by_z.back().value;
    layer_height = Katana::Instance().config.get("layer_height");
    if (!std::isfinite(layer_height) || layer_height <= 0)
        throw std::runtime_error("layer_height must be positive.");
    std::printf("Slicing from %f to %f\n", Katana::Instance().min_z, max_z);

    // now do the sweep over all vertices, interrupted at every next_layer_z to fill
    float next_layer_z = Katana::Instance().min_z + layer_height;    // layer_height/2

    DPRINTF("First layer Z: %f\n", next_layer_z);

    bool jud_slice_top = false;
    for (unsigned int i = 0; i < by_z.size(); i++)
    {
        float z = by_z[i].value;
        DPRINTF("Next vertex z %f\n", z);
        // add all layers passed by the sweep so far
        while (z > next_layer_z) {
            DPRINTF("  creating layers as Z: %f > next_layer_z: %f.\n", z, next_layer_z);
            // create new layer
            Layer layer;
            layer.z = next_layer_z;
            // copy triangle pointers to the layer
            DPRINTF("Triangles in this %f layer:\n", layer.z);
            for (std::map<Triangle*, int>::iterator j = activeTriangles.begin(); j != activeTriangles.end(); ++j) {
                layer.triangles.push_back(j->first);
                DPRINTF("   (%f, %f, %f), (%f, %f, %f), (%f, %f, %f)\n", j->first->vertices[0]->x, j->first->vertices[0]->y, j->first->vertices[0]->z,
                    j->first->vertices[1]->x, j->first->vertices[1]->y, j->first->vertices[1]->z, j->first->vertices[2]->x, j->first->vertices[2]->y, j->first->vertices[2]->z);
            }
            // add layer to list
            Katana::Instance().layers.push_back(layer);
            // advance to next layer height
            cont_layer++;

                next_layer_z += layer_height;

        }
        // now the layers are on par with the sweep

        // update the heap by the current vertex
        // get triangle this vertex is of
        Triangle* triangle = by_z[i].triangle;
        DPRINTF("Triangle this belongs to: (%f, %f, %f), (%f, %f, %f),(%f, %f, %f)\n", triangle->vertices[0]->x, triangle->vertices[0]->y, triangle->vertices[0]->z,
            triangle->vertices[1]->x, triangle->vertices[1]->y, triangle->vertices[1]->z, triangle->vertices[2]->x, triangle->vertices[2]->y, triangle->vertices[2]->z);
        int verticesPassed; // how many vertices of the triangle we passed
        if (activeTriangles.count(triangle) == 0)
            // ta new triangle is encountered, we just see its first vertex
            verticesPassed = 1;
        else
            // we already know this triangle, so we pass another of its vertices
            verticesPassed = activeTriangles[triangle] + 1;
        // store new count
        activeTriangles[triangle] = verticesPassed;
        DPRINTF("Vertices of this triangle passed: %d\n", verticesPassed);
        // if we reach a third vertex, it is the triangle's top vertex
        // so we passed it completely. remove it.
        if (verticesPassed == 3) {
            activeTriangles.erase(triangle);
            DPRINTF("All vertices of this triangle are seen. No more active\n");
        }
        DPRINTF("Active triangles: %lu\n", activeTriangles.size());
        // the heap now contains an up to date collection of active triangles
    }
    // the sweep is over, any triangle should have been passed and removed.
    assert(activeTriangles.size() == 0);

    // print amount of layers found.
    printf("Layers: %d\n", (int)Katana::Instance().layers.size());
}

// compute intersection of a segment given by two vertices with a z plane
Vertex Slicer::computeIntersection(Vertex& a, Vertex& b, float z)
{
    float t = (z - a.z) / (b.z - a.z); // projective length
    Vertex intersection = {
      a.x + t * (b.x - a.x),
      a.y + t * (b.y - a.y),
      z
    };

    return intersection;
}

// compute intersection of a triangle with a z plane
// the triangle vertices must be ordered in z
Segment Slicer::computeSegment(Triangle& t, float z)
{
    Vertex** vs = t.vertices;

    // two vertices to return
    Segment segment;

    segment.neighbours[0] = NULL;
    segment.neighbours[1] = NULL;
    segment.orderIndex = -1;

    // triangle vertices are always ordered by z
    assert(vs[0]->z <= vs[1]->z);
    assert(vs[1]->z <= vs[2]->z);

    // ensure the triangles are correctly assigned to the layers
    assert(z >= vs[0]->z);
    assert(z <= vs[2]->z);

    // so we just need to check the second vertex to decide which edges
    // get intersected.
    if (z < vs[1]->z) {
        segment.vertices[0] = this->computeIntersection(*vs[0], *vs[1], z);
        segment.vertices[1] = this->computeIntersection(*vs[0], *vs[2], z);

        segment.adjoint_mappint_point[0].begin = Katana::Instance().mappint_para_point_index[Vertex(*vs[0])];
        segment.adjoint_mappint_point[0].end = Katana::Instance().mappint_para_point_index[Vertex(*vs[1])];
        segment.adjoint_mappint_point[1].begin = Katana::Instance().mappint_para_point_index[Vertex(*vs[0])];
        segment.adjoint_mappint_point[1].end = Katana::Instance().mappint_para_point_index[Vertex(*vs[2])];
        double temp_dis = Distance2D(cv::Point2d(segment.vertices[0].x, segment.vertices[0].z), cv::Point2d(Vertex(*vs[0]).x, Vertex(*vs[0]).z));
        segment.adjoint_mappint_point[0].weight = 1-(temp_dis
            / (Distance2D(cv::Point2d(segment.vertices[0].x, segment.vertices[0].z), cv::Point2d(Vertex(*vs[1]).x, Vertex(*vs[1]).z))+ temp_dis));

        temp_dis = Distance2D(cv::Point2d(segment.vertices[1].x, segment.vertices[1].z), cv::Point2d(Vertex(*vs[0]).x, Vertex(*vs[0]).z));
        segment.adjoint_mappint_point[1].weight =1-(temp_dis
            / (Distance2D(cv::Point2d(segment.vertices[1].x, segment.vertices[1].z), cv::Point2d(Vertex(*vs[2]).x, Vertex(*vs[2]).z)) + temp_dis));
    }
    else {
        segment.vertices[0] = this->computeIntersection(*vs[1], *vs[2], z);
        segment.vertices[1] = this->computeIntersection(*vs[0], *vs[2], z);

        segment.adjoint_mappint_point[0].begin = Katana::Instance().mappint_para_point_index[Vertex(*vs[1])];
        segment.adjoint_mappint_point[0].end = Katana::Instance().mappint_para_point_index[Vertex(*vs[2])];
        segment.adjoint_mappint_point[1].begin = Katana::Instance().mappint_para_point_index[Vertex(*vs[0])];
        segment.adjoint_mappint_point[1].end = Katana::Instance().mappint_para_point_index[Vertex(*vs[2])];
        double temp_dis = Distance2D(cv::Point2d(segment.vertices[0].x, segment.vertices[0].z), cv::Point2d(Vertex(*vs[1]).x, Vertex(*vs[1]).z));
        segment.adjoint_mappint_point[0].weight = 1-(temp_dis
            / (Distance2D(cv::Point2d(segment.vertices[0].x, segment.vertices[0].z), cv::Point2d(Vertex(*vs[2]).x, Vertex(*vs[2]).z)) + temp_dis));

        temp_dis = Distance2D(cv::Point2d(segment.vertices[1].x, segment.vertices[1].z), cv::Point2d(Vertex(*vs[0]).x, Vertex(*vs[0]).z));
        segment.adjoint_mappint_point[1].weight = 1-(temp_dis
            / (Distance2D(cv::Point2d(segment.vertices[1].x, segment.vertices[1].z), cv::Point2d(Vertex(*vs[2]).x, Vertex(*vs[2]).z)) + temp_dis));
    }

    segment.triangle_points.push_back(*t.vertices[0]);
    segment.triangle_points.push_back(*t.vertices[1]);
    segment.triangle_points.push_back(*t.vertices[2]);

    t.normal = calTriNormal(*vs[0], *vs[1], *vs[2]);
    Vertex n = t.normal;

    segment.normal = n;
    return segment;
}

Vertex Slicer::calTriNormal(Vertex ver1, Vertex ver2, Vertex ver3)
{
    float temp1[3], temp2[3], normal[3];
    float length = 0.0;
    temp1[0] = ver2.x - ver1.x;
    temp1[1] = ver2.y - ver1.y;
    temp1[2] = ver2.z - ver1.z;
    temp2[0] = ver3.x - ver2.x;
    temp2[1] = ver3.y - ver2.y;
    temp2[2] = ver3.z - ver2.z;

    normal[0] = temp1[1] * temp2[2] - temp1[2] * temp2[1];
    normal[1] = -(temp1[0] * temp2[2] - temp1[2] * temp2[0]);
    normal[2] = temp1[0] * temp2[1] - temp1[1] * temp2[0];

    length = sqrt(normal[0] * normal[0] + normal[1] * normal[1] + normal[2] * normal[2]);
    if (length == 0.0f) { length = 1.0f; }
    normal[0] /= length;
    normal[1] /= length;
    normal[2] /= length;
    Vertex e_normal(normal[0], normal[1], normal[2]);
    return e_normal;
}

// unify the vertices shared by more than one segment to a map that can be used to find adjacent segments.
// for manifold geomertry, every vertex mappes to exactly two segments then.
// however for non manifold geometry, segmentsByVertex can map to any number of segments.
void Slicer::unifySegmentVertices(std::vector<Segment>& segments, std::map<Vertex, std::vector<Segment*>>& segmentsByVertex)
{
    for (unsigned int i = 0; i < segments.size(); i++)
    {
        Segment& s = segments[i];
        for (int j = 0; j < 2; j++) {
            Vertex& v = s.vertices[j];
            segmentsByVertex[v].push_back(&s);
        }
    }
}

// offset segments by moving them in normal direction and recompute vertices
// this is used to match an extruded segment of certain width to the outer contour of the model
// and place the infill inside of the perimeters

// build segments to be printed for a layer
// first, the contour gained by intersecting the triangles with it's z plane
// second, the infill as generated by fill(..)

void Slicer::buildSegments()
{
    // we try to build closed loops of sements for efficient printing
    Segment last_layer_end_point;
    for (unsigned int layerIndex = 0; layerIndex < Katana::Instance().layers.size(); layerIndex++)
    {
        Layer& layer = Katana::Instance().layers[layerIndex];

        DPRINTF("Building line segments by intersecting the triangles with it's z plane\n");

        // generate segments by intersecting the triangles touching this layer
        for (unsigned int i = 0; i < layer.triangles.size(); i++)
        {
            Triangle* t = layer.triangles[i];
            Segment s = this->computeSegment(*t, layer.z);

            // TODO what if a triangle is sliced at a very flat angle?
            // those would give poor normals and may cause bad contour offsetting

            if (s.vertices[0] != s.vertices[1])
                layer.segments.push_back(s);
        }

        // offset segments inward to correct for extrusion diameter

        // unify segment vertices
        std::map<Vertex, std::vector<Segment*>> segmentsByVertex;
        this->unifySegmentVertices(layer.segments, segmentsByVertex);

        bool jud_close = true;
        std::vector<int> end_segments_index;
        for (std::map<Vertex, std::vector<Segment*>>::iterator i = segmentsByVertex.begin(); i != segmentsByVertex.end(); ++i)
        {
            std::vector<Segment*>& ss = i->second;
            if (ss.size() == 1)
            {
                for (int j = 0; j < layer.segments.size(); j++)
                {
                    if (layer.segments[j].vertices[0] == ss[0]->vertices[0]
                        && layer.segments[j].vertices[1] == ss[0]->vertices[1])
                    {
                        end_segments_index.push_back(j);
                        jud_close = false;
                        break;
                    }
                }
                for (int j = 0; j < end_segments_index.size() - 1; j++)
                {
                    if (end_segments_index[j] == end_segments_index[end_segments_index.size() - 1])
                        end_segments_index.pop_back();
                }
            }
        }

        for (int i = 0; i < end_segments_index.size(); i++)
            for (int j = i + 1; j < end_segments_index.size(); j++)
            {
                if (end_segments_index[i] > end_segments_index[j])
                {
                    swap(end_segments_index[j], end_segments_index[i]);
                }
            }
        for (int i = 0; i < end_segments_index.size(); i++)
        {
            Segment temp = layer.segments[i];
            layer.segments[i] = layer.segments[end_segments_index[i]];
            layer.segments[end_segments_index[i]] = temp;
        }

        //                + pow((layer.segments[end_segments_index[i]].vertices[0].y - last_layer_end_point.vertices[0].y), 2)

        //                + pow((layer.segments[i].vertices[0].y - last_layer_end_point.vertices[0].y), 2)

        segmentsByVertex.clear();
        this->unifySegmentVertices(layer.segments, segmentsByVertex);

        // link segments by neighbour pointers using the unique vertex map
        for (std::map<Vertex, std::vector<Segment*>>::iterator i = segmentsByVertex.begin(); i != segmentsByVertex.end(); ++i)
        {
            std::vector<Segment*>& ss = i->second;

            // checks disabled to accept non manifolds

            if (ss.size() != 2) continue;

            Vertex v = i->first;

            // as we don't know the direction of each segment in the final trajectory,
            // we just link them in the same order as they list their vertices.
            // use two indices for the corresponding neighbour pointers
            // TODO maybe we should make this simpler and just use the first free neighbour pointer,
            // however errors are harder to track than.
            int index0, index1;
            if (ss[0]->vertices[0] == v) index0 = 1;
            else if (ss[0]->vertices[1] == v) index0 = 0;
            else     assert(!"bad index0");

            if (ss[1]->vertices[0] == v) index1 = 1;
            else if (ss[1]->vertices[1] == v) index1 = 0;
            else     assert(!"bad index1");

            // now index0, index1 should point to a free end of the segment
            assert(ss[0]->neighbours[index0] == NULL);
            assert(ss[1]->neighbours[index1] == NULL);

            // finally link both segments
            ss[0]->neighbours[index0] = ss[1];
            ss[1]->neighbours[index1] = ss[0];
        }

        // now order the segments into consecutive loops.
        int loops = 0;
        long orderIndex = 0;
        for (unsigned int i = 0; i < layer.segments.size(); i++) {
            Segment& segment = layer.segments[i];

            // only handle new loops
            if (segment.orderIndex != -1) continue;

            // collect a loop
            Segment* s2 = &segment;
            while (true) {
                s2->orderIndex = orderIndex++;
                // DIRTY: check for NULL neighbours to survive non manifolds
                if (s2->neighbours[0] != NULL && s2->neighbours[0]->orderIndex == -1)
                    s2 = s2->neighbours[0];
                else if (s2->neighbours[1] != NULL && s2->neighbours[1]->orderIndex == -1)
                    s2 = s2->neighbours[1];
                else break;
            };

            // the loop should be closed:
            // DIRTY: ignore check to accept non manifolds

            loops++;
        }
        std::sort(layer.segments.begin(), layer.segments.end());

        for (unsigned int i = 0; i < layer.segments.size(); i++)
        {

             //printf("Segment %d: (%f, %f, %f) -> (%f, %f, %f)\n", i, layer.segments[i].vertices[0].x, layer.segments[i].vertices[0].y,layer.segments[i].vertices[0].z,

        }

        // debug output

        // caution: the neighbour[..] and other segment pointers are invalid now!
    }

}
