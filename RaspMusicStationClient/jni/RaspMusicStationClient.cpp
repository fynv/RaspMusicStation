#include <jni.h>
#define _USE_MATH_DEFINES
#include <cmath>
#include <stdlib.h>

#define max(a,b) ((a) > (b) ? (a) : (b))

static const float W[32]={
	0.223402,
	0.236686,
	0.250760,
	0.265671,
	0.281469,
	0.298206,
	0.315938,
	0.334725,
	0.354629,
	0.375716,
	0.398058,
	0.421727,
	0.446805,
	0.473373,
	0.501521,
	0.531343,
	0.562939,
	0.596413,
	0.631877,
	0.669451,
	0.709258,
	0.751433,
	0.796116,
	0.843455,
	0.893610,
	0.946747,
	1.003043,
	1.062687,
	1.125878,
	1.192826,
	1.263755,
	1.338902
};

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

static const int Dec_Window=2048;
static const float Dec_Abs_Thresh=50.0f;
static const float Dec_Rel_Thresh=2.0f;

static short DecodeBuffer(jshort* array)
{
	floatItem V[32];

		int j;
		for (j=0;j<32;j++)
		{
			V[j].index=j;

			float acc0=0.0f;
			float acc1=0.0f;
			float phase=0;
			float w=W[j];

			int k;
			for (k=0;k<Dec_Window;k++,phase+=w)
			{
				acc0+=cosf(phase)*(float)array[k];
				acc1+=sinf(phase)*(float)array[k];
			}
			V[j].value=sqrtf(acc0*acc0+acc1*acc1)/(float)Dec_Window;
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
				return index[0]+index[1]*16;
			}
		}


	return -1;
}

extern "C" {
JNIEXPORT jint JNICALL Java_com_example_raspmusicstationclient_IPDecodeFromAudio_DecodeAudioBuffer
(JNIEnv *, jclass, jshortArray);
}

JNIEXPORT jint JNICALL Java_com_example_raspmusicstationclient_IPDecodeFromAudio_DecodeAudioBuffer
  (JNIEnv *env, jclass, jshortArray buffer)
{
	jshort *array;
	array = env->GetShortArrayElements( buffer, NULL);
	if(array == NULL){
		return -1;
	}

  short values[2];
  values[0]=DecodeBuffer(array);
  values[1]=DecodeBuffer(array+Dec_Window);

  env->ReleaseShortArrayElements(buffer, array, 0);

  int value=*(int*)(values);

  return value;
}


