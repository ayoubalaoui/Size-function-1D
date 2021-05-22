#pragma once

#include "TriMesh.h"

float GeometricMomentAt(const TriMesh* mesh, int p,int q,int r,int i);

float GeometricMomentAll(const TriMesh* mesh, int p,int q,int r);

void GeometricMomentDescriptor(const TriMesh *mesh,int maxOrder, const std::string &fileName);

void StatisticMoment(TriMesh *mesh,int maxOrder, const std::string &fileName);