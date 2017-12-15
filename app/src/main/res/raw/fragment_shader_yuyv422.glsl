precision mediump float;
uniform sampler2D u_TextureYUYV422;
varying vec2 vTextureCoord;

void main() {

    vec3 yuv = vec3(
        texture2D(u_TextureY, vTextureCoord).r - 0.0625,
        texture2D(u_TextureUV, vTextureCoord).a - 0.5,
        texture2D(u_TextureUV, vTextureCoord).r - 0.5
    );

    vec3 yuv = nec3(
        texture2D(u_TextureYUYV422, vTextureCoord).rb,
        texture2D(u_TextureYUYV422, vTextureCoord).g - 0.5,
        texture2D(u_TextureYUYV422, vTextureCoord).a - 0.5
    );

    vec3 rgb = mat3(
                      1,           1,         1,
                      0,          -0.39465,   2.03211,
                      1.13983,    -0.58060,   0
                ) * yuv;
    gl_FragColor = vec4(rgb, 1);
}