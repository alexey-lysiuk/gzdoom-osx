/*
 ** gl_backbuffer_fbo.cpp
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

#include "gl/system/gl_backbuffer_fbo.h"

#include "i_system.h"
#include "version.h"

#include "gl/renderer/gl_renderer.h"
#include "gl/utility/gl_clock.h"


IMPLEMENT_CLASS( OpenGLBackbufferFBO )


OpenGLBackbufferFBO::Parameters OpenGLBackbufferFBO::s_parameters = 
{
	1.0f, // pixelScale
	
	0.0f, // shiftX
	0.0f, // shiftY
	
	1.0f, // width
	1.0f  // height
};


OpenGLBackbufferFBO::OpenGLBackbufferFBO()
{
	
}

OpenGLBackbufferFBO::OpenGLBackbufferFBO( int width, int height, bool fullscreen )
: OpenGLFrameBuffer( 0, width, height, 32, 60, fullscreen )
{
	static const char ERROR_MESSAGE[] = 
		"The graphics cards in your system does not support %s.\n"
		"It is required to run this version of " GAMENAME ".\n"
		"You can try to use SDL-based version where this feature is not mandatory.";
	
	if ( !( gl.flags & RFL_GL_21 ) )
	{
		I_FatalError( ERROR_MESSAGE, "OpenGL 2.1" );
	}
	
	if ( !( gl.flags & RFL_FRAMEBUFFER ) )
	{
		I_FatalError( ERROR_MESSAGE, "Frame Buffer Object (FBO)" );
	}
	
	InitFBO();
}

OpenGLBackbufferFBO::~OpenGLBackbufferFBO()
{
	gl.DeleteTextures( 1, &m_depthStencilID );
	gl.DeleteTextures( 1, &m_colorID );
	gl.DeleteFramebuffers( 1, &m_fboID );
}


bool OpenGLBackbufferFBO::Lock( bool buffered )
{
	if ( 0 == m_Lock )
	{
		gl.BindFramebuffer( GL_FRAMEBUFFER, m_fboID );
	}
	
	return Super::Lock( buffered );
}

void OpenGLBackbufferFBO::Update()
{
	if ( !CanUpdate() )
	{
		GLRenderer->Flush();
		return;
	}
	
	Begin2D( false );
	
	DrawRateStuff();
	GLRenderer->Flush();
	
	DrawFBO();
	
	Swap();
	Unlock();
	
	CheckBench();
}


void OpenGLBackbufferFBO::InitFBO()
{
	// TODO: check and setup texture parameters if needed
	
	gl.GenTextures( 1, &m_colorID );
    gl.BindTexture( GL_TEXTURE_2D, m_colorID );
    gl.TexImage2D ( GL_TEXTURE_2D, 0, GL_RGBA8, Width, Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL );
    gl.BindTexture( GL_TEXTURE_2D, 0 );
	
	gl.GenTextures( 1, &m_depthStencilID );
	gl.BindTexture( GL_TEXTURE_2D, m_depthStencilID );
    gl.TexImage2D ( GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, Width, Height, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL );
    gl.BindTexture( GL_TEXTURE_2D, 0 );
	
	gl.GenFramebuffers( 1, &m_fboID );
	gl.BindFramebuffer( GL_FRAMEBUFFER, m_fboID );
	
	gl.FramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_colorID, 0 );
	gl.FramebufferTexture2D( GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, m_depthStencilID, 0 );
	
	gl.BindFramebuffer( GL_FRAMEBUFFER_EXT, 0 );
}


void OpenGLBackbufferFBO::DrawFBO()
{
	gl.BindFramebuffer( GL_FRAMEBUFFER, 0 );
	
	gl.Disable( GL_BLEND );
	gl.Disable( GL_ALPHA_TEST );
	
	gl.ActiveTexture( GL_TEXTURE0 );
	gl.BindTexture( GL_TEXTURE_2D, m_colorID );
	
	gl.TexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	gl.TexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	
//	gl.TexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
//	gl.TexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );

//	glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL );

	GLint viewport[4] = {0};
	glGetIntegerv( GL_VIEWPORT, viewport );
	
	gl.Viewport( s_parameters.shiftX, s_parameters.shiftY, s_parameters.width, s_parameters.height );
	
	static const float U0 = 0.0f, U1 = 1.0f;
	static const float V0 = 0.0f, V1 = 1.0f;
	
	const float x1 = 0.0f,  y1 = 0.0f;
	const float x2 = Width, y2 = Height;
	
	gl.Begin( GL_QUADS );
	gl.Color4f( 1.0f, 1.0f, 1.0f, 1.0f );
	gl.TexCoord2f( U0, V1 );
	gl.Vertex2f( x1, y1 );
	gl.TexCoord2f( U1, V1 );
	gl.Vertex2f( x2, y1 );
	gl.TexCoord2f( U1, V0 );
	gl.Vertex2f( x2, y2 );
	gl.TexCoord2f( U0, V0 );
	gl.Vertex2f( x1, y2 );
	gl.End();
	
	gl.UseProgram(0);
	
	gl.Viewport( viewport[0], viewport[1], viewport[2], viewport[3] );
	
	gl.BindTexture( GL_TEXTURE_2D, 0 );
	
	gl.Enable( GL_ALPHA_TEST );
	gl.Enable( GL_BLEND );
}


OpenGLBackbufferFBO::Parameters& OpenGLBackbufferFBO::GetParameters()
{
	return s_parameters;
}
