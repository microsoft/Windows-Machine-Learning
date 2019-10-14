RWStructuredBuffer<float> src : register(u0);
RWStructuredBuffer<float> dst : register(u1);

cbuffer Constants
{
    uint srcStartIndex;
    uint srcElementCount;
};

[numthreads(64, 1, 1)]
void Relu(uint3 dtid : SV_DispatchThreadId)
{
    uint index = srcStartIndex + dtid.x;

    if (index < srcElementCount)
    {
        dst[index] = max(0, src[index]);
    }
}