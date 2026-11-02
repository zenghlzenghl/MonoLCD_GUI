#include "GUI/Font.h"

/*============================================================================
 * 汉字检索：UTF-8 三字节二分查找（表按 UTF-8 字节序升序排列）
 *==========================================================================*/

uint8_t FontCnFind(uint8_t b0, uint8_t b1, uint8_t b2)
{
    uint8_t lo = 0u;
    uint8_t hi = (uint8_t)FONT_CN16_COUNT;
    uint8_t mid;

    while (lo < hi) {
        mid = (uint8_t)((lo + hi) >> 1u);
        if (b0 < FontCnKey[mid][0u]) {
            hi = mid;
        } else if (b0 > FontCnKey[mid][0u]) {
            lo = (uint8_t)(mid + 1u);
        } else if (b1 < FontCnKey[mid][1u]) {
            hi = mid;
        } else if (b1 > FontCnKey[mid][1u]) {
            lo = (uint8_t)(mid + 1u);
        } else if (b2 < FontCnKey[mid][2u]) {
            hi = mid;
        } else if (b2 > FontCnKey[mid][2u]) {
            lo = (uint8_t)(mid + 1u);
        } else {
            return mid;
        }
    }
    return FONT_CN_NOT_FOUND;
}

const uint8_t ROM *FontCnData(uint8_t idx)
{
    if (idx >= (uint8_t)FONT_CN16_COUNT) {
        idx = 0u;
    }
    return &FontCn16Data[idx][0u];
}
