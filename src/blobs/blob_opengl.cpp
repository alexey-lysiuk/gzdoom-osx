
#include "gl/data/gl_data.cpp"
#include "gl/data/gl_portaldata.cpp"
#include "gl/data/gl_sections.cpp"
#include "gl/data/gl_setup.cpp"
#include "gl/data/gl_vertexbuffer.cpp"

#include "gl/dynlights/a_dynlight.cpp"
#include "gl/dynlights/gl_dynlight.cpp"
#include "gl/dynlights/gl_dynlight1.cpp"
#include "gl/dynlights/gl_glow.cpp"
#include "gl/dynlights/gl_lightbuffer.cpp"

#include "gl/models/gl_models.cpp"
#include "gl/models/gl_models_md2.cpp"
#include "gl/models/gl_models_md3.cpp"
#include "gl/models/gl_voxels.cpp"

#include "gl/renderer/gl_lightdata.cpp"
#include "gl/renderer/gl_renderer.cpp"
#include "gl/renderer/gl_renderstate.cpp"

#include "gl/scene/gl_bsp.cpp"
#include "gl/scene/gl_clipper.cpp"
#include "gl/scene/gl_decal.cpp"
#include "gl/scene/gl_drawinfo.cpp"
#include "gl/scene/gl_fakeflat.cpp"
#include "gl/scene/gl_flats.cpp"
#include "gl/scene/gl_portal.cpp"
#include "gl/scene/gl_renderhacks.cpp"
#include "gl/scene/gl_scene.cpp"
#include "gl/scene/gl_sky.cpp"
#include "gl/scene/gl_skydome.cpp"
#include "gl/scene/gl_sprite.cpp"
#include "gl/scene/gl_spritelight.cpp"
#include "gl/scene/gl_vertex.cpp"
#include "gl/scene/gl_walls.cpp"
#include "gl/scene/gl_walls_draw.cpp"
#include "gl/scene/gl_weapon.cpp"

#include "gl/shaders/gl_shader.cpp"
#include "gl/shaders/gl_texshader.cpp"

#include "gl/system/gl_framebuffer.cpp"
#include "gl/system/gl_interface.cpp"
#include "gl/system/gl_menu.cpp"
#include "gl/system/gl_threads.cpp"
#include "gl/system/gl_wipe.cpp"

#include "gl/textures/gl_bitmap.cpp"
#include "gl/textures/gl_hirestex.cpp"
#include "gl/textures/gl_hqresize.cpp"
#include "gl/textures/gl_hwtexture.cpp"
#include "gl/textures/gl_material.cpp"
#include "gl/textures/gl_skyboxtexture.cpp"
#include "gl/textures/gl_texture.cpp"
#include "gl/textures/gl_translate.cpp"

#include "gl/utility/gl_clock.cpp"
#include "gl/utility/gl_cycler.cpp"
#include "gl/utility/gl_geometric.cpp"

#ifdef NO_SDL
#include "cocoa/cocoaglvideo.cpp"
#else // !NO_SDL
#include "sdl/sdlglvideo.cpp"
#endif // NO_SDL
