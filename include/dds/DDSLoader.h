#pragma once

#include <vector>

/*
 File Structure:
  Section     Length
  ///////////////////
  FILECODE     4
  DDS_HEADER   124
  HEADER_DX10* 20	(https://msdn.microsoft.com/en-us/library/bb943983(v=vs.85).aspx)
  bdata(2)     fseek(f, 0, SEEK_END); (ftell(f) - 128) - (fourCC == "DX10" ? 17 or 20 : 0)
* the link tells you that this section isn't written unless its a DX10 file
Supports DXT1, DXT3, DXT5, DXT10

File Byte Order:
typedef unsigned int DWORD;                           32bits little endian
  type   index      attribute                         description
///////////////////////////////////////////////////////////////////////////////////////////////
+-FILECODE
|  DWORD  0         Magic;                            magic number, always `DDS `, or 0x20534444
+-END OF FILECODE                                     
+-DDS_HEADER                                          
|  DWORD  4         size;                             size of the header, always 124 (includes PIXELFORMAT)
|  DWORD  8         flags;                            bitflags that tells you if data is present in the file
|                                                         CAPS         0x1
|                                                         HEIGHT       0x2
|                                                         WIDTH        0x4
|                                                         PITCH        0x8
|                                                         PIXELFORMAT  0x1000
|                                                         MIPMAPCOUNT  0x20000
|                                                         LINEARSIZE   0x80000
|                                                         DEPTH        0x800000
|  DWORD  12        height;                           height of the base image (biggest mipmap)
|  DWORD  16        width;                            width of the base image (biggest mipmap)
|  DWORD  20        pitchOrLinearSize;                bytes per scan line in an uncompressed texture, or bytes in the top level texture for a compressed texture
|                                                        D3DX11.lib and other similar libraries unreliably or inconsistently provide the pitch, convert with
|                                                        DX* && BC*: max( 1, ((width+3)/4) ) * block-size
|                                                        *8*8_*8*8 && UYVY && YUY2: ((width+1) >> 1) * 4
|                                                        (width * bits-per-pixel + 7)/8 (divide by 8 for byte alignment, whatever that means)
|  DWORD  24        depth;                            Depth of a volume texture (in pixels), garbage if no volume data
|  DWORD  28        mipMapCount;                      number of mipmaps, garbage if no pixel data
|  DWORD  32        reserved1[11];                    unused
|+-DDS_PIXELFORMAT  ddspf                             The pixel format DDS_PIXELFORMAT
|| DWORD  76        Size                              size of the following 32 bytes (PIXELFORMAT)
|| DWORD  80        Flags;                            bitflags that tells you if data is present in the file for following 28 bytes
||                                                        ALPHAPIXELS  0x1
||                                                        ALPHA        0x2
||                                                        FOURCC       0x4
||                                                        RGB          0x40
||                                                        YUV          0x200
||                                                        LUMINANCE    0x20000
|| DWORD  84        FourCC;                           File format: DXT1, DXT2, DXT3, DXT4, DXT5, DX10. 
|| DWORD  88        RGBBitCount;                      Bits per pixel
|| DWORD  92        RBitMask;                         Bit mask for R channel
|| DWORD  96        GBitMask;                         Bit mask for G channel
|| DWORD  100       BBitMask;                         Bit mask for B channel
|| DWORD  104       ABitMask;                         Bit mask for A channel
|+-END OF DDS_PIXELFORMAT                   
|  DWORD  108       caps;                             0x1000 for a texture w/o mipmaps
|                                                         0x401008 for a texture w/ mipmaps
|                                                         0x1008 for a cube map
|  DWORD  112       caps2;                            bitflags that tells you if data is present in the file
|                                                         CUBEMAP           0x200     Required for a cube map.
|                                                         CUBEMAP_POSITIVEX 0x400     Required when these surfaces are stored in a cube map.
|                                                         CUBEMAP_NEGATIVEX 0x800     ^
|                                                         CUBEMAP_POSITIVEY 0x1000    ^
|                                                         CUBEMAP_NEGATIVEY 0x2000    ^
|                                                         CUBEMAP_POSITIVEZ 0x4000    ^
|                                                         CUBEMAP_NEGATIVEZ 0x8000    ^
|                                                         VOLUME            0x200000  Required for a volume texture.
|  DWORD  114       caps3;                            unused
|  DWORD  116       caps4;                            unused
|  DWORD  120       reserved2;                        unused
+-END OF DDS_HEADER
+-DDS_HEADER_DXT10
|  UINT  124 dxgiFormat                               The surface pixel format
|  UINT  128 resourceDimension                        Identifies the type of resource. The following values for this member are a subset
|                                                     of the values in the D3D10_RESOURCE_DIMENSION or D3D11_RESOURCE_DIMENSION enumeration:
|  UINT  132 miscFlag                                 Identifies other, less common options for resources. The following value for this
|                                                     member is a subset of the values in the D3D10_RESOURCE_MISC_FLAG or D3D11_RESOURCE_MISC_FLAG enumeration:
|  UINT  136 arraySize                                The number of elements in the array.
|                                                     For a 2D texture that is also a cube-map texture, this number represents the number of cubes. This number
|                                                     is the same as the number in the NumCubes member of D3D10_TEXCUBE_ARRAY_SRV1 or D3D11_TEXCUBE_ARRAY_SRV).
|                                                     In this case, the DDS file contains arraySize*6 2D textures. For more information about this case, see the miscFlag description.
|                                                     For a 3D texture, you must set this number to 1.
|  UINT  140 miscFlags2                               Contains additional metadata (formerly was reserved). The lower 3 bits indicate the alpha
|                                                     mode of the associated resource. The upper 29 bits are reserved and are typically 0.
|  BYTE  144 bdata[]                                  A pointer to an array of bytes that contains the main surface data. 
|  BYTE  ^+bdata bdata2[]                             A pointer to an array of bytes that contains the remaining surfaces such as;
|                                                     mipmap levels, faces in a cube map, depths in a volume texture. Follow these
|                                                     links for more information about the DDS file layout for a: texture, a cube map, or a volume texture.
+-END OF DDS_HEADER_DXT10

*/

enum DXGI_FORMAT : int;

namespace Dds
{
  enum class Flag : uint8_t
  {
    None  = 0,
    DXT1  = 1 << 0,
    DXT3  = 1 << 1,
    DXT5  = 1 << 2,
    BC4_U = 1 << 3,
    BC5_U = 1 << 4,
    BC7   = 1 << 5
  };

  struct BitFlag
  {
    uint8_t flagValue = 0;

    void SetFlag(Flag t_flag) {
      flagValue |= static_cast<uint8_t>(t_flag);
    }

    void UnsetFlag(Flag t_flag) {
      flagValue &= ~static_cast<uint8_t>(t_flag);
    }

    void FlipFlag(Flag t_flag) {
      flagValue ^= static_cast<uint8_t>(t_flag);
    }

    [[nodiscard]] constexpr bool HasFlag(Flag t_flag) const {
      return (flagValue & static_cast<uint8_t>(t_flag)) == static_cast<uint8_t>(t_flag);
    }

    [[nodiscard]] constexpr bool HasAnyFlag(Flag t_multiFlag) const {
      return (flagValue & static_cast<uint8_t>(t_multiFlag)) != 0;
    }
  };

  // Enable bitwise operations for the enum
  constexpr Flag operator|(Flag t_a, Flag t_b) {
    return static_cast<Flag>(static_cast<uint8_t>(t_a) | static_cast<uint8_t>(t_b));
  }

  constexpr Flag operator&(Flag t_a, Flag t_b) {
    return static_cast<Flag>(static_cast<uint8_t>(t_a) & static_cast<uint8_t>(t_b));
  }

  enum D3D10_RESOURCE_DIMENSION : int // make sure we use the default base type NOLINT(performance-enum-size)
  {
    D3D10_RESOURCE_DIMENSION_UNKNOWN   = 0,
    D3D10_RESOURCE_DIMENSION_BUFFER    = 1,
    D3D10_RESOURCE_DIMENSION_TEXTURE1D = 2,
    D3D10_RESOURCE_DIMENSION_TEXTURE2D = 3,
    D3D10_RESOURCE_DIMENSION_TEXTURE3D = 4
  };
}

class LoadDds
{
public:
  // DDS structures are 1-byte packed
#pragma pack(push, 1)
  struct DDS_PIXELFORMAT
  {
    uint32_t dwSize;
    uint32_t dwFlags;
    uint32_t dwFourCC = 0; // <-- where "DXT1", "DXT3", "DXT5" live
    uint32_t dwRGBBitCount;
    uint32_t dwRBitMask;
    uint32_t dwGBitMask;
    uint32_t dwBBitMask;
    uint32_t dwABitMask;
  };

  struct DDS_HEADER
  {
    uint32_t        dwSize;
    uint32_t        dwFlags;
    uint32_t        dwHeight = 1;
    uint32_t        dwWidth  = 1;
    uint32_t        dwPitchOrLinearSize;
    uint32_t        dwDepth;
    uint32_t        dwMipMapCount = 1;
    uint32_t        dwReserved1[11];
    DDS_PIXELFORMAT ddspf; // offset 76
    uint32_t        dwCaps;
    uint32_t        dwCaps2;
    uint32_t        dwCaps3;
    uint32_t        dwCaps4;
    uint32_t        dwReserved2;
  };

  struct DDS_HEADER_DXT10
  {
    DXGI_FORMAT                   dxgiFormat        = static_cast<DXGI_FORMAT>(0); // DXGI_FORMAT_UNKNOWN
    Dds::D3D10_RESOURCE_DIMENSION resourceDimension = Dds::D3D10_RESOURCE_DIMENSION_UNKNOWN;
    uint32_t                      miscFlag          = 0;
    uint32_t                      arraySize         = 0;
    uint32_t                      miscFlags2        = 0;
  };
#pragma pack(pop)

  struct MIP_LEVEL
  {
    uint32_t               width  = 0;
    uint32_t               height = 0;
    std::vector<std::byte> data;
  };

  struct DDS_FILE
  {
    DDS_HEADER             header;
    DDS_HEADER_DXT10       dxt10Header;
    Dds::BitFlag           flags;
    uint32_t               blockSize = 0;
    uint32_t               glFormat  = 0; // fallback format
    std::vector<MIP_LEVEL> mipMaps;
    size_t                 totalSizeBytes = 0;

    DDS_FILE()                              = default;
    ~DDS_FILE()                             = default;
    DDS_FILE(DDS_FILE&& t_other)            = default;
    DDS_FILE(const DDS_FILE& t_other)       = delete;
    DDS_FILE& operator=(DDS_FILE&& t_other) = default;
    DDS_FILE& operator=(const DDS_FILE&)    = delete;
  };

  static DDS_FILE TextureLoadDds(const char* t_path);
  static void     FlipVerticalOnLoad(bool t_flip);

private:
  static constexpr uint32_t DXT1  = 0x31545844;
  static constexpr uint32_t DXT3  = 0x33545844;
  static constexpr uint32_t DXT5  = 0x35545844;
  static constexpr uint32_t DX10  = 0x30315844;
  static constexpr uint32_t BC5_U = 0x55354342;

#if !defined(GL_VERSION_4_2)
  enum OGL_FORMAT
  {
    GL_COMPRESSED_RGB_S3TC_DXT1_EXT        = 0x83F0, // DXT1 RGB linear
    GL_COMPRESSED_RGBA_S3TC_DXT1_EXT       = 0x83F1, // DXT1 RGBA linear
    GL_COMPRESSED_RGBA_S3TC_DXT3_EXT       = 0x83F2, // DXT3 RGBA linear
    GL_COMPRESSED_RGBA_S3TC_DXT5_EXT       = 0x83F3, // DXT5 RGBA linear
    GL_COMPRESSED_SRGB_S3TC_DXT1_EXT       = 0x8C4C, // DXT1 RGB sRGB 
    GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT = 0x8C4D, // DXT1 RGBA sRGB
    GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT = 0x8C4E, // DXT3 RGBA sRGB
    GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT = 0x8C4F, // DXT5 RGBA sRGB
    GL_COMPRESSED_RGBA_BPTC_UNORM          = 0x8E8C, // BC7 RGBA linear
    GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM    = 0x8E8D, // BC7 RGBA sRGB
    GL_COMPRESSED_RED_RGTC1                = 0x8DBB, // BC4u R linear
    GL_COMPRESSED_RG_RGTC2                 = 0x8DBD, // BC5n RG linear
  };
#endif

  static inline bool m_flipOnLoad = false;

  static bool ValidateExpectedSize(DDS_FILE&                               t_ddsFile,
                                   size_t                                  t_remainingBytes,
                                   const std::vector<std::byte>::iterator& t_itBegin);
  static void Flip(DDS_FILE& t_ddsFile);
  // general 4-byte row swap (DXT1 color/DXT3 alpha or color)
  static void Flip4ByteRow(std::byte* t_colorBlock);
  // DXT5 alpha / BC4 / BC5 single channel
  static void Flip3BitIndicesBlock(std::byte* t_block);
  // Flips a single 8-byte DXT1 block vertically
  static void FlipDxt1Block(std::byte* t_block);
  static void FlipDxt3Block(std::byte* t_block);
  // Flip a single 16-byte DXT5 block vertically
  static void FlipDxt5Block(std::byte* t_block);
  static void FlipBc4Block(std::byte* t_block);
  // Flip a single 16-byte BC5 block vertically (red + green channels)
  static void FlipBc5Block(std::byte* t_block);
};
