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


	//寻找编码器
	AVCodec* codec = avcodec_find_encoder(AV_CODEC_ID_MP3);
	if (!codec)
	{
		cout << "avcodec_find_encoder failed" << endl;
		return -1;
	}

	//创建并配置编解码器上下文
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

	//打开编解码器
	ret = avcodec_open2(ctx, codec, 0);
	if (ret < 0) {
		cout << "avcodec_open2 failed" << endl;
		return -1;
	}

	//打开输出的文件流
	foutput = fopen(outputfile, "wb");
	if (!foutput)
	{
		cout << "avcodec_open2 failed" << endl;
		return -1;
	}


	//准备一个结构体来接受重采样的每一帧的音频数据,每帧的样本大小为1152。
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


	//创建音频重采样上下文
	SwrContext* swr = swr_alloc();
	if (!swr)
	{
		cout << "swr_alloc failed" << endl;
		return -1;
	}

	//设置重采样输入参数，通道布局立体声，采样率44100，样本格式不一致,输入S16交错存储，输出S16P平面存储
	av_opt_set_int(swr,"in_channel_layout",AV_CH_LAYOUT_STEREO,0);
	av_opt_set_int(swr, "in_sample_rate", 44100, 0);
	av_opt_set_sample_fmt(swr,"in_sample_fmt",AV_SAMPLE_FMT_S16,0);

	//设置重采样输出参数
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
	//存储从pcm文件读取过来的数据，进行缓存
	uint8_t** input_data = NULL;
	//缓存重采样之后的数据
	uint8_t * *output_data = NULL;
	int input_linesize, output_linesize;

	//给保存pcm文件数据分配空间
	ret = av_samples_alloc_array_and_samples(&input_data,&input_linesize,2,1152,AV_SAMPLE_FMT_S16,0);
	if (ret < 0) {
		cout << "av_samples_alloc_array_and_samples input failed" << endl;
		return -1;
	}
	//缓存重采样数据的空间分配
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
		//左声道数据
		frame->data[0] = output_data[0];

		//右声道数据
		frame->data[1] = output_data[1];


		//编码,写入mp3文件，实际上是对frame这个结构体里面的数据进行编码操作。
		ret = avcodec_send_frame(ctx, frame);
		if (ret < 0)
		{
			cout << "avcodec_send_frame failed" << endl;
			return -1;
		}

		while (ret >= 0) {

			ret = avcodec_receive_packet(ctx, pkt);

			//AVERROR(EEAGAIN) -11  AVERROR_EOF表示已经没有数据了
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