#include <nan.h>
#include <vector>
#include "fontface.h"

using namespace v8;

Persistent<FunctionTemplate> FontFace::constructor;

FontFace::FontFace(const FT_Byte* file_base, FT_Long file_size) {
  FT_New_Memory_Face(library, file_base, file_size, 0, &face);
}

FontFace::~FontFace() {
}

FT_Library FontFace::library;

void FontFace::Init(Handle<Object> target) {
  NanScope();

  if (FT_Init_FreeType(&library)) exit(EXIT_FAILURE);

  Local<FunctionTemplate> ctor = NanNew<FunctionTemplate>(New);
  ctor->InstanceTemplate()->SetInternalFieldCount(1);
  ctor->SetClassName(NanNew("FontFace"));

  NODE_SET_PROTOTYPE_METHOD(ctor, "render", Render);
  NODE_SET_PROTOTYPE_METHOD(ctor, "kerning", Kerning);
  NODE_SET_PROTOTYPE_METHOD(ctor, "hasKerning", hasKerning);

  // target->Set(NanNew("Canvas"), ctor->GetFunction());
  // proto->SetAccessor(NanNew("xxxx"), GetXxxx);

  target->Set(NanNew("FontFace"), ctor->GetFunction());
}

NAN_METHOD(FontFace::New) {
  NanScope();

  FontFace* obj = new FontFace(
    (FT_Byte*)node::Buffer::Data(args[0]->ToObject()),
    (FT_Long)node::Buffer::Length(args[0]->ToObject())
  );

  obj->SetObjectProperties(args.This());
  obj->Wrap(args.This());

  NanReturnValue(args.This());
}

// http://www.freetype.org/freetype2/docs/reference/ft2-base_interface.html#FT_FaceRec
void FontFace::SetObjectProperties(Handle<Object> obj) {
  obj->Set(NanNew("num_faces"), NanNew<Number>(this->face->num_faces));
  obj->Set(NanNew("face_index"), NanNew<Number>(this->face->face_index));

  obj->Set(NanNew("face_flags"), NanNew<Number>(this->face->face_flags));
  obj->Set(NanNew("style_flags"), NanNew<Number>(this->face->style_flags));

  obj->Set(NanNew("num_glyphs"), NanNew<Number>(this->face->num_glyphs));

  obj->Set(NanNew("family_name"), NanNew(this->face->family_name));
  obj->Set(NanNew("style_name"), NanNew(this->face->style_name));

  obj->Set(NanNew("num_fixed_sizes"), NanNew<Number>(this->face->num_fixed_sizes));
  // obj->Set(NanNew("available_sizes"), NanNew(this->face->available_sizes));

  obj->Set(NanNew("num_charmaps"), NanNew<Number>(this->face->num_charmaps));
  // obj->Set(NanNew("charmaps"), NanNew(this->face->charmaps));

  // obj->Set(NanNew("generic"), NanNew(this->face->generic));


  obj->Set(NanNew("units_per_EM"), NanNew<Number>(this->face->units_per_EM));
  // obj->Set(NanNew("bbox"), NanNew(this->face->bbox));

  obj->Set(NanNew("ascender"), NanNew<Number>(this->face->ascender));
  obj->Set(NanNew("descender"), NanNew<Number>(this->face->descender));
  obj->Set(NanNew("height"), NanNew<Number>(this->face->height));

  obj->Set(NanNew("max_advance_width"), NanNew<Number>(this->face->max_advance_width));
  obj->Set(NanNew("max_advance_height"), NanNew<Number>(this->face->max_advance_height));

  obj->Set(NanNew("underline_position"), NanNew<Number>(this->face->underline_position));
  obj->Set(NanNew("underline_thickness"), NanNew<Number>(this->face->underline_thickness));

  std::vector<FT_UInt> acv = this->AvailableCharacters();
  Local<Array> aca = NanNew<Array>(acv.size());
  for (size_t i = 0; i < acv.size(); i++) {
    aca->Set(i, NanNew(acv.at(i)));
  }
  obj->Set(NanNew("available_characters"), aca);
}

std::vector<FT_UInt> FontFace::AvailableCharacters() {
  FT_ULong charcode;
  FT_UInt gindex;
  std::vector<FT_UInt> charVect;

  charcode = FT_Get_First_Char(this->face, &gindex);
  while (gindex != 0) {
    charVect.push_back(charcode);
    charcode = FT_Get_Next_Char(this->face, charcode, &gindex);
  }

  return charVect;
}

NAN_METHOD(FontFace::hasKerning) {
  NanScope();
  FontFace *ff = ObjectWrap::Unwrap<FontFace>(args.This());
  NanReturnValue(NanNew<Boolean>(FT_HAS_KERNING(ff->face)));
}

NAN_METHOD(FontFace::Kerning) {
  NanScope();

  FT_Error error;

  FT_UInt left = (FT_UInt) args[0]->NumberValue();
  FT_UInt right = (FT_UInt) args[1]->NumberValue();

  FT_Int char_width  = args[1]->NumberValue();
  FT_Int char_height = args[2]->NumberValue();
  FT_Int xdpi   = args[3]->NumberValue();
  FT_Int ydpi   = args[4]->NumberValue();

  FontFace *ff = ObjectWrap::Unwrap<FontFace>(args.This());
  error = FT_Set_Char_Size( ff->face, char_width, char_height, xdpi, ydpi );

  FT_Vector  delta;

  FT_Select_Charmap(ff->face, FT_ENCODING_UNICODE);
  error = FT_Get_Kerning( ff->face, FT_Get_Char_Index(ff->face, left), FT_Get_Char_Index(ff->face, right),
    FT_KERNING_UNFITTED, &delta );

  Local<Object> res = NanNew<Object>();
  res->Set(NanNew("x"), NanNew<Number>(delta.x));
  res->Set(NanNew("y"), NanNew<Number>(delta.y));
  NanReturnValue(res);
}

NAN_METHOD(FontFace::Render) {
  FT_Error error;
  //FT_Face  face;
  FT_GlyphSlot slot;
  FT_Int width, height;

  NanScope();

  FontFace *ff = ObjectWrap::Unwrap<FontFace>(args.This());

  FT_UInt index = 0;
  FT_UInt charcode = (FT_UInt) args[0]->NumberValue();
  //if (args[0]->IsNumber())
   //index = FT_Get_Char_Index(ff->face, charcode);
   index = charcode;
  // else if (args[0]->IsString())
  //   String::Utf8Value str(args[0]->ToString());

  FT_Int char_width  = args[1]->NumberValue();
  FT_Int char_height = args[2]->NumberValue();
  FT_Int xdpi   = args[3]->NumberValue();
  FT_Int ydpi   = args[4]->NumberValue();

  error = FT_Set_Char_Size( ff->face, char_width, char_height, xdpi, ydpi );

  error = FT_Load_Char( ff->face, index, FT_LOAD_RENDER );
  slot = ff->face->glyph;

  width = slot->bitmap.width;
  height = slot->bitmap.rows;

  Local<Object> obj = NanNew<Object>();
  obj->Set(NanNew("id"), NanNew<Number>(charcode));
  obj->Set(NanNew("index"), NanNew<Number>(index));
  obj->Set(NanNew("width"), NanNew<Number>(width));
  obj->Set(NanNew("height"), NanNew<Number>(height));
  obj->Set(NanNew("x"), NanNew<Number>(slot->bitmap_left));
  obj->Set(NanNew("y"), NanNew<Number>(slot->bitmap_top));
  obj->Set(NanNew("offX"), NanNew<Number>(slot->advance.x));
  obj->Set(NanNew("offY"), NanNew<Number>(slot->advance.y));
  obj->Set(NanNew("image"), NanNewBufferHandle((char*)slot->bitmap.buffer, width*height));

  NanReturnValue(obj);

  //NanReturnValue(NanNewBufferHandle((char*)slot->bitmap.buffer, width*height));
}
