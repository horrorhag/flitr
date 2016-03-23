/* -*- Mode: C -*- */
uniform sampler2DRect textureID0;
uniform sampler2DRect textureID1;
uniform sampler2DRect textureID2;

uniform vec4 colourMask0;
uniform vec4 colourMask1;
uniform vec4 colourMask2;

void main(void)
{
    vec2 texCoord0 = gl_TexCoord[0].xy;
    vec2 texCoord1 = gl_TexCoord[1].xy;
    vec2 texCoord2 = gl_TexCoord[2].xy;
    
    vec4 c0  = texture2DRect(textureID0, texCoord0);
    vec4 c1  = texture2DRect(textureID1, texCoord1);
    vec4 c2  = texture2DRect(textureID2, texCoord2);
    

    gl_FragColor = c0*colourMask0 + c1*colourMask1 + c2*colourMask2;
}
