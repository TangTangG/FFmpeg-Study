//
// Created by CaiGao on 2019/7/29.
//

#ifndef FFMPEGDEMO_REFERENCE_H
#define FFMPEGDEMO_REFERENCE_H


#include "../Data.h"

class Reference {
public:
    NativePlayerContext *pCtx;
    void initP(NativePlayerContext *ctx);
    void init(NativePlayerContext &ctx);
    void print(NativePlayerContext &ctx);
};


#endif //FFMPEGDEMO_REFERENCE_H
