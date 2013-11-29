// Shaped Bokeh Depth of Field
// CoC Calculations based off Knu's DoF shader for MGE
// Blurring Algorithm Created by Tomerk
 
 
// **
// ** ADJUSTABLE VARIABLES
 
#define SMOOTHBLUR // when on, smooths the blur, may have a performance hit
 
static const float fr = 30.0; // retina focus point, dpt
// set slightly lower than fp to simulate myopia or as an alternative to MGE's distant blur
// set slightly higher than fp+fpa to simulate hyperopia
 
static const float fp = 60.0; // eye relaxed focus power, dpt
static const float fpa = 10.0; // accomodation, dpt
// set lower to simulate presbyopia
 
static const float base_blur_radius = 1.5; // base blur radius;
// higher values mean more blur when out of DoF and shorter DoF. 
 
static const float R = 16.0; // maximum blur radius in pixels;
 
static const float EdgeWeighting = 0; //0 has constant weighting in highlights,
//1 gives edges more weight in highlights
 
// ** END OF
// **
 
static float k = 0.00001;
 
float2 rcpres;
 
float4x4 m44proj;
static const float nearZ = m44proj._43 / m44proj._33;
static const float farZ = (m44proj._33 * nearZ) / (m44proj._33 - 1.0f);
static const float depthRange = (nearZ-farZ)*0.01;
 
 
 
texture OriginalScene;
texture CurrentScene;
texture DepthScene;
 
sampler frameSampler = sampler_state {texture = <CurrentScene>;	AddressU = Mirror;	AddressV = Mirror;	FILTER = MIN_MAG_MIP_LINEAR; };
sampler depthSampler = sampler_state {texture = <DepthScene>;	AddressU = CLAMP;	AddressV = CLAMP;	FILTER = MIN_MAG_MIP_LINEAR; };
sampler passSampler = sampler_state {texture= <OriginalScene>; AddressU=Mirror; AddressV=Mirror; FILTER = MIN_MAG_MIP_LINEAR; };
 
struct VSOUT
{
	float4 vertPos : POSITION;
	float2 UVCoord : TEXCOORD0;
};
 
struct VSIN
{
	float4 vertPos : POSITION0;
	float2 UVCoord : TEXCOORD0;
};
 
VSOUT FrameVS(VSIN IN)
{
	VSOUT OUT = (VSOUT)0.0f;
	OUT.vertPos = IN.vertPos;
	OUT.UVCoord = IN.UVCoord;
	return OUT;
}
 
float readDepth(in float2 coord : TEXCOORD0)
{
	float posZ = DepthScene.Sample(depthSampler, coord).x;
	return (2.0f * nearZ) / (nearZ + farZ - posZ * (farZ - nearZ));
}
 
static float focus = readDepth(float2(0.5,0.5));
 
#define M 60
static float2 taps[M] =
{
float2( 0.0000, 0.2500 ),
float2( -0.2165, 0.1250 ),
float2( -0.2165, -0.1250 ),
float2( -0.0000, -0.2500 ),
float2( 0.2165, -0.1250 ),
float2( 0.2165, 0.1250 ),
float2( 0.0000, 0.5000 ),
float2( -0.2500, 0.4330 ),
float2( -0.4330, 0.2500 ),
float2( -0.5000, 0.0000 ),
float2( -0.4330, -0.2500 ),
float2( -0.2500, -0.4330 ),
float2( -0.0000, -0.5000 ),
float2( 0.2500, -0.4330 ),
float2( 0.4330, -0.2500 ),
float2( 0.5000, -0.0000 ),
float2( 0.4330, 0.2500 ),
float2( 0.2500, 0.4330 ),
float2( 0.0000, 0.7500 ),
float2( -0.2565, 0.7048 ),
float2( -0.4821, 0.5745 ),
float2( -0.6495, 0.3750 ),
float2( -0.7386, 0.1302 ),
float2( -0.7386, -0.1302 ),
float2( -0.6495, -0.3750 ),
float2( -0.4821, -0.5745 ),
float2( -0.2565, -0.7048 ),
float2( -0.0000, -0.7500 ),
float2( 0.2565, -0.7048 ),
float2( 0.4821, -0.5745 ),
float2( 0.6495, -0.3750 ),
float2( 0.7386, -0.1302 ),
float2( 0.7386, 0.1302 ),
float2( 0.6495, 0.3750 ),
float2( 0.4821, 0.5745 ),
float2( 0.2565, 0.7048 ),
float2( 0.0000, 1.0000 ),
float2( -0.2588, 0.9659 ),
float2( -0.5000, 0.8660 ),
float2( -0.7071, 0.7071 ),
float2( -0.8660, 0.5000 ),
float2( -0.9659, 0.2588 ),
float2( -1.0000, 0.0000 ),
float2( -0.9659, -0.2588 ),
float2( -0.8660, -0.5000 ),
float2( -0.7071, -0.7071 ),
float2( -0.5000, -0.8660 ),
float2( -0.2588, -0.9659 ),
float2( -0.0000, -1.0000 ),
float2( 0.2588, -0.9659 ),
float2( 0.5000, -0.8660 ),
float2( 0.7071, -0.7071 ),
float2( 0.8660, -0.5000 ),
float2( 0.9659, -0.2588 ),
float2( 1.0000, -0.0000 ),
float2( 0.9659, 0.2588 ),
float2( 0.8660, 0.5000 ),
float2( 0.7071, 0.7071 ),
float2( 0.5000, 0.8660 ),
float2( 0.2588, 0.9659 )
};
 
 
float4 dof( float2 tex : TEXCOORD0 ) : COLOR0
{
	float s = focus*depthRange;
	float depth = readDepth(tex);
	float z = depth*depthRange;
 
	float fpf = clamp(1 / s + fr, fp, fp + fpa);
	float c = base_blur_radius * 0.009 * (fr - fpf + 1 / z) / fr / k;
	c = sign(z-s) * min(abs(c), R) / (2 * R);
 
	c += 0.5;
 
	return float4(c, 1, 1, 1);
}
 
float4 smartblur( float2 tex : TEXCOORD0 ) : COLOR0
{
    float4 color = OriginalScene.Sample(frameSampler, tex);
    float c = 2 * R * (CurrentScene.Sample(passSampler, tex).r - 0.5);
 
    float weight = (1/(c*c+1))*dot(color.rgb + 0.01, float3(0.2126,0.7152,0.0722) );
    if (EdgeWeighting)
        weight *= 0.25;
 
    color *= weight;
    float amount = weight;
 
    for (int i = 0; i < M; i++)
    {
 
        float2 dir = taps[i];
 
        float2 s_tex = tex + rcpres * dir * c;
        float4 s_color = OriginalScene.Sample(frameSampler, s_tex);
        float2 s_c_sample = CurrentScene.Sample(passSampler, s_tex).rg;
        float s_c = abs(( s_c_sample.r - 0.5) * 2 * R);
 
		if (c < 0)
			s_c = max(abs(c),s_c)*s_c_sample.g;
 
        weight = (1/(s_c*s_c+1))*dot(s_color.rgb+0.01, float3(0.2126,0.7152,0.0722))*(1-smoothstep(s_c, s_c*1.1, length(dir)*abs(c)));
 
		if (EdgeWeighting)
			weight *= saturate( 0.25 + 0.75*pow(length(dir)*c / (s_c), 2) );
 
        color += s_color * weight;
        amount += weight;
    }
 
    return float4(color.rgb / amount, 1);
}
 
float4 posthorblur( float2 tex : TEXCOORD0 ) : COLOR0
{
    float s = focus*depthRange;
    float z = readDepth(tex)*depthRange;
 
    float fpf = clamp(1 / s + fr, fp, fp + fpa);
    float c = base_blur_radius * 0.009 * (fr - fpf + 1 / z) / fr / k;
    c = min(abs(c), R);
 
	clip(c-0.001);
 
	float scale = c / 8;
 
	float4 color = CurrentScene.Sample(passSampler, tex) * 6;
 
	color += CurrentScene.Sample(passSampler, float2(tex.x-(rcpres.x)*scale, tex.y)) * 4;
	color += CurrentScene.Sample(passSampler, float2(tex.x+(rcpres.x)*scale, tex.y)) * 4;
 
	color += CurrentScene.Sample(passSampler, float2(tex.x-(rcpres.x*2)*scale, tex.y)) * 1;
	color += CurrentScene.Sample(passSampler, float2(tex.x+(rcpres.x*2)*scale, tex.y)) * 1;
 
	return color / 16;
}
 
 
float4 postvertblur( float2 tex : TEXCOORD0 ) : COLOR0
{
    float s = focus*depthRange;
    float z = readDepth(tex)*depthRange;
 
    float fpf = clamp(1 / s + fr, fp, fp + fpa);
    float c = base_blur_radius * 0.009 * (fr - fpf + 1 / z) / fr / k;
    c = min(abs(c), R);
 
	clip(c-0.001);
 
	float scale = c / 8;
 
	float4 color = CurrentScene.Sample(passSampler, tex) * 6;
 
	color += CurrentScene.Sample(passSampler, float2(tex.x, tex.y-(rcpres.y)*scale )) * 4;
	color += CurrentScene.Sample(passSampler, float2(tex.x, tex.y+(rcpres.y)*scale )) * 4;
 
	color += CurrentScene.Sample(passSampler, float2(tex.x, tex.y-(rcpres.y*2)*scale )) * 1;
	color += CurrentScene.Sample(passSampler, float2(tex.x, tex.y+(rcpres.y*2)*scale )) * 1;
 
	return color / 16;
}
 
 
technique11 T0
{ 
	pass p0
	{
		VertexShader = compile vs_5_0 FrameVS();
		PixelShader = compile ps_5_0 dof();
	}
 
	pass p1
	{
		PixelShader = compile ps_5_0 smartblur();
	}
	#ifdef SMOOTHBLUR
	pass p2
	{
		PixelShader = compile ps_5_0 posthorblur();
	}
	pass p3
	{
		PixelShader = compile ps_5_0 postvertblur();
	}
	#endif
}
