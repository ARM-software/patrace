#ifndef _TRACER_INTERACTIVE_CMD_H_
#define _TRACER_INTERACTIVE_CMD_H_

enum INTERATIVE_CMD {
    UNKNOWN_CMD = 0,
    TO_FRAME,        // run until we hit a given frame
    SNAP_FRAME,      // capture the final framebuffer image
    SNAP_DRAW_FRAME, // capture all draw calls for a frame
    SNAP_ALL_FRAMES_NOWAIT // snap every N frames
};

struct InteractiveCmd {
    INTERATIVE_CMD      cmd;
    unsigned int        frameNo;
    unsigned int        frameRate;
    unsigned int        requiredTid;
    const char*         nameCString;
    static const char* names[];

    InteractiveCmd():
        cmd(UNKNOWN_CMD)
        ,frameNo(0)
        ,frameRate(0)
        ,requiredTid(0)
        ,nameCString(0x0)
    {}
};

InteractiveCmd GetInteractiveCmd();


#endif

