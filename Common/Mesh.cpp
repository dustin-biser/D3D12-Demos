/**
 * Mesh.cpp
 */

#include "pch.h"
using Microsoft::WRL::ComPtr;

#include <iostream>

#include "Mesh.hpp"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tinyobjloader\tiny_obj_loader.h>


class MeshImpl {
private:
	friend class Mesh;

	// Mesh Data
	std::vector<Mesh::Vertex> vertexData;
	std::vector<Mesh::Index> indexData;

	void MeshImpl::assembleVertexData (
		_In_ const tinyobj::mesh_t & mesh
	);

	void MeshImpl::acquireIndexData (
		_In_ const tinyobj::mesh_t & mesh
	);

}; // end class MeshImpl


//---------------------------------------------------------------------------------------
// Assemble interleaved data so all Mesh vertices are contiguous in memory.
void MeshImpl::assembleVertexData (
	_In_ const tinyobj::mesh_t & mesh
) {
	const std::vector<uint> & indices = mesh.indices;
	const std::vector<float> & positions = mesh.positions;
	const std::vector<float> & normals = mesh.normals;
	const std::vector<float> & texCoords = mesh.texcoords;

	this->vertexData.resize(indices.size());

	uint vertexIndex(0);
	//-- Each index corresponds to 3 positions, 3 normals, and 2 texCoords:
	for (uint index : indices) {
		float * position = this->vertexData[vertexIndex].position;
		position[0] = positions[3*index];
		position[1] = positions[3*index+1];
		position[2] = positions[3*index+2];

		float * normal = this->vertexData[vertexIndex].normal;
		normal[0] = normals[3*index];
		normal[1] = normals[3*index+1];
		normal[2] = normals[3*index+2];

		float * texCoord = this->vertexData[vertexIndex].texCoord;
		texCoord[0] = texCoords[2*index];
		texCoord[1] = texCoords[2*index+1];

		++vertexIndex;
	}
}

//---------------------------------------------------------------------------------------
void MeshImpl::acquireIndexData (
	_In_ const tinyobj::mesh_t & mesh
) {
	std::vector<Mesh::Index> & indexData = this->indexData;
	const auto & meshIndices = mesh.indices;

	size_t numIndices = meshIndices.size();
	indexData.resize(numIndices);

	for (size_t i(0); i < numIndices; ++i) {
		// Convert index to appropriate type, then copy it.
		indexData[i] = Mesh::Index(meshIndices[i]); 
	}

}

//---------------------------------------------------------------------------------------
Mesh::Mesh (
	const char * assetPath
)
{
	assert(assetPath);

	this->impl = new MeshImpl();

	std::string inputfile(assetPath);
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;

	std::string err;
	bool ret = tinyobj::LoadObj(shapes, materials, err, inputfile.c_str());
	if (!err.empty()) { // `err` may contain warning messages.
		std::cerr << err << std::endl;
	}
	if (!ret) {
		// Problem loading .obj file.
		throw;
	}

	// Should only have a single shape with a single mesh.
	assert(shapes.size() == 1);
	tinyobj::mesh_t & mesh = shapes[0].mesh;

	std::vector<float> & normals = mesh.normals;
	std::vector<float> & texCoords = mesh.texcoords;
	size_t numPositions = mesh.positions.size();
	//--Insert dummy data so that positions, normals, and texCoords have the same size:
	{
		if (normals.size() == 0) { normals.resize(numPositions, 0.0f); }
		if (texCoords.size() == 0) { texCoords.resize(numPositions, 0.0f); }
	}

	impl->assembleVertexData(mesh);
	impl->acquireIndexData(mesh);
}

//---------------------------------------------------------------------------------------
Mesh::~Mesh()
{
    delete impl;
}

//---------------------------------------------------------------------------------------
const std::vector<Mesh::Vertex> & Mesh::vertexData() const
{
	return impl->vertexData;
}

//---------------------------------------------------------------------------------------
const std::vector<Mesh::Index> & Mesh::indexData() const
{
	return impl->indexData;
}

