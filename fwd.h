#ifndef FWD_H
#define FWD_H

class Volume;
typedef std::unique_ptr<Volume> VolumePtr;

class Volume3d;
typedef std::unique_ptr<Volume3d> Volume3dPtr;

class Slice;
typedef std::unique_ptr<Slice> SlicePtr;

class IsoSurface;
typedef std::unique_ptr<IsoSurface> IsoSurfacePtr;

class Material;
typedef std::shared_ptr<Material> MaterialSharedPtr;

class Surface2D;
typedef std::unique_ptr<Surface2D> Surface2DPtr;

class Cube;
typedef std::unique_ptr<Cube> CubePtr;

class Mesh;
typedef std::unique_ptr<Mesh> MeshPtr;

class Lines;
typedef std::unique_ptr<Lines> LinesPtr;

class Texture;
typedef std::shared_ptr<Texture> TexturePtr;

#endif /* FWD_H */
