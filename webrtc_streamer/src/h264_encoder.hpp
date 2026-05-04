#pragma once

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

#include <opencv2/opencv.hpp>
#include <vector>
#include <stdexcept>

class H264Encoder {
public:
    H264Encoder(int width, int height, int fps = 30, int bitrate = 2000000) 
        : width(width), height(height) {
        
        const AVCodec* codec = avcodec_find_encoder(AV_CODEC_ID_H264);
        if (!codec) throw std::runtime_error("H.264 codec not found");

        codecContext = avcodec_alloc_context3(codec);
        if (!codecContext) throw std::runtime_error("Could not allocate video codec context");

        codecContext->bit_rate = bitrate;
        codecContext->width = width;
        codecContext->height = height;
        codecContext->time_base = {1, fps};
        codecContext->framerate = {fps, 1};
        codecContext->gop_size = 10;
        codecContext->max_b_frames = 0;
        codecContext->pix_fmt = AV_PIX_FMT_YUV420P;

        av_opt_set(codecContext->priv_data, "preset", "ultrafast", 0);
        av_opt_set(codecContext->priv_data, "tune", "zerolatency", 0);

        if (avcodec_open2(codecContext, codec, NULL) < 0) {
            throw std::runtime_error("Could not open codec");
        }

        frame = av_frame_alloc();
        frame->format = codecContext->pix_fmt;
        frame->width = codecContext->width;
        frame->height = codecContext->height;

        if (av_frame_get_buffer(frame, 32) < 0) {
            throw std::runtime_error("Could not allocate the video frame data");
        }

        swsContext = sws_getContext(width, height, AV_PIX_FMT_BGR24,
                                    width, height, AV_PIX_FMT_YUV420P,
                                    SWS_BICUBIC, NULL, NULL, NULL);
    }

    ~H264Encoder() {
        avcodec_free_context(&codecContext);
        av_frame_free(&frame);
        sws_freeContext(swsContext);
    }

    std::vector<uint8_t> encode(const cv::Mat& bgrMat) {
        // Convert BGR to YUV420P
        const int stride[] = { static_cast<int>(bgrMat.step[0]) };
        sws_scale(swsContext, &bgrMat.data, stride, 0, height, frame->data, frame->linesize);
        
        frame->pts = frameCount++;

        int ret = avcodec_send_frame(codecContext, frame);
        if (ret < 0) return {};

        std::vector<uint8_t> packetData;
        AVPacket* pkt = av_packet_alloc();
        while (ret >= 0) {
            ret = avcodec_receive_packet(codecContext, pkt);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) break;
            if (ret < 0) break;

            packetData.insert(packetData.end(), pkt->data, pkt->data + pkt->size);
            av_packet_unref(pkt);
        }
        av_packet_free(&pkt);
        return packetData;
    }

    int getWidth() const { return width; }
    int getHeight() const { return height; }

private:
    AVCodecContext* codecContext = nullptr;
    AVFrame* frame = nullptr;
    SwsContext* swsContext = nullptr;
    int width, height;
    int frameCount = 0;
};
