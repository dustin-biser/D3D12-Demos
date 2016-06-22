# D3D12-Demos
A collection of demos using various aspects of the D3D12 API.

## [IndexRendering](IndexRendering/)
<img src="./Images/index_rendering.png" height="128px" align="right">
This demo shows how to create an index buffer resource, copy the index data to the GPU's Default Heap for long term storage, and create an Index Buffer View to be used with the Input Assembler.

Additional Features:
* Precompiled shaders
* sRGB Render Target View for gamma correction

## [ConstantBuffer](ConstantBuffer/)
<img src="./Images/constant_buffer.png" height="128px" align="right">
Uses an upload heap buffer to manage constant buffer data that is updated per frame.  
