//
// ObjFileLoader.hpp
//
#pragma once

#include "Common/BasicTypes.hpp"

struct Vertex {
	float position[3];
	float normal[3];
	float uv_diffuse[2];
};

/// Index into vertex list
typedef ushort VertexIndex;

struct Mesh {
	Vertex * vertices;
	uint32 numVertices;
	VertexIndex * indices;
	uint32 numIndices;
};


namespace MeshFileLoader {
	void loadObjAsset (
		const char * objFilePath,
		Mesh & mesh
	);
};
