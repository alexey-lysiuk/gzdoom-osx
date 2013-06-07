//hqnx filter library
//----------------------------------------------------------
//Copyright (C) 2003 MaxSt ( maxst@hiend3d.com )
//Copyright (C) 2009 Benjamin Berkels
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU Lesser General Public
//License as published by the Free Software Foundation; either
//version 2.1 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
//Lesser General Public License for more details.
//
//You should have received a copy of the GNU Lesser General Public
//License along with this program; if not, write to the Free Software
//Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

#ifndef __HQNX_H__
#define __HQNX_H__

#pragma warning(disable:4799)

#include "Image.h"

#include <mmintrin.h>

#if defined _WIN32 || defined __APPLE__ && !defined __x86_64__ && !defined __clang__
// Visual Studio 2010 has no _mm_cvtsi64_m64() intrinsic
// LLVM GCC defines _mm_cvtsi64_m64() intrinsic for x64 only
static inline __m64 _mm_cvtsi64_m64(long long number)
{
  return *reinterpret_cast<__m64*>(&number);
}
#endif // _WIN32 || __APPLE__ && !__x86_64__ && !__clang__

class hq_vec
{
  // IMPORTANT NOTE!
  // This is not generic vectorized math class
  // Each member function or overloaded operator does specific task to simplify client code
  // To reimplement this class for different platform you need check very carefully
  // the Intel C++ Intrinsic Reference at http://software.intel.com/file/18072/

public:
  hq_vec(const int value)
  : m_value(_mm_cvtsi32_si64(value))
  {
  }

  hq_vec(const long long value)
  : m_value(_mm_cvtsi64_m64(value))
  {
  }

#define HQ_VEC_MM_ZERO _mm_cvtsi32_si64(0)

  static hq_vec load(const int source)
  {
    return _mm_unpacklo_pi8(_mm_cvtsi32_si64(source), HQ_VEC_MM_ZERO);
  }

  void store(unsigned char* const destination) const
  {
    *reinterpret_cast<int*>(destination) = _mm_cvtsi64_si32(_mm_packs_pu16(m_value, HQ_VEC_MM_ZERO));
  }

#undef HQ_VEC_MM_ZERO

  static void reset()
  {
    _mm_empty();
  }

  hq_vec& operator+=(const hq_vec& right)
  {
    m_value = _mm_add_pi16(m_value, right.m_value);
    return *this;
  }

  hq_vec& operator*=(const hq_vec& right)
  {
    m_value = _mm_mullo_pi16(m_value, right.m_value);
    return *this;
  }

  hq_vec& operator<<(const int count)
  {
    m_value = _mm_sll_pi16(m_value, _mm_cvtsi32_si64(count));
    return *this;
  }

  hq_vec& operator>>(const int count)
  {
    m_value = _mm_srl_pi16(m_value, _mm_cvtsi32_si64(count));
    return *this;
  }

private:
  __m64 m_value;

  hq_vec(const __m64 value)
  : m_value(value)
  {
  }

  friend hq_vec operator- (const hq_vec&, const hq_vec&);
  friend hq_vec operator* (const hq_vec&, const hq_vec&);
  friend hq_vec operator| (const hq_vec&, const hq_vec&);
  friend bool   operator!=(const int,     const hq_vec&);

};

inline hq_vec operator-(const hq_vec& left, const hq_vec& right)
{
  return _mm_subs_pu8(left.m_value, right.m_value);
}

inline hq_vec operator*(const hq_vec& left, const hq_vec& right)
{
  return _mm_mullo_pi16(left.m_value, right.m_value);
}

inline hq_vec operator|(const hq_vec& left, const hq_vec& right)
{
  return _mm_or_si64(left.m_value, right.m_value);
}

inline bool operator!=(const int left, const hq_vec& right)
{
  return left != _mm_cvtsi64_si32(right.m_value);
}

void DLL hq2x_32( int * pIn, unsigned char * pOut, int Xres, int Yres, int BpL );
void DLL hq3x_32( int * pIn, unsigned char * pOut, int Xres, int Yres, int BpL );
void DLL hq4x_32( int * pIn, unsigned char * pOut, int Xres, int Yres, int BpL );
int DLL hq4x_32 ( CImage &ImageIn, CImage &ImageOut );

void DLL InitLUTs();


#endif //__HQNX_H__