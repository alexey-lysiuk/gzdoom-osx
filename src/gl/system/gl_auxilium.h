/*
 ** gl_auxilium.h
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

#ifndef GL_SYSTEM_AUXILIUM_H_INCLUDED
#define GL_SYSTEM_AUXILIUM_H_INCLUDED


#include "gl/system/gl_framebuffer.h"


namespace GLAuxilium
{

class NonCopyable
{
protected:
	NonCopyable()
	{
	}
	
	~NonCopyable()
	{
	}
	
private:
	NonCopyable( const NonCopyable& );
	const NonCopyable& operator=( const NonCopyable& );
	
};


// ---------------------------------------------------------------------------


template < typename T >
class NoUnbind : private NonCopyable
{
public:
	void BindImpl( const GLuint resourceID )
	{
		T::DoBind( resourceID );
	}
	
	void UnbindImpl()
	{
		
	}
	
};


template < typename T >
class UnbindToDefault : private NonCopyable
{
public:
	void BindImpl( const GLuint resourceID )
	{
		T::DoBind( resourceID );
	}
	
	void UnbindImpl()
	{
		T::DoBind(0);
	}
	
};


template < typename T >
class UnbindToPrevious : private NonCopyable
{
public:
	UnbindToPrevious()
	: m_oldID(0)
	{
		
	}
	
	void BindImpl( const GLuint resourceID )
	{
		const GLuint oldID = this->GetBoundID();
		
		if ( oldID != resourceID )
		{
			T::DoBind( resourceID );
			
			m_oldID = oldID;
		}
	}
	
	void UnbindImpl()
	{
		T::DoBind( m_oldID );
	}
	
private:
	GLuint m_oldID;
	
	GLuint GetBoundID()
	{
		GLint result;
		
		glGetIntegerv( T::GetBoundName(), &result );
		
		return static_cast< GLuint >( result );
	}
	
};


// ---------------------------------------------------------------------------


template < typename Type, 
	template < typename Type > class BindPolicy >
class Resource : private BindPolicy< Type >
{
public:
	Resource()
	: m_ID(0)
	{
		
	}
	
	~Resource()
	{
		this->Unbind();
	}
	
	void Bind()
	{
		GetBindPolicy()->BindImpl( m_ID );
	}
	
	void Unbind()
	{
		GetBindPolicy()->UnbindImpl();
	}
	
protected:
	GLuint m_ID;
	
private:
	typedef BindPolicy< Type >* BindPolicyPtr;
	
	BindPolicyPtr GetBindPolicy()
	{
		return static_cast< BindPolicyPtr >( this );
	}
	
}; // class Resource


// ---------------------------------------------------------------------------


enum TextureFormat
{
	TEXTURE_FORMAT_COLOR_RGBA,
	TEXTURE_FORMAT_DEPTH_STENCIL
};

enum TextureFilter
{
	TEXTURE_FILTER_NEAREST,
	TEXTURE_FILTER_LINEAR
};


GLint GetInternalFormat( const TextureFormat format );	
GLint GetFormat( const TextureFormat format );
GLint GetDataType( const TextureFormat format );

GLint GetFilter( const TextureFilter filter );


template < GLenum target >
inline GLenum GetTextureBoundName();

template <>
inline GLenum GetTextureBoundName< GL_TEXTURE_1D >()
{
	return GL_TEXTURE_BINDING_1D;
}

template <>
inline GLenum GetTextureBoundName< GL_TEXTURE_2D >()
{
	return GL_TEXTURE_BINDING_2D;
}	


template < GLenum target >
struct TextureImageHandler
{
	static void DoSetImageData( const TextureFormat format, const GLsizei width, const GLsizei height, const void* const data );
};

template <>
struct TextureImageHandler< GL_TEXTURE_1D >
{
	static void DoSetImageData( const TextureFormat format, const GLsizei width, const GLsizei, const void* const data )
	{
		gl.TexImage1D( GL_TEXTURE_1D, 0, GetInternalFormat( format ), 
			width, 0, GetFormat( format ), GetDataType( format ), data );
	}
};

template <>
struct TextureImageHandler< GL_TEXTURE_2D >
{
	static void DoSetImageData( const TextureFormat format, const GLsizei width, const GLsizei height, const void* const data )
	{
		gl.TexImage2D( GL_TEXTURE_2D, 0, GetInternalFormat( format ), 
			width, height, 0, GetFormat( format ), GetDataType( format ), data );
	}
};


template < GLenum target >
class Texture : public Resource< Texture< target >, UnbindToDefault >,
	private TextureImageHandler< target >
{
	friend class RenderTarget;
	
public:
	Texture()
	{
		gl.GenTextures( 1, &this->m_ID );
	}
	
	~Texture()
	{
		gl.DeleteTextures( 1, &this->m_ID );
	}
	
	
	static void DoBind( const GLuint resourceID )
	{
		gl.BindTexture( target, resourceID );
	}
	
	static GLenum GetBoundName()
	{
		return GetTextureBoundName< target >();
	}
	
	
	void SetFilter( const TextureFilter filter )
	{
		const GLint glFilter = GetFilter( filter );
		
		this->Bind();
		
		gl.TexParameteri( target, GL_TEXTURE_MIN_FILTER, glFilter );
		gl.TexParameteri( target, GL_TEXTURE_MAG_FILTER, glFilter );
		
		gl.TexParameteri( target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
		gl.TexParameteri( target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
		
		this->Unbind();
	}
	
	void SetImageData( const TextureFormat format, const GLsizei width, const GLsizei height, const void* const data )
	{
		this->Bind();
		this->DoSetImageData( format, width, height, data );
		this->Unbind();
	}
	
	
	void Draw2D( const GLsizei width, const GLsizei height )
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
		
		this->Bind();
		
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
		
		this->Unbind();
		
		gl.Enable( GL_ALPHA_TEST );
		gl.Enable( GL_BLEND );
	}
	
}; // class Texture


typedef Texture< GL_TEXTURE_1D > Texture1D;
typedef Texture< GL_TEXTURE_2D > Texture2D;


// ---------------------------------------------------------------------------


class RenderTarget : public Resource< RenderTarget, UnbindToPrevious >
{
public:
	RenderTarget( const GLsizei width, const GLsizei height );
	~RenderTarget();
	
	static void DoBind( const GLuint resourceID );
	static GLenum GetBoundName();
	
	Texture2D& GetColorTexture();
	
private:
	Texture2D m_color;
	Texture2D m_depthStencil;
	
}; // class RenderTarget


// ---------------------------------------------------------------------------


class ShaderProgram : public Resource< ShaderProgram, UnbindToDefault >
{
public:
	ShaderProgram( const char* const vertexName, const char* const fragmentName );
	~ShaderProgram();
	
	static void DoBind( const GLuint resourceID );
	static GLenum GetBoundName();
	
	void SetUniform( const char* const name, const GLint value );
	
private:
	GLuint m_vertexShaderID;
	GLuint m_fragmentShaderID;
	
}; // class ShaderProgram


// ---------------------------------------------------------------------------


class BackBuffer : public OpenGLFrameBuffer, private NonCopyable
{
	typedef OpenGLFrameBuffer Super;
	
public:
	BackBuffer( int width, int height, bool fullscreen );
	
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
	
private:
	RenderTarget  m_renderTarget;
	ShaderProgram m_gammaProgram;
	
	Texture1D m_gammaTexture;
	
	static const size_t GAMMA_TABLE_SIZE = 256;
	uint32_t m_gammaTable[ GAMMA_TABLE_SIZE ];
	
	static Parameters s_parameters;
	
	void DrawRenderTarget();
	
}; // class BackBuffer

} // namespace GLAuxilium


#endif // GL_SYSTEM_AUXILIUM_H_INCLUDED
