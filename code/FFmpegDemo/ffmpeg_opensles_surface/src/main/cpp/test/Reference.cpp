//
// Created by CaiGao on 2019/7/29.
//

#include "Reference.h"
#include "../util/FF_Log.h"

void Reference::initP(NativePlayerContext *ctx) {
pCtx = ctx;
    LOGD("0---%d",pCtx->play_state);
    ctx->play_state = 2222;
}

void Reference::init(NativePlayerContext &ctx) {
    LOGD("2---%d",ctx.play_state);

}

void Reference::print(NativePlayerContext &ctx) {
    LOGD("1---%d",pCtx->play_state);
    LOGD("3---%d",ctx.play_state);
}
