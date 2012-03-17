/*
 ** gl_auxilium.cpp
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

#include "gl/system/gl_auxilium.h"

#include "i_system.h"
#include "version.h"
#include "w_wad.h"

#include "gl/renderer/gl_renderer.h"
#include "gl/utility/gl_clock.h"


namespace GLAuxilium
{

RenderTarget::RenderTarget()
{
	InitDefaults( 0, 0 );
}

RenderTarget::RenderTarget( const GLsizei width, const GLsizei height )
{
	InitDefaults( width, height );
}

RenderTarget::~RenderTarget()
{
	Release();
}


GLsizei RenderTarget::GetWidth() const
{
	return m_width;
}

void RenderTarget::SetWidth( const GLsizei width )
{
	m_width = width;
}

GLsizei RenderTarget::GetHeight() const
{
	return m_height;
}

void RenderTarget::SetHeight( const GLsizei height )
{
	m_height = height;
}

GLsizei RenderTarget::GetTextureFilter() const
{
	return m_textureFilter;
}

void RenderTarget::SetTextureFilter( const GLint filter )
{
	m_textureFilter = filter;
}


GLuint RenderTarget::GetColorTexture() const
{
	return m_colorID;
}


void RenderTarget::Init()
{
	// TODO: check hardware support
	
	gl.GenTextures( 1, &m_colorID );
	gl.BindTexture( GL_TEXTURE_2D, m_colorID );
	gl.TexImage2D ( GL_TEXTURE_2D, 0, GL_RGBA8, m_width, m_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL );
	SetTextureParameters( GL_TEXTURE_2D, m_textureFilter );
	
	gl.GenTextures( 1, &m_depthStencilID );
	gl.BindTexture( GL_TEXTURE_2D, m_depthStencilID );
	gl.TexImage2D ( GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, m_width, m_height, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL );
	SetTextureParameters( GL_TEXTURE_2D, GL_NEAREST );
	
	// TODO: check errors
	
	gl.GenFramebuffers( 1, &m_fboID );
	gl.BindFramebuffer( GL_FRAMEBUFFER, m_fboID );
	
	gl.FramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,        GL_TEXTURE_2D, m_colorID,        0 );
	gl.FramebufferTexture2D( GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, m_depthStencilID, 0 );
	
	// TODO: check FBO completeness
}

void RenderTarget::Release()
{
	gl.DeleteTextures( 1, &m_depthStencilID );
	m_depthStencilID = 0;
	
	gl.DeleteTextures( 1, &m_colorID );
	m_colorID = 0;
	
	gl.DeleteFramebuffers( 1, &m_fboID );
	m_fboID = 0;
}


void RenderTarget::Bind()
{
	GLint oldFBO = 0;
	glGetIntegerv( GL_FRAMEBUFFER_BINDING, &oldFBO );
	
	if ( GLuint( oldFBO ) != m_fboID )
	{
		gl.BindFramebuffer( GL_FRAMEBUFFER, m_fboID );
		
		m_oldFBOID = oldFBO;
	}
}

void RenderTarget::Unbind()
{
	gl.BindFramebuffer( GL_FRAMEBUFFER, m_oldFBOID );
}


void RenderTarget::InitDefaults( const GLsizei width, const GLsizei height )
{
	m_width          = width;
	m_height         = height;
	
	m_textureFilter  = GL_NEAREST;
	
	m_fboID          = 0;
	m_colorID        = 0;
	m_depthStencilID = 0;
	
	m_oldFBOID       = 0;
}


// ---------------------------------------------------------------------------


ShaderProgram::ShaderProgram()
{
	InitDefaults( NULL, NULL );
}

ShaderProgram::ShaderProgram( const char* vertexName, const char* fragmentName )
{
	InitDefaults( vertexName, fragmentName );
}

ShaderProgram::~ShaderProgram()
{
	Release();
}


const char* ShaderProgram::GetVertexName() const
{
	return m_vertexName.GetChars();
}

void ShaderProgram::SetVertexName( const char* name )
{
	m_vertexName = name;
}

const char* ShaderProgram::GetFragmentName() const
{
	return m_fragmentName.GetChars();
}

void ShaderProgram::SetFragmentName( const char* name )
{
	m_fragmentName = name;
}


GLuint ShaderProgram::GetProgram() const
{
	return m_programID;
}

GLuint ShaderProgram::GetVertexShader() const
{
	return m_vertexShaderID;
}


GLuint ShaderProgram::GetFragmentShader() const
{
	return m_fragmentShaderID;
}


static GLuint CreateShader( const GLenum type, const char* name )
{
	const int shaderLumpID = Wads.CheckNumForFullName( name );
	
	if ( -1 == shaderLumpID )
	{
		Printf( "Unable to load shader \"%s\"\n", name );
		
		return 0;
	}

	FMemLump shaderLump = Wads.ReadLump( shaderLumpID );
	
	const char* shaderString = shaderLump.GetString().GetChars();
	GLint shaderSize = strlen( shaderString );
	
	static char errorBuffer[ 8 * 1024 ];
	memset( errorBuffer, 0, sizeof ( errorBuffer ) );
	
	const GLuint result = gl.CreateShader( type );
	
	gl.ShaderSource( result, 1, &shaderString, &shaderSize );
	gl.CompileShader( result );
	gl.GetShaderInfoLog( result, sizeof( errorBuffer ), NULL, errorBuffer );
	
	if ( '\0' != *errorBuffer )
	{
		Printf( "Shader \"%s\" compilation failed:\n%s\n", name, errorBuffer );
	}
	
	return result;
}


void ShaderProgram::Init()
{
	const bool hasVertexShader = m_vertexName.Len() > 0;
	if ( hasVertexShader )
	{
		m_vertexShaderID = CreateShader( GL_VERTEX_SHADER, m_vertexName );
	}
	
	const bool hasFragmentShader = m_fragmentName.Len() > 0;
	if ( hasFragmentShader )
	{
		m_fragmentShaderID = CreateShader( GL_FRAGMENT_SHADER, m_fragmentName );
	}
	
	if ( 0 == m_vertexShaderID && 0 == m_fragmentShaderID )
	{
		return;
	}
	
	m_programID = gl.CreateProgram();
	
	gl.AttachShader( m_programID, m_vertexShaderID );
	gl.AttachShader( m_programID, m_fragmentShaderID );
	gl.LinkProgram ( m_programID );
	
	static char errorBuffer[ 8 * 1024 ];
	memset( errorBuffer, 0, sizeof ( errorBuffer ) );
	
	gl.GetProgramInfoLog( m_programID, sizeof( errorBuffer ), NULL, errorBuffer );
	
	if ( '\0' != *errorBuffer )
	{
		Printf( "Program link failed:\n%s\n", errorBuffer );
		Printf( "Vertex shader: %s\n",   hasVertexShader   ? m_vertexName.GetChars()   : "<none>" );
		Printf( "Fragment shader: %s\n", hasFragmentShader ? m_fragmentName.GetChars() : "<none>" );
	}
}

void ShaderProgram::Release()
{
	gl.DeleteProgram( m_programID );
	m_programID = 0;
	
	gl.DeleteShader( m_fragmentShaderID );
	m_fragmentShaderID = 0;
	
	gl.DeleteShader( m_vertexShaderID );
	m_vertexShaderID = 0;
}


void ShaderProgram::Bind()
{
	GLint oldProgram = 0;
	glGetIntegerv( GL_CURRENT_PROGRAM, &oldProgram );
	
	if ( GLuint( oldProgram ) != m_programID )
	{
		gl.UseProgram( m_programID );
		
		m_oldProgramID = oldProgram;
	}
}

void ShaderProgram::Unbind()
{
	gl.UseProgram( m_oldProgramID );
}


void ShaderProgram::InitDefaults( const char* vertexName, const char* fragmentName )
{
	if ( NULL != vertexName )
	{
		m_vertexName = vertexName;
	}
	
	if ( NULL != fragmentName )
	{
		m_fragmentName = fragmentName;
	}
	
	m_programID        = 0;
	m_vertexShaderID   = 0;
	m_fragmentShaderID = 0;
	
	m_oldProgramID     = 0;
}


// ---------------------------------------------------------------------------


BackBuffer::Parameters BackBuffer::s_parameters = 
{
	1.0f, // pixelScale
	
	0.0f, // shiftX
	0.0f, // shiftY
	
	1.0f, // width
	1.0f  // height
};


static const uint32_t GAMMA_TABLE_ALPHA = 0xFF000000;


BackBuffer::BackBuffer( int width, int height, bool fullscreen )
: OpenGLFrameBuffer( 0, width, height, 32, 60, fullscreen )
, m_renderTarget( width, height )
, m_gammaTableID(0)
{
	static const char ERROR_MESSAGE[] = 
		"The graphics hardware in your system does not support %s.\n"
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
	
	InitRenderTarget();
	InitGammaCorrection();
}

BackBuffer::~BackBuffer()
{
	gl.DeleteTextures( 1, &m_gammaTableID );
}


bool BackBuffer::Lock( bool buffered )
{
	if ( 0 == m_Lock )
	{
		m_renderTarget.Bind();
	}
	
	return Super::Lock( buffered );
}

void BackBuffer::Update()
{
	if ( !CanUpdate() )
	{
		GLRenderer->Flush();
		return;
	}
	
	Begin2D( false );
	
	DrawRateStuff();
	GLRenderer->Flush();
	
	DrawRenderTarget();
	
	Swap();
	Unlock();
	
	CheckBench();
}


void BackBuffer::GetScreenshotBuffer( const BYTE*& buffer, int& pitch, ESSType& color_type )
{
	m_renderTarget.Bind();
	
	Super::GetScreenshotBuffer( buffer, pitch, color_type );
	
	m_renderTarget.Unbind();
}


void BackBuffer::InitRenderTarget()
{
	const bool isScaled = fabsf( s_parameters.pixelScale - 1.0f ) > 0.01f;
	
	m_renderTarget.SetTextureFilter( isScaled ? GL_LINEAR : GL_NEAREST );
	m_renderTarget.Init();
}

void BackBuffer::InitGammaCorrection()
{
	// Create gamma correction texture
	
	for ( size_t i = 0; i < GAMMA_TABLE_SIZE; ++i )
	{
		m_gammaTable[i] = GAMMA_TABLE_ALPHA + ( i << 16 ) + ( i << 8 ) + i;
	}
	
	gl.GenTextures( 1, &m_gammaTableID );
	gl.BindTexture( GL_TEXTURE_1D, m_gammaTableID );
	SetTextureParameters( GL_TEXTURE_1D, GL_NEAREST );
	gl.TexImage1D ( GL_TEXTURE_1D, 0, GL_RGBA8, 256, 0, GL_RGBA, GL_UNSIGNED_BYTE, m_gammaTable );
	gl.BindTexture( GL_TEXTURE_1D, 0 );
	
	// Create gamma correction shader
	
	m_gammaProgram.SetFragmentName( "shaders/glsl/gamma_correction.fp" );
	m_gammaProgram.Init();
	m_gammaProgram.Bind();
	
	// Setup uniforms for gamma correction shader
	
	const GLuint gammaProgramID     = m_gammaProgram.GetProgram();
	const GLint  backbufferLocation = gl.GetUniformLocation( gammaProgramID, "backbuffer" );
	const GLint  gammaTableLocation = gl.GetUniformLocation( gammaProgramID, "gammaTable" );
	
	gl.Uniform1i( backbufferLocation, 0 );
	gl.Uniform1i( gammaTableLocation, 1 );
	
	m_gammaProgram.Unbind();
}


void BackBuffer::DrawRenderTarget()
{
	m_renderTarget.Unbind();
	
	gl.Disable( GL_BLEND );
	gl.Disable( GL_ALPHA_TEST );
	
	gl.ActiveTexture( GL_TEXTURE0 );
	gl.BindTexture( GL_TEXTURE_2D, m_renderTarget.GetColorTexture() );
	gl.ActiveTexture( GL_TEXTURE1 );
	gl.BindTexture( GL_TEXTURE_1D, m_gammaTableID );
	gl.ActiveTexture( GL_TEXTURE0 );
	
	m_gammaProgram.Bind();
	
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
	
	m_gammaProgram.Unbind();
	
	gl.Viewport( viewport[0], viewport[1], viewport[2], viewport[3] );
	
	gl.BindTexture( GL_TEXTURE_2D, 0 );
	
	gl.Enable( GL_ALPHA_TEST );
	gl.Enable( GL_BLEND );
}


BackBuffer::Parameters& BackBuffer::GetParameters()
{
	return s_parameters;
}


void BackBuffer::GetGammaTable( uint16_t* red, uint16_t* green, uint16_t* blue )
{
	for ( size_t i = 0; i < GAMMA_TABLE_SIZE; ++i )
	{
		const uint32_t r = ( m_gammaTable[i] & 0x000000FF );
		const uint32_t g = ( m_gammaTable[i] & 0x0000FF00 ) >> 8;
		const uint32_t b = ( m_gammaTable[i] & 0x00FF0000 ) >> 16;

		// Convert 8 bits colors to 16 bits by multiplying on 256
		
		red  [i] = Uint16( r << 8 );
		green[i] = Uint16( g << 8 );
		blue [i] = Uint16( b << 8 );
	}	
}

void BackBuffer::SetGammaTable( const uint16_t* red, const uint16_t* green, const uint16_t* blue )
{
	for ( size_t i = 0; i < GAMMA_TABLE_SIZE; ++i )
	{
		// Convert 16 bits colors to 8 bits by dividing on 256
		
		const uint32_t r =   red[i] >> 8;
		const uint32_t g = green[i] >> 8;
		const uint32_t b =  blue[i] >> 8;
		
		m_gammaTable[i] = GAMMA_TABLE_ALPHA + ( b << 16 ) + ( g << 8 ) + r;
	}
	
	gl.BindTexture( GL_TEXTURE_1D, m_gammaTableID );
	gl.TexImage1D ( GL_TEXTURE_1D, 0, GL_RGBA8, 256, 0, GL_RGBA, GL_UNSIGNED_BYTE, m_gammaTable );
	gl.BindTexture( GL_TEXTURE_1D, 0 );
}


// ---------------------------------------------------------------------------


void SetTextureParameters( const GLenum target, const GLint filter )
{
	gl.TexParameteri( target, GL_TEXTURE_MIN_FILTER, filter );
	gl.TexParameteri( target, GL_TEXTURE_MAG_FILTER, filter );
	
	gl.TexParameteri( target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
	gl.TexParameteri( target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
}

} // namespace GLAuxilium
