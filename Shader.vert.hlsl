float4x4 modelView;
struct VSI {
	float4 p : POSITION0;
	float3 c : COLOR;
};
struct VSO {
	float4 p : SV_POSITION;
	float3 c : COLOR;
};

VSO vert(VSI i) {
	VSO ret;
	ret.p = mul(i.p, modelView);
	ret.c = i.c;
	return ret;
};
