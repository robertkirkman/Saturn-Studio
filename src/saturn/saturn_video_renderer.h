#ifndef SaturnVideoRenderer
#define SaturnVideoRenderer

#include <tuple>

#define FUNC(args...) void(*args

#define FUNC_INIT       )(int w, int h)
#define FUNC_RENDER     )(unsigned char* data)
#define FUNC_FINALIZE   )()

typedef std::tuple<FUNC(FUNC_INIT), FUNC(FUNC_RENDER), FUNC(FUNC_FINALIZE)> VideoRenderer;

extern FUNC(video_renderer_init      FUNC_INIT    );
extern FUNC(video_renderer_render    FUNC_RENDER  );
extern FUNC(video_renderer_finalize  FUNC_FINALIZE);

extern void saturn_set_video_renderer(int index);

#endif
