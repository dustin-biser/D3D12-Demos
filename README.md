# D3D12-Demos
A collection of demos using various aspects of the D3D12 API.

## [IndexRendering](IndexRendering/)
<img src="./Images/index_rendering.png" height="128px" align="right">
This demo shows how to create an index buffer resource, copy the index data to the GPU's Default Heap for long term storage, and create an Index Buffer View to be used with the Input Assembler.

Additional Features:
* Precompiled shaders
* sRGB Render Target View for gamma correction
* CommandList resuse

## [InstanceRendering](InstanceRendering/)
Makes use of a constant upload heap buffer to manage per instance data for rendering multiple instances of a single mesh.  
