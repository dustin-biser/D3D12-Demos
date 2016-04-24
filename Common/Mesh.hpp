/**
 * Mesh.h
 */

#pragma once

#include <vector>

//--Forward Declarations:
class MeshImpl;



class Mesh {
public:
	/// Per Vertex data
	struct Vertex {
		float position[3];
		float normal[3];
		float texCoord[2];
	};

	/// Indices are in the range (0 - 65536] per Mesh.
	typedef uint16 Index;

    Mesh (
        const char * assetPath
    );

    ~Mesh();

	const std::vector<Vertex> & vertexData() const;

	const std::vector<Index> & indexData() const;

private:
    MeshImpl * impl;

};
