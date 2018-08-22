#include "$ENGINE$/GpuParticleTileVertex.bslinc"

shader GpuParticleClear
{
	mixin GpuParticleTileVertex;
	
	code
	{	
		void fsmain(VStoFS input, 
			out float4 outPosAndTime : SV_Target0, 
			out float4 outVel : SV_Target1)
		{
			// Time > 1.0f means the particle is dead
			float time = 10.0f;
		
			outPosAndTime.xyz = 0.0f;
			outPosAndTime.w = time;
			
			outVel = 0.0f;		
		}
	};
};