//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//*********************************************************
#pragma once
#include "RaytracingHlslCompat.h"
#ifdef HLSL
#include "ShaderUtil.hlsli"
#endif

struct InputConstants
{
    uint NumberOfElements;
    uint MinTrianglesPerTreelet;
};

// CBVs
#define ConstantsRegister 0

// UAVs
#define HierarchyBufferRegister 0
#define NumTrianglesBufferRegister 1
#define AABBBufferRegister 2
#define ElementBufferRegister 3
#define BubbleBufferRegister 4

#define GlobalDescriptorHeapRegister 0
#define GlobalDescriptorHeapRegisterSpace 1

#ifdef HLSL
// These need to be UAVs despite being read-only because the fallback layer only gets a 
// GPU VA and the API doesn't allow any way to transition that GPU VA from UAV->SRV

globallycoherent RWStructuredBuffer<HierarchyNode> hierarchyBuffer : UAV_REGISTER(HierarchyBufferRegister);
RWByteAddressBuffer NumTrianglesBuffer : UAV_REGISTER(NumTrianglesBufferRegister);
globallycoherent RWStructuredBuffer<AABB> AABBBuffer : UAV_REGISTER(AABBBufferRegister);
RWStructuredBuffer<Primitive> InputBuffer : UAV_REGISTER(ElementBufferRegister);
RWByteAddressBuffer ReorderBubbleBuffer : UAV_REGISTER(BubbleBufferRegister);

cbuffer TreeletConstants : CONSTANT_REGISTER(ConstantsRegister)
{
    InputConstants Constants;
};

inline void SetBubbleBufferBit(uint nodeIndex)
{
	uint dwordByteIndex = ((nodeIndex / 8) / 4) * 4;
	uint byteIndex = ((nodeIndex % 32) / 8) * 8;
	uint bitIndex = nodeIndex & 0x7;

	uint previousValue;
	ReorderBubbleBuffer.InterlockedOr(dwordByteIndex , (1 << bitIndex) << (byteIndex * 8), previousValue);
}

inline void ClearBubbleBufferBit(uint nodeIndex)
{
	uint dwordByteIndex = ((nodeIndex / 8) / 4) * 4;
	uint byteIndex = ((nodeIndex % 32) / 8) * 8;
	uint bitIndex = nodeIndex % 8;

	uint previousValue;
	ReorderBubbleBuffer.InterlockedAnd(dwordByteIndex , ~((1 << bitIndex) << (byteIndex * 8)), previousValue);
}

inline uint ReadBubbleBuffer(uint readIndex)
{
	return ReorderBubbleBuffer.Load(readIndex * SizeOfUINT32);
}

inline bool BubbleBufferBitSet(uint nodeIndex)
{
	uint byteIndex = ((nodeIndex % 32) / 8) * 8;
	uint bitIndex = nodeIndex & 0x7;

	uint loaded = ReorderBubbleBuffer.Load((nodeIndex / 32) * SizeOfUINT32);
	uint byteMask = 0xff << byteIndex;
	uint byte = (loaded & byteMask) >> byteIndex;
	uint bitMask = 1 << bitIndex;
	uint bit = (byte & bitMask) >> bitIndex;
	return bit;
}

#endif