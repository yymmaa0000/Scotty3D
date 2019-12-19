#include "bvh.h"

#include "CMU462/CMU462.h"
#include "static_scene/triangle.h"

#include <iostream>
#include <stack>

using namespace std;

namespace CMU462 {
namespace StaticScene {

	// build the BVH tree using discussed in lecture 15
	// This function is called recursively
	// returns the root of the tree/sub-tree
	// index = indexes of primitives that belonged to this part of tree
	BVHNode* BVHAccel::build_tree(const std::vector<Primitive *> & _primitives, std::vector<int> & index, size_t max_leaf_size) {

		int B = 8; // number of buckets to split
		
		// create root node with all primitives
		BBox bb;
		for (int i : index) bb.expand(primitives[i]->get_bbox());
		BVHNode* result = new BVHNode(bb, index[0], index.size());

		// check for ending condition
		if (index.size() == 2 && index[1] - index[0] != 1) {
			result->l = build_tree(_primitives, std::vector<int>{index[0]}, max_leaf_size);
			result->r = build_tree(_primitives, std::vector<int>{index[1]}, max_leaf_size);
			return result;
		}
		if (index.size() <= max_leaf_size) {
			bool is_consecutive = true;
			for (int i = 1; i < index.size(); i++) {
				if (index[i] - index[i - 1] != 1) {
					is_consecutive = false;
					break;
				}
			}
			if (is_consecutive) return result;
		}

		// if not leaf node, start spliting the space using 
		// method discussed in lecture 15
		int best_split = 0, best_direction = 0;
		double temp_cost, best_cost = INF_D;
		double SN = bb.surface_area(); // total surface area
		double SA, SB; // surface area of two splits
		int NA, NB; // number of primitives in the two region
		double Ctrav = 0.0; // cost of traversing into next branch
		double Cisect = 1.0; // cost of ray-primitive intersection
		for (int direction = 0; direction < 3; direction++) {
			std::vector<std::vector<int>> bucket_index(B);
			std::vector<BBox> bucket_box(B);

			// split bbox into B buckets
			double min_coord = bb.min[direction];
			double max_coord = bb.max[direction];
			double interval = (max_coord - min_coord) / B;
			
			// insert every primitive into specific bucket 
			// in every possible direction
			int bucket;
			Vector3D centroid;
			BBox temp_box;
			for (int i : index) {
				temp_box = _primitives[i]->get_bbox();
				centroid = temp_box.centroid();
				bucket = int((centroid[direction] - min_coord) / interval);
				bucket_index[bucket].push_back(i);
				bucket_box[bucket].expand(temp_box);
			}
			
			// For each of the B-1 possible partitioning planes 
			// that cuts the B number of buckets, evaluate SAH to find
			// the best split
			for (int b = 1; b < B; b++) {
				NA = 0; 
				NB = 0;
				BBox lbox, rbox;
				for (int left = 0; left < b; left++) {
					NA += bucket_index[left].size();
					lbox.expand(bucket_box[left]);
				}
				for (int right = b; right < B; right++) {
					NB += bucket_index[right].size();
					rbox.expand(bucket_box[right]);
				}
				SA = lbox.surface_area();
				SB = rbox.surface_area();

				if (NA == 0 || NB == 0) continue;
				temp_cost = Ctrav + SA / SN * NA*Cisect + SB / SN * NB*Cisect;
				if (temp_cost < best_cost) {
					best_cost = temp_cost;
					best_split = b;
					best_direction = direction;
				}
			}
		}

		// partition the primitives using cut method 
		// that results in the lowest cost
		double min_coord = bb.min[best_direction];
		double max_coord = bb.max[best_direction];
		double limit = (max_coord - min_coord) / B * best_split + min_coord;
		std::vector<int> left_index;
		std::vector<int> right_index;
		Vector3D centroid;
		BBox temp_box;
		for (int i : index) {
			temp_box = _primitives[i]->get_bbox();
			centroid = temp_box.centroid();
			if (centroid[best_direction] >= limit) right_index.push_back(i);
			else left_index.push_back(i);
		}

		// call itself recursively to build the left 
		// and right subtrees
		result->l = build_tree(_primitives, left_index, max_leaf_size);
		result->r = build_tree(_primitives, right_index, max_leaf_size);
		return result;
	}

	BVHAccel::BVHAccel(const std::vector<Primitive *> &_primitives,
		size_t max_leaf_size) {
		this->primitives = _primitives;

		// TODO (PathTracer):
		// Construct a BVH from the given vector of primitives and maximum leaf
		// size configuration. The starter code build a BVH aggregate with a
		// single leaf node (which is also the root) that encloses all the
		// primitives.

		std::vector<int> index(primitives.size());
		for (int i = 0; i < primitives.size(); i++) index[i] = i;
		root = build_tree(_primitives, index, max_leaf_size);
	}

	// delete the BVH tree recursively
	void BVHAccel::delete_tree(BVHNode* node) {
		if (node == NULL) return;
		delete_tree(node->l);
		delete_tree(node->r);
		delete node;
		return;
	}

	BVHAccel::~BVHAccel() {
		// TODO (PathTracer):
		// Implement a proper destructor for your BVH accelerator aggregate
		delete_tree(root);
	}

BBox BVHAccel::get_bbox() const { return root->bb; }

bool BVHAccel::intersect(const Ray &ray) const {
  // TODO (PathTracer):
  // Implement ray - bvh aggregate intersection test. A ray intersects
  // with a BVH aggregate if and only if it intersects a primitive in
  // the BVH that is not an aggregate.

  bool hit = false;
  for (size_t p = 0; p < primitives.size(); ++p) {
    if (primitives[p]->intersect(ray)) hit = true;
  }

  return hit;
}

bool BVHAccel::intersect(const Ray &ray, Intersection *isect) const {
  // TODO (PathTracer):
  // Implement ray - bvh aggregate intersection test. A ray intersects
  // with a BVH aggregate if and only if it intersects a primitive in
  // the BVH that is not an aggregate. When an intersection does happen.
  // You should store the non-aggregate primitive in the intersection data
  // and not the BVH aggregate itself.

  bool hit = false;
  for (size_t p = 0; p < primitives.size(); ++p) {
    if (primitives[p]->intersect(ray, isect)) hit = true;
  }

  return hit;
}

}  // namespace StaticScene
}  // namespace CMU462
