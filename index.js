var freetype = require('./build/Release/freetype2');
var FontFace = freetype.FontFace;

module.exports.parse = function(font) {
  return new FontFace(font);
}
