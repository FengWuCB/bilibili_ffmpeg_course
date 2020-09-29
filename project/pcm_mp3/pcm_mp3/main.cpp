#include <iostream>

using namespace std;

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/opt.h>
#include <libswresample/swresample.h>

}

#pragma comment(lib,"avcodec.lib")
#pragma comment(lib,"avformat.lib")
#pragma comment(lib,"avutil.lib")
#pragma comment(lib,"swresample.lib")


int main()
{
	av_register_all();

	char inputfile[] = "audio.pcm";
	char outputfile[] = "audio.mp3";
	int ret = 0;
	FILE* finput = NULL;
	FILE* foutput = NULL;


	//Ѱ�ұ�����
	AVCodec* codec = avcodec_find_encoder(AV_CODEC_ID_MP3);
	if (!codec)
	{
		cout << "avcodec_find_encoder failed" << endl;
		return -1;
	}

	//���������ñ������������
	AVCodecContext* ctx = NULL;
	ctx = avcodec_alloc_context3(codec);
	if (!ctx)
	{
		cout << "avcodec_alloc_context3 failed" << endl;
		return -1;
	}
	ctx->bit_rate = 64000;
	ctx->channels = 2;
	ctx->channel_layout = AV_CH_LAYOUT_STEREO;
	ctx->sample_rate = 44100;
	ctx->sample_fmt = AV_SAMPLE_FMT_S16P;

	//�򿪱������
	ret = avcodec_open2(ctx, codec, 0);
	if (ret < 0) {
		cout << "avcodec_open2 failed" << endl;
		return -1;
	}

	//��������ļ���
	foutput = fopen(outputfile, "wb");
	if (!foutput)
	{
		cout << "avcodec_open2 failed" << endl;
		return -1;
	}


	//׼��һ���ṹ���������ز�����ÿһ֡����Ƶ����,ÿ֡��������СΪ1152��
	AVFrame* frame;
	frame = av_frame_alloc();
	if (!frame)
	{
		cout << "av_frame_alloc failed" << endl;
		return -1;
	}
	frame->nb_samples = 1152;
	frame->channels = 2;
	frame->channel_layout = AV_CH_LAYOUT_STEREO;
	frame->format = AV_SAMPLE_FMT_S16P;

	ret = av_frame_get_buffer(frame, 0);
	if (ret < 0)
	{
		cout << "av_frame_get_buffer failed" << endl;
		return -1;
	}


	//������Ƶ�ز���������
	SwrContext* swr = swr_alloc();
	if (!swr)
	{
		cout << "swr_alloc failed" << endl;
		return -1;
	}

	//�����ز������������ͨ��������������������44100��������ʽ��һ��,����S16����洢�����S16Pƽ��洢
	av_opt_set_int(swr,"in_channel_layout",AV_CH_LAYOUT_STEREO,0);
	av_opt_set_int(swr, "in_sample_rate", 44100, 0);
	av_opt_set_sample_fmt(swr,"in_sample_fmt",AV_SAMPLE_FMT_S16,0);

	//�����ز����������
	av_opt_set_int(swr, "out_channel_layout", AV_CH_LAYOUT_STEREO, 0);
	av_opt_set_int(swr, "out_sample_rate", 44100, 0);
	av_opt_set_sample_fmt(swr, "out_sample_fmt", AV_SAMPLE_FMT_S16P, 0);

	ret = swr_init(swr);
	if (ret < 0) {
		cout << "swr_init failed" << endl;
		return -1;
	}




	finput = fopen(inputfile,"rb" );
	if (!finput)
	{
		cout << "fopen inputfile failed" << endl;
		return -1;
	}
	//�洢��pcm�ļ���ȡ���������ݣ����л���
	uint8_t** input_data = NULL;
	//�����ز���֮�������
	uint8_t * *output_data = NULL;
	int input_linesize, output_linesize;

	//������pcm�ļ����ݷ���ռ�
	ret = av_samples_alloc_array_and_samples(&input_data,&input_linesize,2,1152,AV_SAMPLE_FMT_S16,0);
	if (ret < 0) {
		cout << "av_samples_alloc_array_and_samples input failed" << endl;
		return -1;
	}
	//�����ز������ݵĿռ����
	ret = av_samples_alloc_array_and_samples(&output_data, &output_linesize, 2, 1152, AV_SAMPLE_FMT_S16P, 0);
	if (ret < 0) {
		cout << "av_samples_alloc_array_and_samples out failed" << endl;
		return -1;
	}


	AVPacket* pkt = av_packet_alloc();
	if (!pkt)
	{
		cout << "av_packet_alloc failed" << endl;
		return -1;
	}


	while (!feof(finput))
	{

		int readsize = fread(input_data[0],1,1152*2*2, finput);
		if (!readsize) {
			break;
		}
		cout << readsize << endl;


		ret = swr_convert(swr, output_data,1152, (const uint8_t**)input_data,1152);
		if (ret < 0)
		{
			cout << "swr_convert failed" << endl;
			return -1;
		}
		//����������
		frame->data[0] = output_data[0];

		//����������
		frame->data[1] = output_data[1];


		//����,д��mp3�ļ���ʵ�����Ƕ�frame����ṹ����������ݽ��б��������
		ret = avcodec_send_frame(ctx, frame);
		if (ret < 0)
		{
			cout << "avcodec_send_frame failed" << endl;
			return -1;
		}

		while (ret >= 0) {

			ret = avcodec_receive_packet(ctx, pkt);

			//AVERROR(EEAGAIN) -11  AVERROR_EOF��ʾ�Ѿ�û��������
			if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
			{
				continue;
			}
			else if (ret < 0) {
				break;
			}
			fwrite(pkt->data, 1, pkt->size, foutput);
			av_packet_unref(pkt);
		}
	
	}



	if (input_data)
	{
		av_freep(input_data);
	}
	if (output_data)
	{
		av_freep(output_data);
	}

	fclose(finput);
	fclose(foutput);


	av_frame_free(&frame);
	av_packet_free(&pkt);

	swr_free(&swr);

	avcodec_free_context(&ctx);

	return 0;
}