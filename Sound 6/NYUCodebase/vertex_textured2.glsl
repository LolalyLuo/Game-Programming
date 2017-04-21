attribute vec4 position;
attribute vec4 color;
attribute vec2 texCoord;
uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;
varying vec4 vertexColor;
varying vec2 varTexCoord;
void main() {
    vec4 p = viewMatrix * modelMatrix  * position;
    vertexColor = color;
    varTexCoord = texCoord;
    gl_Position = projectionMatrix * p;
}
