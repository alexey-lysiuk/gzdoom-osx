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
#include "m_png.h"
#include "version.h"
#include "w_wad.h"

#include "gl/renderer/gl_renderer.h"
#include "gl/utility/gl_clock.h"


namespace GLAuxilium
{

GLint GetInternalFormat( const TextureFormat format )
{
	switch ( format )
	{
		case TEXTURE_FORMAT_COLOR_RGBA:
			return GL_RGBA8;
		
		case TEXTURE_FORMAT_DEPTH_STENCIL:
			return GL_DEPTH24_STENCIL8;
		
		default:
			assert( !"Unknown texture format" );
			return 0;
	}
}

GLint GetFormat( const TextureFormat format )
{
	switch ( format )
	{
		case TEXTURE_FORMAT_COLOR_RGBA:
			return GL_RGBA;
			
		case TEXTURE_FORMAT_DEPTH_STENCIL:
			return GL_DEPTH_STENCIL;
			
		default:
			assert( !"Unknown texture format" );
			return 0;
	}
}

GLint GetDataType( const TextureFormat format )
{
	switch ( format )
	{
		case TEXTURE_FORMAT_COLOR_RGBA:
			return GL_UNSIGNED_BYTE;
			
		case TEXTURE_FORMAT_DEPTH_STENCIL:
			return GL_UNSIGNED_INT_24_8;
			
		default:
			assert( !"Unknown texture format" );
			return 0;
	}
}


GLint GetFilter( const TextureFilter filter )
{
	switch ( filter )
	{
		case TEXTURE_FILTER_NEAREST:
			return GL_NEAREST;
			
		case TEXTURE_FILTER_LINEAR:
			return GL_LINEAR;
			
		default:
			assert( !"Unknown texture filter" );
			return 0;
	}
}


void BoundTextureSetFilter( const GLenum target, const GLint filter )
{
	gl.TexParameteri( target, GL_TEXTURE_MIN_FILTER, filter );
	gl.TexParameteri( target, GL_TEXTURE_MAG_FILTER, filter );
	
	gl.TexParameteri( target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
	gl.TexParameteri( target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );		
}

void BoundTextureDraw2D( const GLsizei width, const GLsizei height )
{
	const bool flipX = width  < 0;
	const bool flipY = height < 0;
	
	const float u0 = flipX ? 1.0f : 0.0f;
	const float v0 = flipY ? 1.0f : 0.0f;
	const float u1 = flipX ? 0.0f : 1.0f;
	const float v1 = flipY ? 0.0f : 1.0f;
	
	const float x1 = 0.0f;
	const float y1 = 0.0f;
	const float x2 = abs( width  );
	const float y2 = abs( height );
	
	gl.Disable( GL_BLEND );
	gl.Disable( GL_ALPHA_TEST );
	
	gl.Begin( GL_QUADS );
	gl.Color4f( 1.0f, 1.0f, 1.0f, 1.0f );
	gl.TexCoord2f( u0, v1 );
	gl.Vertex2f( x1, y1 );
	gl.TexCoord2f( u1, v1 );
	gl.Vertex2f( x2, y1 );
	gl.TexCoord2f( u1, v0 );
	gl.Vertex2f( x2, y2 );
	gl.TexCoord2f( u0, v0 );
	gl.Vertex2f( x1, y2 );
	gl.End();
	
	gl.Enable( GL_ALPHA_TEST );
	gl.Enable( GL_BLEND );
}

bool BoundTextureSaveAsPNG( const GLenum target, const char* const path )
{
	if ( NULL == path )
	{
		return false;
	}
	
	GLint width  = 0;
	GLint height = 0;
	
	glGetTexLevelParameteriv( target, 0, GL_TEXTURE_WIDTH,  &width  );
	glGetTexLevelParameteriv( target, 0, GL_TEXTURE_HEIGHT, &height );
	
	if ( 0 == width || 0 == height )
	{
		Printf( "BoundTextureSaveAsPNG: invalid texture size %ix%i\n", width, height );
		
		return false;
	}

	static const int BYTES_PER_PIXEL = 4;
	
	const int imageSize = width * height * BYTES_PER_PIXEL;
	unsigned char* imageBuffer = static_cast< unsigned char* >( malloc( imageSize ) );
	
	if ( NULL == imageBuffer )
	{
		Printf( "BoundTextureSaveAsPNG: cannot allocate %i bytes\n", imageSize );
		
		return false;
	}
	
	glGetTexImage( target, 0, GL_BGRA, GL_UNSIGNED_BYTE, imageBuffer );

	const int lineSize = width * BYTES_PER_PIXEL;
	unsigned char lineBuffer[ lineSize ];
	
	for ( GLint line = 0; line < height / 2; ++line )
	{
		void* frontLinePtr = &imageBuffer[ line                  * lineSize ];
		void*  backLinePtr = &imageBuffer[ ( height - line - 1 ) * lineSize ];
		
		memcpy(   lineBuffer, frontLinePtr, lineSize );
		memcpy( frontLinePtr,  backLinePtr, lineSize );
		memcpy(  backLinePtr,   lineBuffer, lineSize );
	}
	
	FILE* file = fopen( path, "w" );
	
	if ( NULL == file )
	{
		Printf( "BoundTextureSaveAsPNG: cannot open file %s\n", path );
		
		free( imageBuffer );
		
		return false;
	}
	
	const bool result = 
		   M_CreatePNG( file, &imageBuffer[0], NULL, SS_BGRA, width, height, width * BYTES_PER_PIXEL )
		&& M_FinishPNG( file );
	
	fclose( file );
	
	free( imageBuffer );
	
	return result;
}


// ---------------------------------------------------------------------------


RenderTarget::RenderTarget( const GLsizei width, const GLsizei height, const RenderTarget* const sharedDepth )
{
	m_color.SetImageData( TEXTURE_FORMAT_COLOR_RGBA, width, height, NULL );
	m_color.SetFilter( TEXTURE_FILTER_NEAREST );
	
	if ( NULL == sharedDepth )
	{
		m_depthStencil.SetImageData( TEXTURE_FORMAT_DEPTH_STENCIL, width, height, NULL );
		m_depthStencil.SetFilter( TEXTURE_FILTER_NEAREST );
	}
	
	const GLuint depthStencilID = NULL == sharedDepth 
		? m_depthStencil.m_ID 
		: sharedDepth->m_depthStencil.m_ID;
	
	gl.GenFramebuffers( 1, &m_ID );
	gl.BindFramebuffer( GL_FRAMEBUFFER, m_ID );
	
	gl.FramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,        GL_TEXTURE_2D, m_color.m_ID,   0 );
	gl.FramebufferTexture2D( GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, depthStencilID, 0 );
	
	gl.BindFramebuffer( GL_FRAMEBUFFER, 0 );
}
	
RenderTarget::~RenderTarget()
{
	gl.DeleteFramebuffers( 1, &m_ID );
}


Texture2D& RenderTarget::GetColorTexture()
{
	return m_color;
}


void RenderTarget::DoBind( const GLuint resourceID )
{
	gl.BindFramebuffer( GL_FRAMEBUFFER, resourceID );
}

GLuint RenderTarget::GetBoundName()
{
	return GL_FRAMEBUFFER_BINDING;
}


// ---------------------------------------------------------------------------


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


ShaderProgram::ShaderProgram( const char* const vertexName, const char* const fragmentName )
: m_vertexShaderID(0)
, m_fragmentShaderID(0)
{
	const bool hasVertexShader = NULL != vertexName && strlen( vertexName ) > 0;
	if ( hasVertexShader )
	{
		m_vertexShaderID = CreateShader( GL_VERTEX_SHADER, vertexName );
	}
	
	const bool hasFragmentShader = NULL != fragmentName && strlen( fragmentName ) > 0;
	if ( hasFragmentShader )
	{
		m_fragmentShaderID = CreateShader( GL_FRAGMENT_SHADER, fragmentName );
	}
	
	if ( 0 == m_vertexShaderID && 0 == m_fragmentShaderID )
	{
		return;
	}
	
	m_ID = gl.CreateProgram();
	
	gl.AttachShader( m_ID, m_vertexShaderID );
	gl.AttachShader( m_ID, m_fragmentShaderID );
	gl.LinkProgram ( m_ID );
	
	static char errorBuffer[ 8 * 1024 ];
	memset( errorBuffer, 0, sizeof ( errorBuffer ) );
	
	gl.GetProgramInfoLog( m_ID, sizeof( errorBuffer ), NULL, errorBuffer );
	
	if ( '\0' != *errorBuffer )
	{
		Printf( "Program link failed:\n%s\n", errorBuffer );
		Printf( "Vertex shader: %s\n",   hasVertexShader   ? vertexName   : "<none>" );
		Printf( "Fragment shader: %s\n", hasFragmentShader ? fragmentName : "<none>" );
	}
}

ShaderProgram::~ShaderProgram()
{
	gl.DeleteShader( m_fragmentShaderID );
	gl.DeleteShader( m_vertexShaderID );
	
	gl.DeleteProgram( m_ID );
}


void ShaderProgram::DoBind( const GLuint resourceID )
{
	gl.UseProgram( resourceID );
}

GLenum ShaderProgram::GetBoundName()
{
	return GL_CURRENT_PROGRAM;
}


void ShaderProgram::SetUniform( const char* const name, const GLint value )
{
	Bind();
	
	const GLint location = gl.GetUniformLocation( m_ID, name );
	gl.Uniform1i( location, value );
	
	Unbind();
}

void ShaderProgram::SetUniform( const char* const name, const GLfloat value0, const GLfloat value1 )
{
	Bind();
	
	const GLint location = gl.GetUniformLocation( m_ID, name );
	gl.Uniform2f( location, value0, value1 );
	
	Unbind();
}


// ---------------------------------------------------------------------------


PostProcess::PostProcess( const RenderTarget* const sharedDepth )
: m_width       ( 0    )
, m_height      ( 0    )
, m_renderTarget( NULL )
, m_shader      ( NULL )
, m_sharedDepth ( sharedDepth )
{
	
}

PostProcess::~PostProcess()
{
	Release();
}


void PostProcess::Init( const char* const shaderName, const GLsizei width, const GLsizei height )
{
	assert( NULL != shaderName );
	assert( width  > 0 );
	assert( height > 0 );
	
	Release();
	
	m_width  = width;
	m_height = height;
	
	m_renderTarget = new RenderTarget( m_width, m_height, m_sharedDepth );
	
	m_shader = new ShaderProgram( NULL, shaderName );
	m_shader->SetUniform( "sampler0", 0 );
	m_shader->SetUniform( "resolution", 
		static_cast< GLfloat >( width ), 
		static_cast< GLfloat >( height ) );
}

void PostProcess::Release()
{
	if ( NULL != m_shader )
	{
		delete m_shader;
		m_shader = NULL;
	}
	
	if ( NULL != m_renderTarget )
	{
		delete m_renderTarget;
		m_renderTarget = NULL;
	}
	
	m_width  = 0;
	m_height = 0;
}

	
bool PostProcess::IsInitialized() const
{
	// TODO: check other members?
	return NULL != m_renderTarget;
}
	
	
void PostProcess::Start()
{
	assert( NULL != m_renderTarget );
	
	m_renderTarget->Bind();
}

void PostProcess::Finish()
{
	m_renderTarget->Unbind();
	
	Texture2D& colorTexture = m_renderTarget->GetColorTexture();
	
	gl.ActiveTexture( GL_TEXTURE0 );
	colorTexture.Bind();
	
	m_shader->Bind();
	colorTexture.Draw2D( m_width, m_height );
	m_shader->Unbind();
}


// ---------------------------------------------------------------------------


CapabilityChecker::CapabilityChecker()
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
}


// ---------------------------------------------------------------------------


BackBuffer* BackBuffer::s_instance;


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
, m_gammaProgram( NULL, "shaders/glsl/gamma_correction.fp" )
, m_postProcess( &m_renderTarget )
{
	s_instance = this;
		
	const bool isScaled = fabsf( s_parameters.pixelScale - 1.0f ) > 0.01f;
	
	m_renderTarget.GetColorTexture().SetFilter( isScaled 
		? TEXTURE_FILTER_LINEAR 
		: TEXTURE_FILTER_NEAREST );
	
	// Create gamma correction texture
	
	for ( size_t i = 0; i < GAMMA_TABLE_SIZE; ++i )
	{
		m_gammaTable[i] = GAMMA_TABLE_ALPHA + ( i << 16 ) + ( i << 8 ) + i;
	}
	
	m_gammaTexture.SetFilter( TEXTURE_FILTER_NEAREST );
	m_gammaTexture.SetImageData( TEXTURE_FORMAT_COLOR_RGBA, 256, 1, m_gammaTable );
	
	// Setup uniform samplers for gamma correction shader
	
	m_gammaProgram.SetUniform( "backbuffer", 0 );
	m_gammaProgram.SetUniform( "gammaTable", 1 );
}

BackBuffer::~BackBuffer()
{
	s_instance = NULL;
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


BackBuffer* BackBuffer::GetInstance()
{
	return s_instance;
}

PostProcess& BackBuffer::GetPostProcess()
{
	return m_postProcess;
}

	
void BackBuffer::DrawRenderTarget()
{
	m_renderTarget.Unbind();
	
	Texture2D& colorTexture = m_renderTarget.GetColorTexture();
	
	gl.ActiveTexture( GL_TEXTURE0 );
	colorTexture.Bind();
	gl.ActiveTexture( GL_TEXTURE1 );
	m_gammaTexture.Bind();
	gl.ActiveTexture( GL_TEXTURE0 );
	
	GLint viewport[4] = {0};
	glGetIntegerv( GL_VIEWPORT, viewport );
	
	gl.Viewport( s_parameters.shiftX, s_parameters.shiftY, s_parameters.width, s_parameters.height );
	
	m_gammaProgram.Bind();
	colorTexture.Draw2D( Width, Height );
	m_gammaProgram.Unbind();
	
	gl.Viewport( viewport[0], viewport[1], viewport[2], viewport[3] );
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
	
	m_gammaTexture.SetImageData( TEXTURE_FORMAT_COLOR_RGBA, 256, 1, m_gammaTable );
}

} // namespace GLAuxilium
