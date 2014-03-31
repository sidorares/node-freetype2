#include <nan.h>
#include "fontface.h"

using namespace v8;

void Init(Handle<Object> exports) {
  FontFace::Init(exports);
}

NODE_MODULE(freetype2, Init)
