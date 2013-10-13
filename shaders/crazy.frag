#version 330
in vec3 Position;
in vec3 Normal;
in vec2 TexCoord;
uniform sampler2D Tex0;
uniform vec2 screenres;
out vec4 FragColor;
void main() {
    vec4 texColor = texture( Tex0, TexCoord );
    float dx = gl_FragCoord.x-screenres.x/2.f;
    float dy = gl_FragCoord.y-screenres.y/2.f;
    float tint = 1.f-sqrt(dx*dx+dy*dy)/(screenres.x);
    texColor = vec4(texColor.xyz, texColor.w*tint);
    FragColor = texColor;
}
