# Things needs to be solved before public release

Graphical bugs:

1) Texture matrix are wrongs (UT2003)
2) ~Wrong transformations mode (3Dmark2001 - game 2), BF1942, H&D1 + all Unreal Engine 2 games)~
3) Wrong flipping for DDI < 7
4) Lighting TIJ
5) Projected textures has wrong coordinates GTA VC, TIJ

Runtime:
- locks
- short OpenGL test on start to determine configuration 

Optimizations:
1) cache CPU converted textures (chroma key, palette, DXT). **NOTE:** we're short of memory, specially when VM has less the 1 GB (!) RAM.

