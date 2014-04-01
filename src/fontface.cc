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
  if (FT_Init_FreeType(&library)) exit(EXIT_FAILURE);

  Local<FunctionTemplate> ctor = FunctionTemplate::New(New);
  NanAssignPersistent(FunctionTemplate, constructor, ctor);
  ctor->InstanceTemplate()->SetInternalFieldCount(1);
  ctor->SetClassName(NanSymbol("FontFace"));

  NODE_SET_PROTOTYPE_METHOD(ctor, "render", Render);
  NODE_SET_PROTOTYPE_METHOD(ctor, "kerning", Kerning);
  NODE_SET_PROTOTYPE_METHOD(ctor, "hasKerning", hasKerning);

  // target->Set(NanSymbol("Canvas"), ctor->GetFunction());
  // proto->SetAccessor(NanSymbol("xxxx"), GetXxxx);

  target->Set(NanSymbol("FontFace"), ctor->GetFunction());
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
  obj->Set(NanSymbol("num_faces"), Integer::New(this->face->num_faces));
  obj->Set(NanSymbol("face_index"), Integer::New(this->face->face_index));

  obj->Set(NanSymbol("face_flags"), Integer::New(this->face->face_flags));
  obj->Set(NanSymbol("style_flags"), Integer::New(this->face->style_flags));

  obj->Set(NanSymbol("num_glyphs"), Integer::New(this->face->num_glyphs));

  obj->Set(NanSymbol("family_name"), String::New(this->face->family_name));
  obj->Set(NanSymbol("style_name"), String::New(this->face->style_name));

  obj->Set(NanSymbol("num_fixed_sizes"), Integer::New(this->face->num_fixed_sizes));
  // obj->Set(NanSymbol("available_sizes"), Integer::New(this->face->available_sizes));

  obj->Set(NanSymbol("num_charmaps"), Integer::New(this->face->num_charmaps));
  // obj->Set(NanSymbol("charmaps"), Integer::New(this->face->charmaps));

  // obj->Set(NanSymbol("generic"), Integer::New(this->face->generic));


  obj->Set(NanSymbol("units_per_EM"), Integer::New(this->face->units_per_EM));
  // obj->Set(NanSymbol("bbox"), Integer::New(this->face->bbox));

  obj->Set(NanSymbol("ascender"), Integer::New(this->face->ascender));
  obj->Set(NanSymbol("descender"), Integer::New(this->face->descender));
  obj->Set(NanSymbol("height"), Integer::New(this->face->height));

  obj->Set(NanSymbol("max_advance_width"), Integer::New(this->face->max_advance_width));
  obj->Set(NanSymbol("max_advance_height"), Integer::New(this->face->max_advance_height));

  obj->Set(NanSymbol("underline_position"), Integer::New(this->face->underline_position));
  obj->Set(NanSymbol("underline_thickness"), Integer::New(this->face->underline_thickness));

  std::vector<FT_UInt> acv = this->AvailableCharacters();
  Local<Array> aca = Array::New(acv.size());
  for (size_t i = 0; i < acv.size(); i++) {
    aca->Set(i, Integer::New(acv.at(i)));
  }
  obj->Set(NanSymbol("available_characters"), aca);
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
  NanReturnValue(Boolean::New(FT_HAS_KERNING(ff->face)));
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

  Local<Object> res = Object::New();
  res->Set(NanSymbol("x"), Integer::New(delta.x));
  res->Set(NanSymbol("y"), Integer::New(delta.y));
  NanReturnValue(res);
}

NAN_METHOD(FontFace::Render) {
  FT_Error error;
  //FT_Face  face;
  FT_GlyphSlot slot;
  FT_Int width, height;

  NanScope();

  FontFace *ff = ObjectWrap::Unwrap<FontFace>(args.This());

  FT_UInt index;
  FT_UInt charcode = (FT_UInt) args[0]->NumberValue();
  if (args[0]->IsNumber())
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

  Local<Object> obj = Object::New();
  obj->Set(NanSymbol("id"), Integer::New(charcode));
  obj->Set(NanSymbol("index"), Integer::New(index));
  obj->Set(NanSymbol("width"), Integer::New(width));
  obj->Set(NanSymbol("height"), Integer::New(height));
  obj->Set(NanSymbol("x"), Integer::New(slot->bitmap_left));
  obj->Set(NanSymbol("y"), Integer::New(slot->bitmap_top));
  obj->Set(NanSymbol("offX"), Integer::New(slot->advance.x));
  obj->Set(NanSymbol("offY"), Integer::New(slot->advance.y));
  obj->Set(NanSymbol("image"), NanNewBufferHandle((char*)slot->bitmap.buffer, width*height));

  NanReturnValue(obj);

  //NanReturnValue(NanNewBufferHandle((char*)slot->bitmap.buffer, width*height));
}
