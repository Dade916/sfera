/***************************************************************************
 *   Copyright (C) 1998-2010 by authors (see AUTHORS.txt)                  *
 *                                                                         *
 *   This file is part of Sfera.                                           *
 *                                                                         *
 *   Sfera is free software; you can redistribute it and/or modify         *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   Sfera is distributed in the hope that it will be useful,              *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 *                                                                         *
 ***************************************************************************/

// Boundary Volume Hierarchy accelerator
// Based of "Efficiency Issues for Ray Tracing" by Brian Smits
// Available at http://www.cs.utah.edu/~bes/papers/fastRT/paper.html

#include <iostream>
#include <functional>
#include <algorithm>
#include <deque>

#include "acceleretor/bvhaccel.h"

using std::bind2nd;
using std::ptr_fun;

// BVHAccel Method Definitions

BVHAccel::BVHAccel(const vector<const Sphere *> &spheres,
		const unsigned int treetype, const int icost,
		const int tcost, const float ebonus) :
		isectCost(icost), traversalCost(tcost), emptyBonus(ebonus) {
	// Make sure treeType is 2, 4 or 8
	if (treetype <= 2) treeType = 2;
	else if (treetype <= 4) treeType = 4;
	else treeType = 8;

	Init(spheres);
}

void BVHAccel::Init(const vector<const Sphere *> &spheres) {
	//const double t1 = WallClockTime();

	const size_t nSpheres = spheres.size();

	vector<BVHAccelTreeNode *> bvList(nSpheres);
	for (unsigned int i = 0; i < nSpheres; ++i) {
		BVHAccelTreeNode *ptr = new BVHAccelTreeNode();
		ptr->bsphere = *(spheres[i]);
		ptr->primitiveIndex = i;
		ptr->leftChild = NULL;
		ptr->rightSibling = NULL;
		bvList[i] = ptr;
	}

	//SFERA_LOG("Building Bounding Volume Hierarchy, primitives: " << nSpheres);

	nNodes = 0;
	BVHAccelTreeNode *rootNode = BuildHierarchy(bvList, 0, bvList.size(), 2);
	//assert (CheckBoundingSpheres(rootNode->bsphere, rootNode));

	//SFERA_LOG("Pre-processing Bounding Volume Hierarchy, total nodes: " << nNodes);

	bvhTree = new BVHAccelArrayNode[nNodes];
	BuildArray(rootNode, 0);
	FreeHierarchy(rootNode);

	//const double t2 = WallClockTime();
	//const double dt = t2 - t1;
	//SFERA_LOG("Total BVH memory usage: " << nNodes * sizeof(BVHAccelArrayNode) / 1024 << "Kbytes");
	//SFERA_LOG("Finished building Bounding Volume Hierarchy array: " << fixed << setprecision(1) << dt * 1000.f << "ms");
}

BVHAccel::~BVHAccel() {
	delete[] bvhTree;
}

void BVHAccel::FreeHierarchy(BVHAccelTreeNode *node) {
	if (node) {
		FreeHierarchy(node->leftChild);
		FreeHierarchy(node->rightSibling);

		delete node;
	}
}

// Build an array of comparators for each axis

bool bvh_ltf_x(const BVHAccelTreeNode *n, const float v) {
	return n->bsphere.center.x < v;
}

bool bvh_ltf_y(const BVHAccelTreeNode *n, const float v) {
	return n->bsphere.center.y < v;
}

bool bvh_ltf_z(const BVHAccelTreeNode *n, const float v) {
	return n->bsphere.center.z < v;
}

bool (* const bvh_ltf[3])(const BVHAccelTreeNode *n, const float v) = { bvh_ltf_x, bvh_ltf_y, bvh_ltf_z };

BVHAccelTreeNode *BVHAccel::BuildHierarchy(
		vector<BVHAccelTreeNode *> &list,
		const unsigned int begin,
		const unsigned int end,
		const unsigned int axis) {
	unsigned int splitAxis = axis;
	float splitValue;

	nNodes += 1;
	if (end - begin == 1) // Only a single item in list so return it
		return list[begin];

	BVHAccelTreeNode *parent = new BVHAccelTreeNode();
	parent->primitiveIndex = 0xffffffffu;
	parent->leftChild = NULL;
	parent->rightSibling = NULL;

	vector<unsigned int> splits;
	splits.reserve(treeType + 1);
	splits.push_back(begin);
	splits.push_back(end);
	for (unsigned int i = 2; i <= treeType; i *= 2) { // Calculate splits, according to tree type and do partition
		for (unsigned int j = 0, offset = 0; j + offset < i && splits.size() > j + 1; j += 2) {
			if (splits[j + 1] - splits[j] < 2) {
				j--;
				offset++;
				continue; // Less than two elements: no need to split
			}

			FindBestSplit(list, splits[j], splits[j + 1], &splitValue, &splitAxis);

			vector<BVHAccelTreeNode *>::iterator it =
					partition(list.begin() + splits[j], list.begin() + splits[j + 1], bind2nd(ptr_fun(bvh_ltf[splitAxis]), splitValue));
			unsigned int middle = distance(list.begin(), it);
			middle = Max(splits[j] + 1, Min(splits[j + 1] - 1, middle)); // Make sure coincidental BSs are still split
			splits.insert(splits.begin() + j + 1, middle);
		}
	}

	BVHAccelTreeNode *child, *lastChild;
	// Left Child
	child = BuildHierarchy(list, splits[0], splits[1], splitAxis);
	parent->leftChild = child;
	parent->bsphere = child->bsphere;
	lastChild = child;

	// Add remaining children
	for (unsigned int i = 1; i < splits.size() - 1; i++) {
		child = BuildHierarchy(list, splits[i], splits[i + 1], splitAxis);
		lastChild->rightSibling = child;
		parent->bsphere = Union(parent->bsphere, child->bsphere);
		lastChild = child;
	}

	return parent;
}

void BVHAccel::FindBestSplit(
		vector<BVHAccelTreeNode *> &list,
		const unsigned int begin, const unsigned int end,
		float *splitValue, unsigned int *bestAxis) {
	if (end - begin == 2) {
		// Trivial case with two elements
		*splitValue = (list[begin]->bsphere.center[0] + list[end - 1]->bsphere.center[0]) / 2.f;
		*bestAxis = 0;
	} else {
		// Calculate BSs mean center
		Point mean(0.f, 0.f, 0.f), var(0.f, 0.f, 0.f);
		for (unsigned int i = begin; i < end; i++)
			mean += list[i]->bsphere.center;
		mean /= end - begin;

		// Calculate variance
		for (unsigned int i = begin; i < end; i++) {
			Vector v = list[i]->bsphere.center - mean;

			v.x *= v.x;
			v.y *= v.y;
			v.z *= v.z;

			var += v;
		}

		// Select axis with more variance
		if (var.x > var.y && var.x > var.z)
			*bestAxis = 0;
		else if (var.y > var.z)
			*bestAxis = 1;
		else
			*bestAxis = 2;

		// Split in half around the mean center
		*splitValue = mean[*bestAxis];
	}
}

unsigned int BVHAccel::BuildArray(BVHAccelTreeNode *node, unsigned int offset) {
	// Build array by recursively traversing the tree depth-first
	while (node) {
		BVHAccelArrayNode *p = &bvhTree[offset];

		p->bsphere = node->bsphere;
		p->primitiveIndex = node->primitiveIndex;
		offset = BuildArray(node->leftChild, offset + 1);
		p->skipIndex = offset;

		node = node->rightSibling;
	}

	return offset;
}

bool BVHAccel::Intersect(Ray *ray, Sphere **hitSphere, unsigned int *primitiveIndex) const {
	unsigned int currentNode = 0; // Root Node
	unsigned int stopNode = bvhTree[0].skipIndex; // Non-existent
	*primitiveIndex = 0xffffffffu;

	while (currentNode < stopNode) {
		float hitT;
		if (bvhTree[currentNode].bsphere.IntersectP(ray, &hitT)) {
			if ((bvhTree[currentNode].primitiveIndex != 0xffffffffu) && (hitT < ray->maxt)){
				ray->maxt = hitT;
				*primitiveIndex = bvhTree[currentNode].primitiveIndex;
				*hitSphere = &bvhTree[currentNode].bsphere;
				// Continue testing for closer intersections
			}

			currentNode++;
		} else
			currentNode = bvhTree[currentNode].skipIndex;
	}

	return (*primitiveIndex) != 0xffffffffu;
}

// For some debuging
bool BVHAccel::CheckBoundingSpheres(const Sphere &parentSphere, const BVHAccelTreeNode *bvhTree) {
	if (bvhTree->leftChild) {
		if (!parentSphere.Contains(bvhTree->leftChild->bsphere))
			return false;

		return CheckBoundingSpheres(bvhTree->bsphere, bvhTree->leftChild);
	}

	if (bvhTree->rightSibling) {
		if (!parentSphere.Contains(bvhTree->rightSibling->bsphere))
			return false;

		return CheckBoundingSpheres(parentSphere, bvhTree->rightSibling);
	}

	return true;
}