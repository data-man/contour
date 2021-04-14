varying vec4 fs_textColor;

#if __VERSION__ >= 130
out vec4 outColor;
#endif

void main()
{
#if __VERSION__ >= 130
    outColor = fs_textColor;
#else
    gl_FragColor = fs_textColor;
#endif
}
