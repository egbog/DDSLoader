#pragma once
#include <D3d10.h>
#include <Dxgiformat.h>
#include <fstream>
#include <string>
#include <unordered_map>
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

class LoadDds
{
public:
  // DDS structures are 1-byte packed
#pragma pack(push, 1)

  struct DDS_PIXELFORMAT
  {
    uint32_t dwSize;
    uint32_t dwFlags;
    uint32_t dwFourCC; // <-- where "DXT1", "DXT3", "DXT5" live
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
    uint32_t        dwHeight;
    uint32_t        dwWidth;
    uint32_t        dwPitchOrLinearSize;
    uint32_t        dwDepth;
    uint32_t        dwMipMapCount;
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
    DXGI_FORMAT              dxgiFormat;
    D3D10_RESOURCE_DIMENSION resourceDimension;
    unsigned int             miscFlag;
    unsigned int             arraySize;
    unsigned int             miscFlags2;
  };

  struct DDS_FILE
  {
    DDS_HEADER        header;
    DDS_HEADER_DXT10  dxt10Header;
    std::vector<char> file;
    unsigned int      blockSize;
    unsigned int      glFormat;
  };

#pragma pack(pop)

  static DDS_FILE TextureLoadDds(const char* t_path) {
    DDS_FILE ddsFile;

    // open the DDS file for binary reading and get file size
    std::ifstream file(t_path, std::ios::binary | std::ios::ate);

    if (!file) {
      throw std::runtime_error("Failed to open DDS file: " + std::string(t_path));
    }

    // get file size
    const long long fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    char magic[4];

    // read magic
    if (!file.read(magic, 4) || std::memcmp(magic, "DDS ", 4) != 0) {
      throw std::runtime_error("Not a DDS file");
    }

    // read header struct directly
    if (!file.read(reinterpret_cast<char*>(&ddsFile.header), sizeof(DDS_HEADER))) {
      throw std::runtime_error("Failed to read DDS header");
    }

    // read dxt10 header struct directly
    if (!file.read(reinterpret_cast<char*>(&ddsFile.dxt10Header), sizeof(DDS_HEADER_DXT10))) {
      throw std::runtime_error("Failed to read DDS header");
    }

    switch (ddsFile.header.ddspf.dwFourCC) {
      case DXT1:                   // "DXT1" little-endian
        ddsFile.glFormat = 0x83F1; // GL_COMPRESSED_RGBA_S3TC_DXT1_EXT
        ddsFile.blockSize = 8;
        break;
      case DXT3:                   // "DXT3"
        ddsFile.glFormat = 0x83F2; // GL_COMPRESSED_RGBA_S3TC_DXT3_EXT
        ddsFile.blockSize = 16;
        break;
      case DXT5:                   // "DXT5"
        ddsFile.glFormat = 0x83F3; // GL_COMPRESSED_RGBA_S3TC_DXT5_EXT
        ddsFile.blockSize = 16;
        break;
      case DX10: // "DX10" FourCC (DXT10 extension header)
        if (DxgiFormatToString(ddsFile.dxt10Header.dxgiFormat).ends_with("SRGB")) {
          ddsFile.glFormat = 0x8E8D; // COMPRESSED_SRGB_ALPHA_BPTC_UNORM_ARB
        }
        else {
          ddsFile.glFormat = 0x8E8C; // COMPRESSED_RGBA_BPTC_UNORM_ARB
        }
        ddsFile.blockSize = 16;
        break;
      default:
        file.close();
        throw std::runtime_error("Unsupported DDS format");
    }

    // read rest of file
    ddsFile.file.resize(fileSize - 128);
    if (!file.read(ddsFile.file.data(), fileSize - 128)) {
      file.close();
      throw std::runtime_error("Failed to read DDS file: " + std::string(t_path));
    }

    return ddsFile;
  }

private:
  static inline const std::unordered_map<DXGI_FORMAT, const char*> DXGI_FORMAT_NAMES = {
    {DXGI_FORMAT_UNKNOWN, "DXGI_FORMAT_UNKNOWN"}, {DXGI_FORMAT_R32G32B32A32_TYPELESS, "DXGI_FORMAT_R32G32B32A32_TYPELESS"},
    {DXGI_FORMAT_R32G32B32A32_FLOAT, "DXGI_FORMAT_R32G32B32A32_FLOAT"},
    {DXGI_FORMAT_R32G32B32A32_UINT, "DXGI_FORMAT_R32G32B32A32_UINT"},
    {DXGI_FORMAT_R32G32B32A32_SINT, "DXGI_FORMAT_R32G32B32A32_SINT"},
    {DXGI_FORMAT_R32G32B32_TYPELESS, "DXGI_FORMAT_R32G32B32_TYPELESS"},
    {DXGI_FORMAT_R32G32B32_FLOAT, "DXGI_FORMAT_R32G32B32_FLOAT"}, {DXGI_FORMAT_R32G32B32_UINT, "DXGI_FORMAT_R32G32B32_UINT"},
    {DXGI_FORMAT_R32G32B32_SINT, "DXGI_FORMAT_R32G32B32_SINT"},
    {DXGI_FORMAT_R16G16B16A16_TYPELESS, "DXGI_FORMAT_R16G16B16A16_TYPELESS"},
    {DXGI_FORMAT_R16G16B16A16_FLOAT, "DXGI_FORMAT_R16G16B16A16_FLOAT"},
    {DXGI_FORMAT_R16G16B16A16_UNORM, "DXGI_FORMAT_R16G16B16A16_UNORM"},
    {DXGI_FORMAT_R16G16B16A16_UINT, "DXGI_FORMAT_R16G16B16A16_UINT"},
    {DXGI_FORMAT_R16G16B16A16_SNORM, "DXGI_FORMAT_R16G16B16A16_SNORM"},
    {DXGI_FORMAT_R16G16B16A16_SINT, "DXGI_FORMAT_R16G16B16A16_SINT"},
    {DXGI_FORMAT_R32G32_TYPELESS, "DXGI_FORMAT_R32G32_TYPELESS"}, {DXGI_FORMAT_R32G32_FLOAT, "DXGI_FORMAT_R32G32_FLOAT"},
    {DXGI_FORMAT_R32G32_UINT, "DXGI_FORMAT_R32G32_UINT"}, {DXGI_FORMAT_R32G32_SINT, "DXGI_FORMAT_R32G32_SINT"},
    {DXGI_FORMAT_R32G8X24_TYPELESS, "DXGI_FORMAT_R32G8X24_TYPELESS"},
    {DXGI_FORMAT_D32_FLOAT_S8X24_UINT, "DXGI_FORMAT_D32_FLOAT_S8X24_UINT"},
    {DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS, "DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS"},
    {DXGI_FORMAT_X32_TYPELESS_G8X24_UINT, "DXGI_FORMAT_X32_TYPELESS_G8X24_UINT"},
    {DXGI_FORMAT_R10G10B10A2_TYPELESS, "DXGI_FORMAT_R10G10B10A2_TYPELESS"},
    {DXGI_FORMAT_R10G10B10A2_UNORM, "DXGI_FORMAT_R10G10B10A2_UNORM"},
    {DXGI_FORMAT_R10G10B10A2_UINT, "DXGI_FORMAT_R10G10B10A2_UINT"}, {DXGI_FORMAT_R11G11B10_FLOAT, "DXGI_FORMAT_R11G11B10_FLOAT"},
    {DXGI_FORMAT_R8G8B8A8_TYPELESS, "DXGI_FORMAT_R8G8B8A8_TYPELESS"}, {DXGI_FORMAT_R8G8B8A8_UNORM, "DXGI_FORMAT_R8G8B8A8_UNORM"},
    {DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, "DXGI_FORMAT_R8G8B8A8_UNORM_SRGB"},
    {DXGI_FORMAT_R8G8B8A8_UINT, "DXGI_FORMAT_R8G8B8A8_UINT"}, {DXGI_FORMAT_R8G8B8A8_SNORM, "DXGI_FORMAT_R8G8B8A8_SNORM"},
    {DXGI_FORMAT_R8G8B8A8_SINT, "DXGI_FORMAT_R8G8B8A8_SINT"}, {DXGI_FORMAT_R16G16_TYPELESS, "DXGI_FORMAT_R16G16_TYPELESS"},
    {DXGI_FORMAT_R16G16_FLOAT, "DXGI_FORMAT_R16G16_FLOAT"}, {DXGI_FORMAT_R16G16_UNORM, "DXGI_FORMAT_R16G16_UNORM"},
    {DXGI_FORMAT_R16G16_UINT, "DXGI_FORMAT_R16G16_UINT"}, {DXGI_FORMAT_R16G16_SNORM, "DXGI_FORMAT_R16G16_SNORM"},
    {DXGI_FORMAT_R16G16_SINT, "DXGI_FORMAT_R16G16_SINT"}, {DXGI_FORMAT_R32_TYPELESS, "DXGI_FORMAT_R32_TYPELESS"},
    {DXGI_FORMAT_D32_FLOAT, "DXGI_FORMAT_D32_FLOAT"}, {DXGI_FORMAT_R32_FLOAT, "DXGI_FORMAT_R32_FLOAT"},
    {DXGI_FORMAT_R32_UINT, "DXGI_FORMAT_R32_UINT"}, {DXGI_FORMAT_R32_SINT, "DXGI_FORMAT_R32_SINT"},
    {DXGI_FORMAT_R24G8_TYPELESS, "DXGI_FORMAT_R24G8_TYPELESS"}, {DXGI_FORMAT_D24_UNORM_S8_UINT, "DXGI_FORMAT_D24_UNORM_S8_UINT"},
    {DXGI_FORMAT_R24_UNORM_X8_TYPELESS, "DXGI_FORMAT_R24_UNORM_X8_TYPELESS"},
    {DXGI_FORMAT_X24_TYPELESS_G8_UINT, "DXGI_FORMAT_X24_TYPELESS_G8_UINT"},
    {DXGI_FORMAT_R8G8_TYPELESS, "DXGI_FORMAT_R8G8_TYPELESS"}, {DXGI_FORMAT_R8G8_UNORM, "DXGI_FORMAT_R8G8_UNORM"},
    {DXGI_FORMAT_R8G8_UINT, "DXGI_FORMAT_R8G8_UINT"}, {DXGI_FORMAT_R8G8_SNORM, "DXGI_FORMAT_R8G8_SNORM"},
    {DXGI_FORMAT_R8G8_SINT, "DXGI_FORMAT_R8G8_SINT"}, {DXGI_FORMAT_R16_TYPELESS, "DXGI_FORMAT_R16_TYPELESS"},
    {DXGI_FORMAT_R16_FLOAT, "DXGI_FORMAT_R16_FLOAT"}, {DXGI_FORMAT_D16_UNORM, "DXGI_FORMAT_D16_UNORM"},
    {DXGI_FORMAT_R16_UNORM, "DXGI_FORMAT_R16_UNORM"}, {DXGI_FORMAT_R16_UINT, "DXGI_FORMAT_R16_UINT"},
    {DXGI_FORMAT_R16_SNORM, "DXGI_FORMAT_R16_SNORM"}, {DXGI_FORMAT_R16_SINT, "DXGI_FORMAT_R16_SINT"},
    {DXGI_FORMAT_R8_TYPELESS, "DXGI_FORMAT_R8_TYPELESS"}, {DXGI_FORMAT_R8_UNORM, "DXGI_FORMAT_R8_UNORM"},
    {DXGI_FORMAT_R8_UINT, "DXGI_FORMAT_R8_UINT"}, {DXGI_FORMAT_R8_SNORM, "DXGI_FORMAT_R8_SNORM"},
    {DXGI_FORMAT_R8_SINT, "DXGI_FORMAT_R8_SINT"}, {DXGI_FORMAT_A8_UNORM, "DXGI_FORMAT_A8_UNORM"},
    {DXGI_FORMAT_R1_UNORM, "DXGI_FORMAT_R1_UNORM"}, {DXGI_FORMAT_R9G9B9E5_SHAREDEXP, "DXGI_FORMAT_R9G9B9E5_SHAREDEXP"},
    {DXGI_FORMAT_R8G8_B8G8_UNORM, "DXGI_FORMAT_R8G8_B8G8_UNORM"}, {DXGI_FORMAT_G8R8_G8B8_UNORM, "DXGI_FORMAT_G8R8_G8B8_UNORM"},
    {DXGI_FORMAT_BC1_TYPELESS, "DXGI_FORMAT_BC1_TYPELESS"}, {DXGI_FORMAT_BC1_UNORM, "DXGI_FORMAT_BC1_UNORM"},
    {DXGI_FORMAT_BC1_UNORM_SRGB, "DXGI_FORMAT_BC1_UNORM_SRGB"}, {DXGI_FORMAT_BC2_TYPELESS, "DXGI_FORMAT_BC2_TYPELESS"},
    {DXGI_FORMAT_BC2_UNORM, "DXGI_FORMAT_BC2_UNORM"}, {DXGI_FORMAT_BC2_UNORM_SRGB, "DXGI_FORMAT_BC2_UNORM_SRGB"},
    {DXGI_FORMAT_BC3_TYPELESS, "DXGI_FORMAT_BC3_TYPELESS"}, {DXGI_FORMAT_BC3_UNORM, "DXGI_FORMAT_BC3_UNORM"},
    {DXGI_FORMAT_BC3_UNORM_SRGB, "DXGI_FORMAT_BC3_UNORM_SRGB"}, {DXGI_FORMAT_BC4_TYPELESS, "DXGI_FORMAT_BC4_TYPELESS"},
    {DXGI_FORMAT_BC4_UNORM, "DXGI_FORMAT_BC4_UNORM"}, {DXGI_FORMAT_BC4_SNORM, "DXGI_FORMAT_BC4_SNORM"},
    {DXGI_FORMAT_BC5_TYPELESS, "DXGI_FORMAT_BC5_TYPELESS"}, {DXGI_FORMAT_BC5_UNORM, "DXGI_FORMAT_BC5_UNORM"},
    {DXGI_FORMAT_BC5_SNORM, "DXGI_FORMAT_BC5_SNORM"}, {DXGI_FORMAT_B5G6R5_UNORM, "DXGI_FORMAT_B5G6R5_UNORM"},
    {DXGI_FORMAT_B5G5R5A1_UNORM, "DXGI_FORMAT_B5G5R5A1_UNORM"}, {DXGI_FORMAT_B8G8R8A8_UNORM, "DXGI_FORMAT_B8G8R8A8_UNORM"},
    {DXGI_FORMAT_B8G8R8X8_UNORM, "DXGI_FORMAT_B8G8R8X8_UNORM"},
    {DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM, "DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM"},
    {DXGI_FORMAT_B8G8R8A8_TYPELESS, "DXGI_FORMAT_B8G8R8A8_TYPELESS"},
    {DXGI_FORMAT_B8G8R8A8_UNORM_SRGB, "DXGI_FORMAT_B8G8R8A8_UNORM_SRGB"},
    {DXGI_FORMAT_B8G8R8X8_TYPELESS, "DXGI_FORMAT_B8G8R8X8_TYPELESS"},
    {DXGI_FORMAT_B8G8R8X8_UNORM_SRGB, "DXGI_FORMAT_B8G8R8X8_UNORM_SRGB"},
    {DXGI_FORMAT_BC6H_TYPELESS, "DXGI_FORMAT_BC6H_TYPELESS"}, {DXGI_FORMAT_BC6H_UF16, "DXGI_FORMAT_BC6H_UF16"},
    {DXGI_FORMAT_BC6H_SF16, "DXGI_FORMAT_BC6H_SF16"}, {DXGI_FORMAT_BC7_TYPELESS, "DXGI_FORMAT_BC7_TYPELESS"},
    {DXGI_FORMAT_BC7_UNORM, "DXGI_FORMAT_BC7_UNORM"}, {DXGI_FORMAT_BC7_UNORM_SRGB, "DXGI_FORMAT_BC7_UNORM_SRGB"},
    {DXGI_FORMAT_AYUV, "DXGI_FORMAT_AYUV"}, {DXGI_FORMAT_Y410, "DXGI_FORMAT_Y410"}, {DXGI_FORMAT_Y416, "DXGI_FORMAT_Y416"},
    {DXGI_FORMAT_NV12, "DXGI_FORMAT_NV12"}, {DXGI_FORMAT_P010, "DXGI_FORMAT_P010"}, {DXGI_FORMAT_P016, "DXGI_FORMAT_P016"},
    {DXGI_FORMAT_420_OPAQUE, "DXGI_FORMAT_420_OPAQUE"}, {DXGI_FORMAT_YUY2, "DXGI_FORMAT_YUY2"},
    {DXGI_FORMAT_Y210, "DXGI_FORMAT_Y210"}, {DXGI_FORMAT_Y216, "DXGI_FORMAT_Y216"}, {DXGI_FORMAT_NV11, "DXGI_FORMAT_NV11"},
    {DXGI_FORMAT_AI44, "DXGI_FORMAT_AI44"}, {DXGI_FORMAT_IA44, "DXGI_FORMAT_IA44"}, {DXGI_FORMAT_P8, "DXGI_FORMAT_P8"},
    {DXGI_FORMAT_A8P8, "DXGI_FORMAT_A8P8"}, {DXGI_FORMAT_B4G4R4A4_UNORM, "DXGI_FORMAT_B4G4R4A4_UNORM"},
    {DXGI_FORMAT_P208, "DXGI_FORMAT_P208"}, {DXGI_FORMAT_V208, "DXGI_FORMAT_V208"}, {DXGI_FORMAT_V408, "DXGI_FORMAT_V408"},
    {DXGI_FORMAT_SAMPLER_FEEDBACK_MIN_MIP_OPAQUE, "DXGI_FORMAT_SAMPLER_FEEDBACK_MIN_MIP_OPAQUE"},
    {DXGI_FORMAT_SAMPLER_FEEDBACK_MIP_REGION_USED_OPAQUE, "DXGI_FORMAT_SAMPLER_FEEDBACK_MIP_REGION_USED_OPAQUE"},
    {DXGI_FORMAT_A4B4G4R4_UNORM, "DXGI_FORMAT_A4B4G4R4_UNORM"}, {DXGI_FORMAT_FORCE_UINT, "DXGI_FORMAT_FORCE_UINT"}};

  static std::string DxgiFormatToString(const DXGI_FORMAT t_fmt) {
    const auto it = DXGI_FORMAT_NAMES.find(t_fmt);
    return it != DXGI_FORMAT_NAMES.end() ? it->second : "UNKNOWN_DXGI_FORMAT";
  }

  static constexpr uint32_t DXT1 = 0x31545844; // "DXT1"
  static constexpr uint32_t DXT3 = 0x33545844; // "DXT3"
  static constexpr uint32_t DXT5 = 0x35545844; // "DXT5"
  static constexpr uint32_t DX10 = 0x30315844; // "DX10"
};
