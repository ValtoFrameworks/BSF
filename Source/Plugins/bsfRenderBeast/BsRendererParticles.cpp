//************************************ bs::framework - Copyright 2018 Marko Pintera **************************************//
//*********** Licensed under the MIT license. See LICENSE.md for full terms. This notice is not to be removed. ***********//
#include "BsRendererParticles.h"
#include "Particles/BsParticleManager.h"
#include "Renderer/BsRendererUtility.h"
#include "RenderAPI/BsGpuBuffer.h"
#include "RenderAPI/BsVertexBuffer.h"
#include "Mesh/BsMeshData.h"
#include "RenderAPI/BsVertexDataDesc.h"

namespace bs { namespace ct
{
	template<bool LOCK_Y, bool GPU>
	const ShaderVariation& _getParticleShaderVariation(ParticleOrientation orient)
	{
		switch (orient)
		{
		default:
		case ParticleOrientation::ViewPlane:
			return getParticleShaderVariation<ParticleOrientation::ViewPlane, LOCK_Y, GPU>();
		case ParticleOrientation::ViewPosition:
			return getParticleShaderVariation<ParticleOrientation::ViewPosition, LOCK_Y, GPU>();
		case ParticleOrientation::Plane:
			return getParticleShaderVariation<ParticleOrientation::Plane, LOCK_Y, GPU>();
		}
	}

	template<bool GPU>
	const ShaderVariation& _getParticleShaderVariation(ParticleOrientation orient, bool lockY)
	{
		if (lockY)
			return _getParticleShaderVariation<true, GPU>(orient);

		return _getParticleShaderVariation<false, GPU>(orient);
	}

	const ShaderVariation& getParticleShaderVariation(ParticleOrientation orient, bool lockY, bool gpu)
	{
		if(gpu)
			return _getParticleShaderVariation<true>(orient, lockY);

		return _getParticleShaderVariation<false>(orient, lockY);
	}

	ParticlesParamDef gParticlesParamDef;

	ParticleTexturePool::~ParticleTexturePool()
	{
		for (auto& sizeEntry : mBufferList)
		{
			for (auto& entry : sizeEntry.second.buffers)
				mAlloc.destruct(entry);
		}
	}

	const ParticleTextures* ParticleTexturePool::alloc(const ParticleCPUSimulationData& simulationData)
	{
		const UINT32 size = simulationData.color.getWidth();

		const ParticleTextures* output = nullptr;
		BuffersPerSize& buffers = mBufferList[size];
		if (buffers.nextFreeIdx < (UINT32)buffers.buffers.size())
		{
			output = buffers.buffers[buffers.nextFreeIdx];
			buffers.nextFreeIdx++;
		}

		if (!output)
		{
			output = createNewTextures(size);
			buffers.nextFreeIdx++;
		}

		// Populate texture contents
		// Note: Perhaps instead of using write-discard here, we should track which frame has finished rendering and then
		// just use no-overwrite? write-discard will very likely allocate memory under the hood.
		output->positionAndRotation->writeData(simulationData.positionAndRotation, 0, 0, true);
		output->color->writeData(simulationData.color, 0, 0, true);
		output->sizeAndFrameIdx->writeData(simulationData.sizeAndFrameIdx, 0, 0, true);

		const auto numParticles = (UINT32)simulationData.indices.size();
		if(numParticles > 0)
		{
			auto* const indices = (UINT32*)output->indices->lock(GBL_WRITE_ONLY_DISCARD);
			const UINT32 numRows = Math::divideAndRoundUp(numParticles, size);

			UINT32 idx = 0;
			UINT32 y = 0;
			for (; y < numRows - 1; y++)
			{
				for (UINT32 x = 0; x < size; x++)
					indices[idx++] = (x & 0xFFFF) | (y << 16);
			}

			// Final row
			const UINT32 remainingParticles = numParticles - (numRows - 1) * size;
			for (UINT32 x = 0; x < remainingParticles; x++)
				indices[idx++] = (x & 0xFFFF) | (y << 16);

			output->indices->unlock();
		}

		return output;
	}

	void ParticleTexturePool::clear()
	{
		for(auto& buffers : mBufferList)
			buffers.second.nextFreeIdx = 0;
	}

	ParticleTextures* ParticleTexturePool::createNewTextures(UINT32 size)
	{
		ParticleTextures* output = mAlloc.construct<ParticleTextures>();

		TEXTURE_DESC texDesc;
		texDesc.type = TEX_TYPE_2D;
		texDesc.width = size;
		texDesc.height = size;
		texDesc.usage = TU_DYNAMIC;

		texDesc.format = PF_RGBA32F;
		output->positionAndRotation = Texture::create(texDesc);

		texDesc.format = PF_RGBA8;
		output->color = Texture::create(texDesc);

		texDesc.format = PF_RGBA16F;
		output->sizeAndFrameIdx = Texture::create(texDesc);

		GPU_BUFFER_DESC bufferDesc;
		bufferDesc.type = GBT_STANDARD;
		bufferDesc.elementCount = size * size;
		bufferDesc.format = BF_16X2U;

		output->indices = GpuBuffer::create(bufferDesc);

		mBufferList[size].buffers.push_back(output);

		return output;
	}

	struct ParticleRenderer::Members
	{
		SPtr<VertexBuffer> billboardVB;
		SPtr<VertexDeclaration> billboardVD;
	};

	ParticleRenderer::ParticleRenderer()
		:m(bs_new<Members>())
	{
		SPtr<VertexDataDesc> vertexDesc = bs_shared_ptr_new<VertexDataDesc>();
		vertexDesc->addVertElem(VET_FLOAT3, VES_POSITION);
		vertexDesc->addVertElem(VET_FLOAT2, VES_TEXCOORD);

		m->billboardVD = VertexDeclaration::create(vertexDesc);

		VERTEX_BUFFER_DESC vbDesc;
		vbDesc.numVerts = 4;
		vbDesc.vertexSize = m->billboardVD->getProperties().getVertexSize(0);
		m->billboardVB = VertexBuffer::create(vbDesc);

		MeshData meshData(4, 0, vertexDesc);
		auto vecIter = meshData.getVec3DataIter(VES_POSITION);
		vecIter.addValue(Vector3(-0.5f, -0.5f, 0.0f));
		vecIter.addValue(Vector3(-0.5f, 0.5f, 0.0f));
		vecIter.addValue(Vector3(0.5f, -0.5f, 0.0f));
		vecIter.addValue(Vector3(0.5f, 0.5f, 0.0f));

		auto uvIter = meshData.getVec2DataIter(VES_TEXCOORD);
		uvIter.addValue(Vector2(0.0f, 1.0f));
		uvIter.addValue(Vector2(0.0f, 0.0f));
		uvIter.addValue(Vector2(1.0f, 1.0f));
		uvIter.addValue(Vector2(1.0f, 0.0f));

		m->billboardVB->writeData(0, meshData.getStreamSize(0), meshData.getStreamData(0), BWT_DISCARD);
	}

	ParticleRenderer::~ParticleRenderer()
	{
		bs_delete(m);
	}

	void ParticleRenderer::drawBillboards(UINT32 count)
	{
		SPtr<VertexBuffer> vertexBuffers[] = { m->billboardVB };

		RenderAPI& rapi = RenderAPI::instance();
		rapi.setVertexDeclaration(m->billboardVD);
		rapi.setVertexBuffers(0, vertexBuffers, 1);
		rapi.setDrawOperation(DOT_TRIANGLE_STRIP);
		rapi.draw(0, 4, count);
	}
}}
