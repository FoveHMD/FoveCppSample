// Constants buffer - these values are updated per frame
cbuffer Constants : register(b0) {
	float4x4 modelviewProjection; // This is the transform to take a point from world coordinates to normalized device coords
	float selection;              // This is the index of the currently selected object, or a negative number to indicate no selection
};

// Vertex input - these values are set per vertex from a model
struct VSI {
	float4 p : POSITION0; // x,y,z position. The w value is used to indicate what object this is part of
	float3 c : COLOR;     // r,g,b color. No alpha is used currently
};

// Vertex output - these values get interpolated across the polygon and passed to the frag shader
struct VSO {
	float4 p : SV_POSITION; // Output position
	float3 c : COLOR;       // Output color
};

// Entry point of the shader
VSO vert(VSI i) {
	VSO o;
	o.p = mul(float4(i.p.xyz, 1.0), modelviewProjection);             // Transform the input world position by the modelview & projection
	float selectColor = 1.5 * max(0.0, 0.5 - abs(selection - i.p.w)); // Determine if this vert belongs to the selected object (0 if not)
	o.c = i.c + float3(selectColor, selectColor, selectColor);        // Pass the color through, but also highlight if selected
	return o;
};
