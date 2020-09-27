#include <iostream>

using namespace std;

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libswresample/swresample.h>

}


#pragma comment(lib,"avformat.lib")
#pragma comment(lib,"avcodec.lib")
#pragma comment(lib,"avutil.lib")
#pragma comment(lib,"swresample.lib")



int main()
{
	av_register_all();

	avcodec_register_all();

	int ret = 0;

	char inputfile[] = "audio.pcm";
	char outputfile[] = "audio.aac";

	AVCodec* codec = avcodec_find_encoder(AV_CODEC_ID_AAC);
	if (!codec)
	{
		cout << "avcodec_find_encoder failed" << endl;
		return -1;
	}

	//���ñ�����������
	AVCodecContext* ac = avcodec_alloc_context3(codec);
	if (!ac){
		cout << "avcodec_alloc_context3 failed" << endl;
		return -1;
	}

	ac->sample_rate = 44100;
	ac->channels = 2;
	ac->channel_layout = AV_CH_LAYOUT_STEREO;

	ac->sample_fmt = AV_SAMPLE_FMT_FLTP;
	ac->bit_rate = 64000;

	ac->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

	//����Ƶ�ı�����
	ret = avcodec_open2(ac, codec,NULL);
	if (ret < 0)
	{
		cout << "avcodec_open2 failed" << endl;
		return -1;
	}

	//����һ�������������
	AVFormatContext* oc = NULL;
	avformat_alloc_output_context2(&oc, NULL, NULL, outputfile);
	if (!oc)
	{
		cout << "avformat_alloc_output_context2 failed" << endl;
		return -1;
	}


	AVStream* st = avformat_new_stream(oc, NULL);
	st->codecpar->codec_tag = 0;

	avcodec_parameters_from_context(st->codecpar, ac);

	av_dump_format(oc,0,outputfile,1);


	ret = avio_open(&oc->pb, outputfile, AVIO_FLAG_WRITE);
	if (ret < 0)
	{
		cout << "avio_open failed" << endl;
		return -1;
	}

	avformat_write_header(oc, NULL);


	SwrContext* ctx = NULL;
	ctx = swr_alloc_set_opts(ctx, ac->channel_layout, ac->sample_fmt, ac->sample_rate,//�������Ƶ����
		AV_CH_LAYOUT_STEREO, AV_SAMPLE_FMT_S16, 44100,//�������Ƶ����
		0, 0

		);
	if (!ctx)
	{
		cout << "swr_alloc_set_opts failed" << endl;
		return -1;
	}

	ret = swr_init(ctx);
	if (ret < 0)
	{
		cout << "swr_init failed" << endl;
		return -1;
	}



	AVFrame* frame = av_frame_alloc();
	frame->format = AV_SAMPLE_FMT_FLTP;
	frame->channels = 2;
	frame->channel_layout = AV_CH_LAYOUT_STEREO;
	frame->nb_samples = 1024;//һ֡��Ƶ����������

	ret = av_frame_get_buffer(frame, 0);
	if (ret < 0) {
		cout << "av_frame_get_buffer failed" << endl;
		return -1;
	}

	int readSize = frame->nb_samples * 2 * 2;
	char* pcms = new char[readSize];

	//��������ļ�
	FILE* fp = fopen(inputfile, "rb");

	for (;;)
	{
		int len = fread(pcms, 1, readSize, fp);
		if (len <= 0)
		{
			break;
		}

		//�ز���֮ǰ������
		const uint8_t* data[1];
		data[0] = (uint8_t*)pcms;
		

		len = swr_convert(ctx, frame->data, frame->nb_samples, //�ز���֮�������
						data, frame->nb_samples  //�ز���֮ǰ������
		);
		
		if (len <= 0)
		{
			break;
		}



		AVPacket pkt;
		av_init_packet(&pkt);

		//���ز��������ݷ��͵������߳�
		ret = avcodec_send_frame(ac, frame);
		if (ret != 0) {
			continue;
		}


		ret = avcodec_receive_packet(ac, &pkt);
		if (ret != 0)
		{
			continue;
		}

		//0��ʾ��Ƶ��
		pkt.stream_index = 0;
		pkt.dts = 0;
		pkt.pts = 0;

		av_interleaved_write_frame(oc,&pkt);

		cout << len << ",";



	}


	delete pcms;
	pcms = NULL;

	//д����Ƶ������
	av_write_trailer(oc);

	//�رմ򿪵��ļ���IO��
	avio_close(oc->pb);

	//�رձ�����
	avcodec_close(ac);

	avcodec_free_context(&ac);

	avformat_free_context(oc);

	return 0;

}
