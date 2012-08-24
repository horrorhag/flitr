/* -*- Mode: C -*- */
uniform sampler2DRect textureID0;
uniform float variable1;

void main(void)
{
    float offset = variable1;

    vec2 texCoord = gl_TexCoord[0].xy;

    vec4 c  = texture2DRect(textureID0, texCoord);
    vec4 bl = texture2DRect(textureID0, texCoord + vec2(-offset, -offset));
    vec4 l  = texture2DRect(textureID0, texCoord + vec2(-offset,     0.0));
    vec4 tl = texture2DRect(textureID0, texCoord + vec2(-offset,  offset));
    vec4 t  = texture2DRect(textureID0, texCoord + vec2(    0.0,  offset));
    vec4 tr = texture2DRect(textureID0, texCoord + vec2( offset,  offset));
    vec4 r  = texture2DRect(textureID0, texCoord + vec2( offset,     0.0));
    vec4 br = texture2DRect(textureID0, texCoord + vec2( offset, -offset));
    vec4 b  = texture2DRect(textureID0, texCoord + vec2(    0.0, -offset));
    
    if (texCoord.x > 100) {
        gl_FragColor = 12*c + -2*(t+b+l+r) + -1*(tl+tr+bl+br);
    } else {
        gl_FragColor = 0.8*c;
    }
}
