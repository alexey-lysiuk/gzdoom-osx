
uniform sampler2D backbuffer;
uniform sampler1D gammaTable;

void main()
{
	vec3 color = texture2D(backbuffer, gl_TexCoord[0].xy).rgb;

//	/* DEBUG */ vec2 uv = gl_TexCoord[0].xy; if (uv.x<0.50) {

	gl_FragColor.r = texture1D(gammaTable, color.r).r;
	gl_FragColor.g = texture1D(gammaTable, color.g).g;
	gl_FragColor.b = texture1D(gammaTable, color.b).b;

//	/* DEBUG */ } else gl_FragColor.rgb = color;

	gl_FragColor.a = 1.0;
}
