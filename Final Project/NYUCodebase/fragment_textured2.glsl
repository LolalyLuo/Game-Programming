uniform sampler2D diffuse;
varying vec4 vertexColor;
varying vec2 varTexCoord;

void main() {
    gl_FragColor = texture2D(diffuse, varTexCoord) * vertexColor;
}
