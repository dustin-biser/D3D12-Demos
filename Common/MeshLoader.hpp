/**
 * MeshLoader.hpp
 */

#pragma once

#include <vector>

//--Forward Declarations:
class MeshLoaderImpl;

struct Mesh {
	/// Per Mesh Vertex data
	struct Vertex {
		float position[3];
		float normal[3];
		float texCoord[2];
	};

	/// Mesh Indices are in the range (0 - 65536].
	typedef uint16 Index;

	std::vector<Vertex> vertices;
	std::vector<Index> indices;
};


class MeshLoader {
public:
	/// Load data into Mesh object from a .obj asset file.
    static void loadMesh (
        _In_ const char * assetPath,
		_Out_ Mesh & mesh
    );

};
