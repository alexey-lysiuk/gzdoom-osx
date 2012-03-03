/*
 ** gl_backbuffer_fbo.h
 **
 **---------------------------------------------------------------------------
 ** Copyright 2012 Alexey Lysiuk
 ** All rights reserved.
 **
 ** Redistribution and use in source and binary forms, with or without
 ** modification, are permitted provided that the following conditions
 ** are met:
 **
 ** 1. Redistributions of source code must retain the above copyright
 **    notice, this list of conditions and the following disclaimer.
 ** 2. Redistributions in binary form must reproduce the above copyright
 **    notice, this list of conditions and the following disclaimer in the
 **    documentation and/or other materials provided with the distribution.
 ** 3. The name of the author may not be used to endorse or promote products
 **    derived from this software without specific prior written permission.
 **
 ** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 ** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 ** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 ** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 ** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 ** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 ** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 ** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 ** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 ** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 **---------------------------------------------------------------------------
 **
 */

#include "gl/system/gl_framebuffer.h"


class OpenGLBackbufferFBO : public OpenGLFrameBuffer
{
	DECLARE_CLASS( OpenGLBackbufferFBO, OpenGLFrameBuffer )
	
public:
	OpenGLBackbufferFBO();
	OpenGLBackbufferFBO( int width, int height, bool fullscreen );
	virtual ~OpenGLBackbufferFBO();
	
	virtual bool Lock( bool buffered );
	virtual void Update();

	virtual void GetScreenshotBuffer( const BYTE*& buffer, int& pitch, ESSType& color_type );
	
	
	struct Parameters
	{
		float pixelScale;
		
		float shiftX;
		float shiftY;
		
		float width;
		float height;
	};
	
	static Parameters& GetParameters();
	
	void GetGammaTable(       uint16_t* red,       uint16_t* green,       uint16_t* blue );
	void SetGammaTable( const uint16_t* red, const uint16_t* green, const uint16_t* blue );
	
	static void SetTextureParameters( const GLenum target, const GLint filter );
	
private:
	GLuint m_fboID;
	GLuint m_colorID;
	GLuint m_depthStencilID;
	
	GLuint m_gammaProgramID;
	GLuint m_gammaShaderID;
	
	GLuint m_gammaTableID;
	
	static const size_t GAMMA_TABLE_SIZE = 256;
	uint32_t m_gammaTable[ GAMMA_TABLE_SIZE ];
	

	static Parameters s_parameters;
	
	
	void InitFBO();
	void InitGammaCorrection();
	
	void DrawFBO();
	
};
