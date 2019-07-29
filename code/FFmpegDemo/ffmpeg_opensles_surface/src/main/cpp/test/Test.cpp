//
// Created by CaiGao on 2019/7/29.
//

#include "Test.h"
#include "Reference.h"

Reference *pReference;

void reference_test() {
    pReference = new Reference();
    NativePlayerContext *ctx;
    ctx = new NativePlayerContext();
    pReference->initP(ctx);
    pReference->init(*ctx);
    ctx->play_state = 111111;
    pReference->print(*ctx);

    /*NativePlayerContext ctx;
    ctx = NativePlayerContext();
    pReference->initP(&ctx);
    pReference->init(ctx);
    ctx.play_state = 111111;
    pReference->print(ctx);*/
}

void Test::run_test_case() {
    reference_test();
}
