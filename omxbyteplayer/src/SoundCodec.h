#ifndef _SoundCodec_h
#define _SoundCodec_h

#define _USE_MATH_DEFINES
#include <cmath>
#include <string.h>

#include <vector>
#include <stdlib.h>

using namespace std;

struct floatItem
{
	int index;
	float value;
};

static int sgnf(float x)
{
    if (x>0) return(1); 
	if (x<0) return(-1); 
    return(0); 

}

static int comp_floatItem(const void *a, const void *b)
{
	float delta= ((floatItem*)a)->value-((floatItem*)b)->value;
	return -sgnf(delta);
}

struct SoundCodec
{
	float m_sampleRate;
	float m_f[32];
	float Enc_Amp;
	
	vector<unsigned char> m_dataBuffer;
	vector<short> m_PCMBuffer;

	int Dec_Window;

	int GetEncPulse()
	{
		return Dec_Window*4.0;
	}

	float Dec_Abs_Thresh;
	float Dec_Rel_Thresh;
	float Dec_Echo_Factor;

	SoundCodec()
	{
		m_sampleRate=44100.0f;
		m_f[0]=1568.0f;

		float Factor=powf(2.0f,(1.0f/12.0f));
		int i;
		for (i=1;i<32;i++)
			m_f[i]=m_f[i-1]*Factor;

		Enc_Amp=15000.0f;

		Dec_Window=2048;

		Dec_Echo_Factor=0.0f;

		Dec_Abs_Thresh=50.0f;
		Dec_Rel_Thresh=2.0f;
		
	}

	float GetW(int i)
	{
		return 2.0f*(float)M_PI*m_f[i]/m_sampleRate;
	}

	void WriteSignal(float WA, float WB, float& PhaseA, float& PhaseB, int len)
	{
		int i;
		for (i=0;i<len;i++)
		{
			short v=(short)(Enc_Amp*(cosf(PhaseA)+cosf(PhaseB)) * powf(sinf((float)i*M_PI/(float)len),4.0f));
			m_PCMBuffer.push_back(v);
			PhaseA+=WA;
			PhaseB+=WB;
		}
	}

	void WriteByte(unsigned char byte, float& PhaseA, float& PhaseB)
	{
		int low=byte%16;
		int high=byte/16;

		float WA=GetW(low);
		float WB=GetW(high+16);

		WriteSignal(WA,WB,PhaseA,PhaseB,GetEncPulse());
	}

    void Encode()
	{
		m_PCMBuffer.clear();
		float PhaseA=(float)M_PI*0.5f;
		float PhaseB=(float)M_PI*0.5f;

		int i;
		for (i=0;i<m_dataBuffer.size();i++)
			WriteByte(m_dataBuffer[i],PhaseA,PhaseB);

	}

	bool Decode()
	{
		m_dataBuffer.clear();
		
		unsigned regB=0;
		unsigned regC=0;

		floatItem V2[32];
		memset(V2,0,sizeof(floatItem)*32);

		int i;
		for ( i=0; m_PCMBuffer.size()-i>Dec_Window; i+=Dec_Window)
		{
			floatItem V[32];

			int j;
			for (j=0;j<32;j++)
			{
				V[j].index=j;

				float acc0=0.0f;
				float acc1=0.0f;
				float phase=0;
				float w=GetW(j);

				int k;
				for (k=i;k<i+Dec_Window;k++,phase+=w)
				{
					acc0+=cosf(phase)*(float)m_PCMBuffer[k];
					acc1+=sinf(phase)*(float)m_PCMBuffer[k];
				}
				V[j].value=sqrtf(acc0*acc0+acc1*acc1)/(float)Dec_Window;		
			}

			// echo remove
			
			for (j=0;j<32;j++)
			{
				float vv=V2[j].value;
				V2[j].value=V[j].value;
			    V[j].value-=vv*Dec_Echo_Factor;				
			}
			
			qsort(V,32,sizeof(floatItem),comp_floatItem);


			if (V[1].value>max(Dec_Abs_Thresh,V[2].value*Dec_Rel_Thresh))
			{				
				int index[2]={-1,-1};
				int groupID=V[0].index/16;
				int ID=V[0].index%16;
				index[groupID]=ID;

				groupID=V[1].index/16;
				ID=V[1].index%16;
				index[groupID]=ID;

				if (index[0]>=0 && index[1]>=0)
				{
					unsigned B=index[0]+index[1]*16;
					if (B==regB) regC++;
					else
					{
						regB=B;
						regC=1;
					}
					if (regC==3)
						regC=0;

					if (regC==2)
					{
						//printf("%c\n",B);
						m_dataBuffer.push_back((unsigned char)B);
					}
				}
				else
				{
					regC=0;
				}				

			}	
			else
			{
				//printf("%f (%d,%f) (%d,%f) (%d,%f) (%d,%f)\n",accPow,V[0].index,V[0].value,V[1].index,V[1].value,V[2].index,V[2].value,V[3].index,V[3].value);
				regC=0;
			}	

		}

		return true;
	}

};

#endif 