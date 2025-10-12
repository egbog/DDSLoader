#include "dds/DDSLoader.h"

#include <Dxgiformat.h>
#include <fstream>
#include <iostream>
#include <string>

LoadDds::DDS_FILE::~DDS_FILE() {
  delete [] file;
  delete [] mipSizeBytes;
}

LoadDds::DDS_FILE::DDS_FILE(DDS_FILE&& t_other) noexcept {
  file                 = t_other.file;
  t_other.file         = nullptr;
  mipSizeBytes         = t_other.mipSizeBytes;
  t_other.mipSizeBytes = nullptr;

  header      = t_other.header;
  dxt10Header = t_other.dxt10Header;
  blockSize   = t_other.blockSize;
  glFormat    = t_other.glFormat;
  flags       = t_other.flags;
}

LoadDds::DDS_FILE& LoadDds::DDS_FILE::operator=(DDS_FILE&& t_other) noexcept {
  if (this != &t_other) {
    file         = t_other.file;
    t_other.file = nullptr;
    mipSizeBytes         = t_other.mipSizeBytes;
  t_other.mipSizeBytes = nullptr;

    header      = t_other.header;
    dxt10Header = t_other.dxt10Header;
    blockSize   = t_other.blockSize;
    glFormat    = t_other.glFormat;
    flags       = t_other.flags;
  }
  return *this;
}

LoadDds::DDS_FILE LoadDds::TextureLoadDds(const char* t_path) {
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
    //ddsFile.file = std::make_unique<std::byte[]>(remainingBytes);
    ddsFile.file = new std::byte[remainingBytes];
    std::memcpy(ddsFile.file, buffer.get() + headerOffset, remainingBytes);

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

    ddsFile.mipSizeBytes = new size_t[ddsFile.header.dwMipMapCount];

    // verify we read all bytes based on the block size, mip maps and resolution
    if (!ValidateExpectedSize(ddsFile, remainingBytes)) {
      throw std::runtime_error("Data size smaller than expected (corrupt or mismatched header)");
    }

    if (m_flipOnLoad) {
      Flip(ddsFile);
    }

    return ddsFile;
  }
  catch (const std::runtime_error& e) {
    std::cerr << "[DDS] - Error: " << e.what() << '\n';
    return {}; // return default initialized
  }
}

void LoadDds::FlipVerticalOnLoad(const bool t_flip) {
  m_flipOnLoad = t_flip;
}

bool LoadDds::ValidateExpectedSize(const DDS_FILE& t_ddsFile, const size_t t_remainingBytes) {
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
    const size_t mipSize = mipSurfaceSize(w, h);
    expectedSize += mipSize;
    t_ddsFile.mipSizeBytes[mip] = mipSize;
    w = std::max(1u, w / 2u);
    h = std::max(1u, h / 2u);
  }

  return t_remainingBytes >= expectedSize;
}

void LoadDds::Flip(const DDS_FILE& t_ddsFile) {
  const uint32_t width     = t_ddsFile.header.dwWidth;
  const uint32_t height    = t_ddsFile.header.dwHeight;
  const uint32_t blockSize = t_ddsFile.blockSize;
  const uint32_t mipCount  = t_ddsFile.header.dwMipMapCount;
  size_t         offset    = 0;

  for (uint32_t mip = 0; mip < mipCount; ++mip) {
    // this mip's resolution
    const size_t mipWidth  = std::max(static_cast<uint32_t>(1), width >> mip);
    const size_t mipHeight = std::max(static_cast<uint32_t>(1), height >> mip);

    const size_t blocksWide = (mipWidth + 3) / 4;
    const size_t blocksHigh = (mipHeight + 3) / 4;

    std::byte* mipPtr = t_ddsFile.file + offset;

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
void LoadDds::Flip4ByteRow(std::byte* t_colorBlock) {
  std::swap(t_colorBlock[0], t_colorBlock[3]);
  std::swap(t_colorBlock[1], t_colorBlock[2]);
}

// DXT5 alpha / BC4 / BC5 single channel
void LoadDds::Flip3BitIndicesBlock(std::byte* t_block) {
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
void LoadDds::FlipDxt1Block(std::byte* t_block) {
  // layout: 2 bytes color0, 2 bytes color1, 4 bytes indices
  // Break indices into 4 rows (8 bits each)
  Flip4ByteRow(t_block + 4);

  // Colors stay the same, no need to touch them
}

void LoadDds::FlipDxt3Block(std::byte* t_block) {
  // Alpha: 4x4 pixels, 4 bits each
  Flip4ByteRow(t_block);

  // Color indices (like DXT1): 4 bytes starting at t_block + 12
  Flip4ByteRow(t_block + 12);
}

// Flip a single 16-byte DXT5 block vertically
void LoadDds::FlipDxt5Block(std::byte* t_block) {
  // Flip alpha (first 8 bytes as BC4)
  Flip3BitIndicesBlock(t_block);

  // Flip color indices (like DXT1)
  Flip4ByteRow(t_block + 12);
}

void LoadDds::FlipBc4Block(std::byte* t_block) {
  Flip3BitIndicesBlock(t_block);
}

// Flip a single 16-byte BC5 block vertically (red + green channels)
void LoadDds::FlipBc5Block(std::byte* t_block) {
  Flip3BitIndicesBlock(t_block);     // Red channel
  Flip3BitIndicesBlock(t_block + 8); // Green channel
}
