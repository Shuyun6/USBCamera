precision mediump float;
uniform sampler2D u_Texture;
varying vec2 vTextureCoord;

void main() {
    gl_FragColor = texture2D(u_Texture, vTextureCoord);

}