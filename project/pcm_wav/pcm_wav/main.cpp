#include <iostream>

using namespace std;

//wav的头部第一部分
typedef struct WAV_HEADER {
	char chunkid[4];
	unsigned long chunksize;
	char format[4];

}pcmHeader;

//wav的头部第二部分，保存着文件的采样率、通道数、码率等等一些基础参数
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

	//打开输入的文件流
	fp=fopen(pcmpath, "rb");
	if (fp == NULL)
	{
		cout << "fopen failed" << endl;
		return -1;
	}

	//第一部分
	memcpy(pcmHeader.chunkid, "RIFF", strlen("RIFF"));//字符REFF
	long fileSize = 44+getFileSize((char*)pcmpath)-8;
	pcmHeader.chunksize = fileSize;//wav文件小
	memcpy(pcmHeader.format, "WAVE", strlen("WAVE"));//字符WAVE


	//第二部分
	memcpy(pcmFmt.subformat, "fmt ", strlen("fmt "));//字符串fmt 
	pcmFmt.sbusize = fmtsize;//存储位宽
	pcmFmt.audioFormat = 1;//pcm数据标识
	pcmFmt.numchannels = channles; //通道数
	pcmFmt.sampleRate = sample_rate;//采样率
	pcmFmt.byteRate = sample_rate * channles * bits / 8;//每秒传递的比特数

	pcmFmt.blockAlign = channles * bits / 8;
	pcmFmt.bitPerSample = bits;//样本格式，16位。


	//第三部分
	memcpy(pcmData.wavdata, "data", strlen("data"));//字符串data
	pcmData.dataSize = getFileSize((char*)pcmpath);//pcm数据长度


	//打开输出的文件流
	fpout = fopen(wavpath, "wb");
	if (fpout == NULL)
	{
		cout << "fopen failed" << endl;
		return -1;
	}

	//写入wav头部信息44个字节
	fwrite(&pcmHeader, sizeof(pcmHeader), 1, fpout);
	fwrite(&pcmFmt, sizeof(pcmFmt), 1, fpout);
	fwrite(&pcmData, sizeof(pcmData), 1, fpout);

	//创建缓存空间
	char* buff = (char*)malloc(512);
	int len;

	//读取pcm数据写入wav文件
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