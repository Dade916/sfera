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

#ifndef _SFERA_BVHACCEL_H
#define	_SFERA_BVHACCEL_H

#include <vector>

#include "acceleretor/acceleretor.h"

struct BVHAccelTreeNode {
	Sphere bsphere;
	unsigned int primitiveIndex;
	BVHAccelTreeNode *leftChild;
	BVHAccelTreeNode *rightSibling;
};

struct BVHAccelArrayNode {
	Sphere bsphere;
	unsigned int primitiveIndex;
	unsigned int skipIndex;
};

// BVHAccel Declarations
class BVHAccel : public Accelerator {
public:
	// BVHAccel Public Methods
	BVHAccel(const vector<const Sphere *> &spheres,
			const unsigned int treetype, const int icost,
			const int tcost, const float ebonus);
	~BVHAccel();

	AcceleratorType GetType() const { return ACCEL_BVH; }

	bool Intersect(Ray *ray, Sphere **hitSphere, unsigned int *primitiveIndex) const;


	unsigned int nNodes;
	BVHAccelArrayNode *bvhTree;

private:
	// For some debuging
	static bool CheckBoundingSpheres(const Sphere &parentSphere, const BVHAccelTreeNode *bvhTree);

	// BVHAccel Private Methods
	void Init(const vector<const Sphere *> &spheres);
	BVHAccelTreeNode *BuildHierarchy(vector<BVHAccelTreeNode *> &list,
			const unsigned int begin, const unsigned int end, const unsigned int axis);
	void FindBestSplit(vector<BVHAccelTreeNode *> &list,
		const unsigned int begin, const unsigned int end, float *splitValue, unsigned int *bestAxis);
	unsigned int BuildArray(BVHAccelTreeNode *node, const unsigned int offset);
	void FreeHierarchy(BVHAccelTreeNode *node);

	unsigned int treeType;
	int isectCost, traversalCost;
	float emptyBonus;
};

#endif	/* _SFERA_BVHACCEL_H */
