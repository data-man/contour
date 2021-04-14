uniform mat4 vs_projection;                 // projection matrix (flips around the coordinate system)

#if __VERSION__ >= 130
layout (location = 0) in vec3 vs_vertex;    // target vertex coordinates
layout (location = 1) in vec4 vs_texCoords; // 3D-atlas texture coordinates
layout (location = 2) in vec4 vs_colors;    // custom foreground colors
out vec4 fs_TexCoord;
out vec4 fs_textColor;
#else
attribute vec3 vs_vertex;
attribute vec4 vs_texCoords;
attribute vec4 vs_colors;
varying vec4 fs_TexCoord;
varying vec4 fs_textColor;
#endif

void main()
{
    gl_Position = vs_projection * vec4(vs_vertex.xyz, 1.0);

    fs_TexCoord = vs_texCoords;
    fs_textColor = vs_colors;
}
