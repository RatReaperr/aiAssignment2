#ifndef _SDL3_TYPEDEFS_HPP_
#define _SDL3_TYPEDEFS_HPP_

#include <SDL3/SDL.h> // SDL_Init, SDL_CreateWindow etc.
#include <memory>     // std::unique_ptr

// Named deleters (not lambdas) give external linkage, avoiding -Wsubobject-linkage warnings.
struct DestroyWindow   { void operator()(SDL_Window*   w) const { SDL_DestroyWindow(w);   } };
struct DestroyRenderer { void operator()(SDL_Renderer* r) const { SDL_DestroyRenderer(r); } };
struct DestroyTexture  { void operator()(SDL_Texture*  t) const { SDL_DestroyTexture(t);  } };
struct DestroySurface  { void operator()(SDL_Surface*  s) const { SDL_DestroySurface(s);  } };

using WindowPtr   = std::unique_ptr<SDL_Window,   DestroyWindow>;
using RendererPtr = std::unique_ptr<SDL_Renderer, DestroyRenderer>;
using TexturePtr  = std::unique_ptr<SDL_Texture,  DestroyTexture>;
using SurfacePtr  = std::unique_ptr<SDL_Surface,  DestroySurface>;

#endif // _SDL3_TYPEDEFS_HPP_
