#pragma once
#include <Dxgiformat.h>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>

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
    DXGI_FORMAT                   dxgiFormat        = DXGI_FORMAT_UNKNOWN;
    Dds::D3D10_RESOURCE_DIMENSION resourceDimension = Dds::D3D10_RESOURCE_DIMENSION_UNKNOWN;
    unsigned int                  miscFlag          = 0;
    unsigned int                  arraySize         = 0;
    unsigned int                  miscFlags2        = 0;
  };

  struct DDS_FILE
  {
    DDS_HEADER                   header;
    DDS_HEADER_DXT10             dxt10Header;
    std::unique_ptr<std::byte[]> file;
    unsigned int                 blockSize = 0;
    unsigned int                 glFormat  = 0; // fallback format
    Dds::BitFlag                 flags;
  };

#pragma pack(pop)

  static DDS_FILE TextureLoadDds(const char* t_path) {
    try {
      DDS_FILE ddsFile;

      // open the DDS file for binary reading and get file size
      std::ifstream file(t_path, std::ios::binary | std::ios::ate);

      if (!file) {
        throw std::runtime_error("Failed to open file: " + std::string(t_path));
      }

      // get file size
      std::streampos pos = file.tellg();
      // validate
      if (pos <= 0) {
        throw std::runtime_error("Failed to determine file size");
      }

      const size_t fileSize = pos;
      file.seekg(0, std::ios::beg);

      if (fileSize == 0) {
        // avoid undefined behavior
        throw std::runtime_error("Filesize 0");
      }

      // read whole file at once
      auto buffer = std::make_unique<std::byte[]>(fileSize);
      if (!file.read(reinterpret_cast<char*>(buffer.get()), static_cast<std::streamsize>(fileSize))) {
        throw std::runtime_error("DDS: Failed to read file");
      }

      if (std::memcmp(buffer.get(), "DDS ", 4) != 0) {
        throw std::runtime_error("Not a .dds file");
      }

      size_t headerOffset = 4;

      // copy header
      std::memcpy(&ddsFile.header, buffer.get() + headerOffset, sizeof(DDS_HEADER));
      headerOffset += sizeof(DDS_HEADER);

      // handle DX10 header if present
      if (ddsFile.header.ddspf.dwFourCC == DX10) {
        std::memcpy(&ddsFile.dxt10Header, buffer.get() + headerOffset, sizeof(DDS_HEADER_DXT10));
        headerOffset += sizeof(DDS_HEADER_DXT10);
      }

      // make sure mip map is always at least 1
      ddsFile.header.dwMipMapCount = ddsFile.header.dwMipMapCount ? ddsFile.header.dwMipMapCount : 1;

      size_t remainingBytes = fileSize - headerOffset;

      // remaining compressed data
      ddsFile.file = std::make_unique<std::byte[]>(remainingBytes);
      std::memcpy(ddsFile.file.get(), buffer.get() + headerOffset, remainingBytes);

      switch (ddsFile.header.ddspf.dwFourCC) {
        case DXT1:                                                   // little-endian
          ddsFile.glFormat = GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT; // assume sRGB for all non-DXT10 header files
          ddsFile.blockSize = 8;
          ddsFile.flags.SetFlag(Dds::Flag::DXT1);
          break;
        case DXT3:
          ddsFile.glFormat = GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT;
          ddsFile.blockSize = 16;
          ddsFile.flags.SetFlag(Dds::Flag::DXT3);
          break;
        case DXT5:
          ddsFile.glFormat = GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT;
          ddsFile.blockSize = 16;
          ddsFile.flags.SetFlag(Dds::Flag::DXT5);
          break;
        case BC5_U: // non DXT10 header BC5u
          ddsFile.glFormat = GL_COMPRESSED_RG_RGTC2;
          ddsFile.blockSize = 16;
          ddsFile.flags.SetFlag(Dds::Flag::BC5_U);
          break;
        case DX10:                                  // FourCC (DXT10 extension header)
          switch (ddsFile.dxt10Header.dxgiFormat) { // NOLINT(clang-diagnostic-switch-enum)
            case DXGI_FORMAT_BC1_UNORM:
            case DXGI_FORMAT_BC1_TYPELESS:
              ddsFile.glFormat = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT; // DXT1
              ddsFile.blockSize = 8;
              ddsFile.flags.SetFlag(Dds::Flag::DXT1);
              break;
            case DXGI_FORMAT_BC1_UNORM_SRGB:
              ddsFile.glFormat = GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT; // DXT1
              ddsFile.blockSize = 8;
              ddsFile.flags.SetFlag(Dds::Flag::DXT1);
              break;
            case DXGI_FORMAT_BC2_UNORM:
            case DXGI_FORMAT_BC2_TYPELESS:
              ddsFile.glFormat = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT; // DXT3
              ddsFile.blockSize = 16;
              ddsFile.flags.SetFlag(Dds::Flag::DXT3);
              break;
            case DXGI_FORMAT_BC2_UNORM_SRGB:
              ddsFile.glFormat = GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT; // DXT3
              ddsFile.blockSize = 16;
              ddsFile.flags.SetFlag(Dds::Flag::DXT3);
              break;
            case DXGI_FORMAT_BC3_UNORM:
            case DXGI_FORMAT_BC3_TYPELESS:
              ddsFile.glFormat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT; // DXT5
              ddsFile.blockSize = 16;
              ddsFile.flags.SetFlag(Dds::Flag::DXT5);
              break;
            case DXGI_FORMAT_BC3_UNORM_SRGB:
              ddsFile.glFormat = GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT; // DXT5
              ddsFile.blockSize = 16;
              ddsFile.flags.SetFlag(Dds::Flag::DXT5);
              break;
            case DXGI_FORMAT_BC4_UNORM:
            case DXGI_FORMAT_BC4_TYPELESS:
              ddsFile.glFormat = GL_COMPRESSED_RED_RGTC1; // BC4u
              ddsFile.blockSize = 8;
              ddsFile.flags.SetFlag(Dds::Flag::BC4_U);
              break;
            case DXGI_FORMAT_BC4_SNORM:
              throw std::runtime_error("Unsupported DX10 DXGI_FORMAT: DXGI_FORMAT_BC4_SNORM");
            case DXGI_FORMAT_BC5_TYPELESS:
            case DXGI_FORMAT_BC5_UNORM:
              ddsFile.glFormat = GL_COMPRESSED_RG_RGTC2; // BC5u
              ddsFile.blockSize = 16;
              ddsFile.flags.SetFlag(Dds::Flag::BC5_U);
              break;
            case DXGI_FORMAT_BC5_SNORM:
              throw std::runtime_error("Unsupported DX10 DXGI_FORMAT: DXGI_FORMAT_BC5_SNORM");
            case DXGI_FORMAT_BC7_UNORM:
            case DXGI_FORMAT_BC7_TYPELESS:
              ddsFile.glFormat = GL_COMPRESSED_RGBA_BPTC_UNORM; // BC7
              ddsFile.blockSize = 16;
              ddsFile.flags.SetFlag(Dds::Flag::BC7);
              break;
            case DXGI_FORMAT_BC7_UNORM_SRGB:
              ddsFile.glFormat = GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM; // BC7
              ddsFile.blockSize = 16;
              ddsFile.flags.SetFlag(Dds::Flag::BC7);
              break;
            default:
              throw std::runtime_error("Unsupported DX10 DXGI_FORMAT");
          }
          break;
        default:
          throw std::runtime_error("Unsupported format");
      }

      // verify we read all bytes based on the block size, mip maps and resolution
      if (!ValidateExpectedSize(ddsFile, remainingBytes)) {
        throw std::runtime_error("Data size smaller than expected (corrupt or mismatched header)");
      }

      if (m_flipOnLoad) {
        std::cout << "Flipped data" << '\n';
        Flip(ddsFile);
      }

      return ddsFile;
    }
    catch (const std::runtime_error& e) {
      std::cerr << "[DDS] - Error: " << e.what() << '\n';
      return {}; // return default initialized
    }
  }

  static constexpr void FlipVerticalOnLoad(const bool t_flip) {
    m_flipOnLoad = t_flip;
  }

private:
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

  static constexpr uint32_t DXT1  = 0x31545844;
  static constexpr uint32_t DXT3  = 0x33545844;
  static constexpr uint32_t DXT5  = 0x35545844;
  static constexpr uint32_t DX10  = 0x30315844;
  static constexpr uint32_t BC5_U = 0x55354342;

  static inline bool m_flipOnLoad = false;

  static bool ValidateExpectedSize(const DDS_FILE& t_ddsFile, const size_t t_remainingBytes) {
    // compute expected size (compressed) and validate
    auto mipSurfaceSize = [&] (const uint32_t t_w, const uint32_t t_h)-> size_t
    {
      const uint32_t blocksW = (t_w + 3) / 4;
      const uint32_t blocksH = (t_h + 3) / 4;
      return static_cast<size_t>(blocksW) * static_cast<size_t>(blocksH) * static_cast<size_t>(t_ddsFile.blockSize);
    };

    size_t   expectedSize = 0;
    uint32_t w            = t_ddsFile.header.dwWidth;
    uint32_t h            = t_ddsFile.header.dwHeight;
    for (uint32_t mip = 0; mip < t_ddsFile.header.dwMipMapCount; ++mip) {
      expectedSize += mipSurfaceSize(w, h);
      w = std::max(1u, w / 2u);
      h = std::max(1u, h / 2u);
    }

    return t_remainingBytes >= expectedSize;
  }

  static void Flip(const DDS_FILE& t_ddsFile) {
    const uint32_t width     = t_ddsFile.header.dwWidth;
    const uint32_t height    = t_ddsFile.header.dwHeight;
    const uint32_t blockSize = t_ddsFile.blockSize;
    const uint32_t mipCount  = t_ddsFile.header.dwMipMapCount;
    auto           data      = t_ddsFile.file.get();
    size_t         offset    = 0;

    for (uint32_t mip = 0; mip < mipCount; ++mip) {
      // this mip's resolution
      const size_t mipWidth  = std::max(static_cast<uint32_t>(1), width >> mip);
      const size_t mipHeight = std::max(static_cast<uint32_t>(1), height >> mip);

      const size_t blocksWide = (mipWidth + 3) / 4;
      const size_t blocksHigh = (mipHeight + 3) / 4;

      std::byte* mipPtr = data + offset;

      const size_t rowSize = blocksWide * blockSize;

      // flip blocks vertically row by row
      for (unsigned int y = 0; y < blocksHigh / 2; ++y) {
        std::byte* topRow    = mipPtr + y * rowSize;
        std::byte* bottomRow = mipPtr + (blocksHigh - 1 - y) * rowSize;
        for (unsigned int x = 0; x < blocksWide; ++x) {
          std::byte* topBlock    = topRow + static_cast<size_t>(x * blockSize);
          std::byte* bottomBlock = bottomRow + static_cast<size_t>(x * blockSize);

          if (t_ddsFile.flags.HasFlag(Dds::Flag::DXT1)) {
            FlipDxt1Block(topBlock);
            FlipDxt1Block(bottomBlock);
          }
          else if (t_ddsFile.flags.HasFlag(Dds::Flag::DXT3)) {
            FlipDxt3Block(topBlock);
            FlipDxt3Block(bottomBlock);
          }
          else if (t_ddsFile.flags.HasFlag(Dds::Flag::DXT5)) {
            FlipDxt5Block(topBlock);
            FlipDxt5Block(bottomBlock);
          }
          else if (t_ddsFile.flags.HasFlag(Dds::Flag::BC4_U)) {
            FlipBc4Block(topBlock);
            FlipBc4Block(bottomBlock);
          }
          else if (t_ddsFile.flags.HasFlag(Dds::Flag::BC5_U)) {
            FlipBc5Block(topBlock);
            FlipBc5Block(bottomBlock);
          }
        }
      }

      if (blocksHigh % 2 == 1) {
        std::byte* middleRow = mipPtr + (blocksHigh / 2) * rowSize;
        for (unsigned int z = 0; z < blocksWide; ++z) {
          if (t_ddsFile.flags.HasFlag(Dds::Flag::DXT1)) {
            FlipDxt1Block(middleRow + static_cast<size_t>(z * blockSize));
          }
          else if (t_ddsFile.flags.HasFlag(Dds::Flag::DXT3)) {
            FlipDxt3Block(middleRow + static_cast<size_t>(z * blockSize));
          }
          else if (t_ddsFile.flags.HasFlag(Dds::Flag::DXT5)) {
            FlipDxt5Block(middleRow + static_cast<size_t>(z * blockSize));
          }
          else if (t_ddsFile.flags.HasFlag(Dds::Flag::BC4_U)) {
            FlipBc4Block(middleRow + static_cast<size_t>(z * blockSize));
          }
          else if (t_ddsFile.flags.HasFlag(Dds::Flag::BC5_U)) {
            FlipBc5Block(middleRow + static_cast<size_t>(z * blockSize));
          }
        }
      }

      // move offset to next mip
      offset += rowSize * blocksHigh;
    }
  }

  // general 4-byte row swap (DXT1 color/DXT3 alpha or color)
  static void Flip4ByteRow(std::byte* t_colorBlock) {
    std::swap(t_colorBlock[0], t_colorBlock[3]);
    std::swap(t_colorBlock[1], t_colorBlock[2]);
  }

  // DXT5 alpha / BC4 / BC5 single channel
  static void Flip3BitIndicesBlock(std::byte* t_block) {
    // Extract 6-byte 3-bit indices like DXT5 alpha
    uint8_t indices[6];
    for (int i = 0; i < 6; ++i) {
      indices[i] = std::to_integer<uint8_t>(t_block[2 + i]);
    }

    uint8_t  grid[16];
    uint64_t bits = 0;
    for (int i = 0; i < 6; ++i) {
      bits |= static_cast<uint64_t>(indices[i]) << (8 * i);
    }
    for (int i = 0; i < 16; ++i) {
      grid[i] = static_cast<uint8_t>((bits >> (3 * i)) & 0x7);
    }

    // Flip 4x4 vertically
    for (int x = 0; x < 4; ++x) {
      std::swap(grid[0 * 4 + x], grid[3 * 4 + x]);
      std::swap(grid[1 * 4 + x], grid[2 * 4 + x]);
    }

    // Repack back into 6 bytes
    bits = 0;
    for (int i = 0; i < 16; ++i) {
      bits |= static_cast<uint64_t>(grid[i] & 0x7) << (3 * i);
    }
    for (int i = 0; i < 6; ++i) {
      t_block[2 + i] = static_cast<std::byte>((bits >> (8 * i)) & 0xFF);
    }
  }

  // Flips a single 8-byte DXT1 block vertically
  static void FlipDxt1Block(std::byte* t_block) {
    // layout: 2 bytes color0, 2 bytes color1, 4 bytes indices
    // Break indices into 4 rows (8 bits each)
    Flip4ByteRow(t_block + 4);

    // Colors stay the same, no need to touch them
  }

  static void FlipDxt3Block(std::byte* t_block) {
    // Alpha: 4x4 pixels, 4 bits each
    Flip4ByteRow(t_block);

    // Color indices (like DXT1): 4 bytes starting at t_block + 12
    Flip4ByteRow(t_block + 12);
  }

  // Flip a single 16-byte DXT5 block vertically
  static void FlipDxt5Block(std::byte* t_block) {
    // Flip alpha (first 8 bytes as BC4)
    Flip3BitIndicesBlock(t_block);

    // Flip color indices (like DXT1)
    Flip4ByteRow(t_block + 12);
  }

  static void FlipBc4Block(std::byte* t_block) {
    Flip3BitIndicesBlock(t_block);
  }

  // Flip a single 16-byte BC5 block vertically (red + green channels)
  static void FlipBc5Block(std::byte* t_block) {
    Flip3BitIndicesBlock(t_block);     // Red channel
    Flip3BitIndicesBlock(t_block + 8); // Green channel
  }
};
