#include <iostream>

using namespace std;

//wav��ͷ����һ����
typedef struct WAV_HEADER {
	char chunkid[4];
	unsigned long chunksize;
	char format[4];

}pcmHeader;

//wav��ͷ���ڶ����֣��������ļ��Ĳ����ʡ�ͨ���������ʵȵ�һЩ��������
typedef struct WAV_FMT {
	char subformat[4];
	unsigned long sbusize;
	unsigned short audioFormat;
	unsigned short numchannels;
	unsigned long sampleRate;
	unsigned long byteRate;
	unsigned short blockAlign;
	unsigned short bitPerSample;
}pcmFmt;




typedef struct WAV_DATA {
	char wavdata[4];
	unsigned long dataSize;
}pcmData;



long getFileSize(char* filename)
{
	FILE* fp = fopen(filename,"r");
	if (!fp) {
		return -1;
	}

	fseek(fp, 0, SEEK_END);
	long size = ftell(fp);
	fclose(fp);

	return size;
}



int pcvToWav(const char* pcmpath, int channles, int sample_rate,int fmtsize, const char* wavpath)
{
	FILE* fp, * fpout;
	WAV_HEADER pcmHeader;
	WAV_FMT pcmFmt;
	WAV_DATA pcmData;

	int bits = 16;

	//��������ļ���
	fp=fopen(pcmpath, "rb");
	if (fp == NULL)
	{
		cout << "fopen failed" << endl;
		return -1;
	}

	//��һ����
	memcpy(pcmHeader.chunkid, "RIFF", strlen("RIFF"));//�ַ�REFF
	long fileSize = 44+getFileSize((char*)pcmpath)-8;
	pcmHeader.chunksize = fileSize;//wav�ļ�С
	memcpy(pcmHeader.format, "WAVE", strlen("WAVE"));//�ַ�WAVE


	//�ڶ�����
	memcpy(pcmFmt.subformat, "fmt ", strlen("fmt "));//�ַ���fmt 
	pcmFmt.sbusize = fmtsize;//�洢λ��
	pcmFmt.audioFormat = 1;//pcm���ݱ�ʶ
	pcmFmt.numchannels = channles; //ͨ����
	pcmFmt.sampleRate = sample_rate;//������
	pcmFmt.byteRate = sample_rate * channles * bits / 8;//ÿ�봫�ݵı�����

	pcmFmt.blockAlign = channles * bits / 8;
	pcmFmt.bitPerSample = bits;//������ʽ��16λ��


	//��������
	memcpy(pcmData.wavdata, "data", strlen("data"));//�ַ���data
	pcmData.dataSize = getFileSize((char*)pcmpath);//pcm���ݳ���


	//��������ļ���
	fpout = fopen(wavpath, "wb");
	if (fpout == NULL)
	{
		cout << "fopen failed" << endl;
		return -1;
	}

	//д��wavͷ����Ϣ44���ֽ�
	fwrite(&pcmHeader, sizeof(pcmHeader), 1, fpout);
	fwrite(&pcmFmt, sizeof(pcmFmt), 1, fpout);
	fwrite(&pcmData, sizeof(pcmData), 1, fpout);

	//��������ռ�
	char* buff = (char*)malloc(512);
	int len;

	//��ȡpcm����д��wav�ļ�
	while ((len = fread(buff, sizeof(char), 512, fp)) != 0) {
		fwrite(buff, sizeof(char), len, fpout);
	}

	free(buff);
	fclose(fp);
	fclose(fpout);


	
}




int main()
{
	pcvToWav("audio.pcm", 2, 44100,16, "outdio.wav");
	//cout << sizeof(long) << endl;
	//cout << sizeof(short) << endl;

	return 0;
}