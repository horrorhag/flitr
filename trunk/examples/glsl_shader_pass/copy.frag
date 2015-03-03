/* -*- Mode: C -*- */
uniform sampler2DRect textureID0;

void main(void)
{
    vec2 texCoord = gl_TexCoord[0].xy;
    vec4 c  = texture2DRect(textureID0, texCoord);
    gl_FragColor = c;
}
