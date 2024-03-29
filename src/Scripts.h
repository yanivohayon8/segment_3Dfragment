#pragma once
#include <string>
#include <vector>
#include <Fragment.h>
#include <Segment.h>
#include <Eigen/Eigenvalues>

Segment segment_intact_surface(ObjFragment& fragment, bool isSave);
void segment_opposite_surface(ObjFragment& fragment, bool isSave,bool isVisualizer);
void segment_sidewalls_surface(ObjFragment& fragment, bool isSave, bool isVisualizer);
void colorSmooth(ObjFragment& fragment, bool isSave, bool isVisualizer);