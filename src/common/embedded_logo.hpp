#pragma once
#include <cstddef>
#include <cstdint>

namespace osmv::assets {
extern const uint8_t kLogoPng[];
extern const size_t kLogoPngSize;
// 32x32 ARGB32 in network byte order (A, R, G, B) for D-Bus StatusNotifierItem
extern const uint8_t kTrayIconArgb32[];
extern const int kTrayIconSize;
} // namespace osmv::assets
