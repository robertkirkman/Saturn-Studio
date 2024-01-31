#include "saturn_video_renderer.h"

#include <iostream>
#include <filesystem>
#include <cstdio>
#include <string>

extern "C" {
#include "pc/pngutils.h"
}

void pngseq_finalize() {}

int video_width;
int video_height;
int png_counter;

FILE* ffmpeg;

void pngseq_init(int w, int h) {
    video_width = w;
    video_height = h;
    png_counter = 0;
    if (std::filesystem::exists("pngseq")) std::filesystem::remove_all("pngseq");
    std::filesystem::create_directory("pngseq");
}

void pngseq_render(unsigned char* data) {
    pngutils_write_png(("pngseq/" + std::to_string(++png_counter) + ".png").c_str(), video_width, video_height, 4, data, 0);
}

void webm_init(int w, int h) {
    video_width = w;
    video_height = h;
    std::string cmd = "ffmpeg -y -r 30 -f rawvideo -pix_fmt rgba -s " + std::to_string(w) + "x" + std::to_string(h) + " -i - -c:v libvpx-vp9 -pix_fmt yuva420p video.webm";
    ffmpeg = popen(cmd.c_str(), "wb");
}

void mp4_init(int w, int h) {
    video_width = w;
    video_height = h;
    std::string cmd = "ffmpeg -y -r 30 -f rawvideo -pix_fmt rgba -s " + std::to_string(w) + "x" + std::to_string(h) + " -i - -c:v h264 -pix_fmt yuv420p video.mp4";
    ffmpeg = popen(cmd.c_str(), "wb");
}

void ffmpeg_render(unsigned char* data) {
    fwrite(data, video_width * video_height * 4, 1, ffmpeg);
}

void ffmpeg_finalize() {
    fclose(ffmpeg);
}

VideoRenderer renderer_pngseq = {
    pngseq_init,
    pngseq_render,
    pngseq_finalize,
};

VideoRenderer renderer_webm = {
    webm_init,
    ffmpeg_render,
    ffmpeg_finalize,
};

VideoRenderer renderer_mp4 = {
    mp4_init,
    ffmpeg_render,
    ffmpeg_finalize,
};

VideoRenderer video_renderers[] =  {
    renderer_pngseq,
    renderer_webm,
    renderer_mp4,
};

FUNC(video_renderer_init      FUNC_INIT    ) = nullptr;
FUNC(video_renderer_render    FUNC_RENDER  ) = nullptr;
FUNC(video_renderer_finalize  FUNC_FINALIZE) = nullptr;

void saturn_set_video_renderer(int index) {
    video_renderer_init     = std::get<0>(video_renderers[index]);
    video_renderer_render   = std::get<1>(video_renderers[index]);
    video_renderer_finalize = std::get<2>(video_renderers[index]);
}