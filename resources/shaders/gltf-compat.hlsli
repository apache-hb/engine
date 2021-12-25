#if HAS_NORMAL
#   define NORMAL_ARG(id) , float3 id : NORMAL
#   define GET_NORMAL(id) id
#else
#   define NORMAL_ARG(id)
#   define GET_NORMAL(id) float3(0.f, 0.f, 0.f)
#endif


#if HAS_TANGENT
#   define TANGENT_ARG(id) , float4 id : TANGENT
#   define GET_TANGENT(id) id
#else
#   define TANGENT_ARG(id)
#   define GET_TANGENT(id) float4(0.f, 0.f, 0.f, 0.f)
#endif


#if HAS_TEXCOORD
#   define TEXCOORD_ARG(id) , float2 id : TEXCOORD
#   define GET_TEXCOORD(id) id
#else
#   define TEXCOORD_ARG(id)
#   define GET_TEXCOORD(id) float2(0.f, 0.f)
#endif
