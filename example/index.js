var fs = require('fs'),
    freetype = require('../index.js');

debugger

fs.readFile(process.argv[2], function(err, buffer) {
  if (!!err) throw err;
  var fontface = freetype.parse(buffer);
  //console.log(fontface);
  debugger
  //console.log('aaa');
  //function range(n) { var i = 0; var res = []; while(n--) res.push(i++); return res; };
  //range(255).forEach( function(i) { range(255).forEach( function(j) { k = fontface.kerning(i,j); if (k.x || k.y) console.log(i, j, k); } ); } );

  //console.log(fontface.hasKerning());
  //console.log(fontface.kerning('W'.charCodeAt(0), 'A'.charCodeAt(0)));
  var renderedFont = fontface.available_characters.map(function(index) {
    var glyph = fontface.render(index);
    return {
      id: index,
      image: glyph.data.toString('base64'),
      width: glyph.width,
      height: glyph.height,
      offX: glyph.advanceX / 64,
      offY: glyph.advanceY / 64,
      x: glyph.x,
      y: glyph.y
    }
  });
  console.log(JSON.stringify(renderedFont));
});
