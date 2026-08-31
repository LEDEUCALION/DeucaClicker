#pragma once

//
// L'unique endroit où <Windows.h> est inclus.
//
// L'inclure ailleurs, c'est la façon dont un projet finit avec des macros
// min/max qui dévorent std::min, et avec un en-tête qui ne compile plus le jour
// où quelqu'un réordonne deux inclusions. Tout ce qui suit est défensif : les
// mêmes définitions sont posées dans CMake, mais une unité de traduction
// égarée qui arriverait ici en premier doit tout de même voir un Windows
// allégé et sans macros parasites.
//

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#ifndef NOSERVICE
#define NOSERVICE
#endif

#ifndef NOMCX
#define NOMCX
#endif

#ifndef NOIME
#define NOIME
#endif

#include <Windows.h>
