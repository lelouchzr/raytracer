# Third-party notices

The root MIT license covers the original code and documentation contributed to this remake. Existing third-party rights and notices remain applicable.

| Component | License and scope |
|---|---|
| Dear ImGui 1.91.9b | MIT; [upstream reference](bonus/vendor/imgui/UPSTREAM.md) and [full license](bonus/vendor/imgui/LICENSE.txt). Compiled into the native studio. |
| SDL2 | [zlib license](https://github.com/libsdl-org/SDL/blob/SDL2/LICENSE.txt); separately installed runtime. |
| SFML | [zlib/png license](https://github.com/SFML/SFML/blob/master/license.md); separately installed runtime. |
| zlib | [zlib license](https://zlib.net/zlib_license.html); separately installed runtime. |
| Original scene fixtures | Preserved for compatibility testing; [scope](tests/legacy/README.md). Not included in binary archives. |
| PBRT comparison | Reference images and measurements used for validation; PBRT source and executable are not included in binary archives. |

Release archives contain Raytracer, its example plugin, authored scenes/assets, and a copy of the Dear ImGui license. They do not bundle SDL2, SFML, zlib, fonts, graphics drivers, or their transitive runtimes. Install the dependencies listed in the archive README and manifest.
