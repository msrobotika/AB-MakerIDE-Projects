#pragma once
#include "Reglas.h"

// ---------- Tabla OUI ampliable: asignaciones MA-L de IEEE ----------
// Fuente y titulares completos: spec/oui-ieee.json y OUI.md.
// No confundir marca con chip. Foscam y Anyka: sin prefijo verificado.
// Generar y validar con tests/generar_oui.mjs; después revisar los cambios.
struct OuiEntry { uint32_t prefix; const char* vendor; Confidence confidence; };
static const OuiEntry OUI_TABLE[] = {
  {0x0C75D2, "Hikvision", ALTA},  // Hangzhou Hikvision Digital Technology Co.,Ltd.
  {0x548C81, "Hikvision", ALTA},  // Hangzhou Hikvision Digital Technology Co.,Ltd.
  {0x244845, "Hikvision", ALTA},  // Hangzhou Hikvision Digital Technology Co.,Ltd.
  {0xECC89C, "Hikvision", ALTA},  // Hangzhou Hikvision Digital Technology Co.,Ltd.
  {0x8CE748, "Hikvision", ALTA},  // Hangzhou Hikvision Digital Technology Co.,Ltd.
  {0x2428FD, "Hikvision", ALTA},  // Hangzhou Hikvision Digital Technology Co.,Ltd.
  {0xACB92F, "Hikvision", ALTA},  // Hangzhou Hikvision Digital Technology Co.,Ltd.
  {0xD4E853, "Hikvision", ALTA},  // Hangzhou Hikvision Digital Technology Co.,Ltd.
  {0x74C929, "Dahua", ALTA},  // Zhejiang Dahua Technology Co., Ltd.
  {0x6C1C71, "Dahua", ALTA},  // Zhejiang Dahua Technology Co., Ltd.
  {0x08EDED, "Dahua", ALTA},  // Zhejiang Dahua Technology Co., Ltd.
  {0xE02EFE, "Dahua", ALTA},  // Zhejiang Dahua Technology Co., Ltd.
  {0x407AA4, "Dahua", ALTA},  // ZHEJIANG DAHUA TECHNOLOGYCO.,LTD
  {0x5CF51A, "Dahua", ALTA},  // Zhejiang Dahua Technology Co., Ltd.
  {0x64F2FB, "EZVIZ", ALTA},  // Hangzhou Ezviz Software Co.,Ltd.
  {0x94EC13, "EZVIZ", ALTA},  // Hangzhou Ezviz Software Co.,Ltd.
  {0xEC97E0, "EZVIZ", ALTA},  // Hangzhou Ezviz Software Co.,Ltd.
  {0xF47018, "EZVIZ", ALTA},  // Hangzhou Ezviz Software Co.,Ltd.
  {0xFC2422, "EZVIZ", ALTA},  // Hangzhou Ezviz Software Co.,Ltd.
  {0xA83162, "Imou", ALTA},  // Hangzhou Huacheng Network Technology Co.,Ltd
  {0x302450, "Imou", ALTA},  // Hangzhou Huacheng Network Technology Co.,Ltd
  {0x906A94, "Imou", ALTA},  // hangzhou huacheng network technology co., ltd
  {0x00D98B, "Imou", ALTA},  // Hangzhou Huacheng Network Technology Co.,Ltd
  {0xAC3DFA, "Imou", ALTA},  // Hangzhou Huacheng Network Technology Co.,Ltd
  {0x1C4D89, "Imou", ALTA},  // Hangzhou Huacheng Network Technology Co.,Ltd
  {0x88263F, "Uniview", ALTA},  // Zhejiang Uniview Technologies Co.,Ltd.
  {0x14BA88, "Uniview", ALTA},  // Zhejiang Uniview Technologies Co.,Ltd.
  {0x6CF17E, "Uniview", ALTA},  // Zhejiang Uniview Technologies Co.,Ltd.
  {0x48EA63, "Uniview", ALTA},  // Zhejiang Uniview Technologies Co., Ltd.
  {0xC47905, "Uniview", ALTA},  // Zhejiang Uniview Technologies Co.,Ltd.
  {0xEC71DB, "Reolink", ALTA},  // Reolink Innovation Limited
  {0x00651E, "Amcrest", ALTA},  // Amcrest Technologies
  {0x9C8ECD, "Amcrest", ALTA},  // Amcrest Technologies
  {0xA06032, "Amcrest", ALTA},  // Amcrest Technologies
  {0xFC9C98, "Arlo", ALTA},  // Arlo Technology
  {0x486264, "Arlo", ALTA},  // Arlo Technology
  {0xA41162, "Arlo", ALTA},  // Arlo Technology
  {0xD03F27, "Wyze", ALTA},  // Wyze Labs Inc
  {0xF0C88B, "Wyze", ALTA},  // Wyze Labs Inc
  {0x80482C, "Wyze", ALTA},  // Wyze Labs Inc
  {0x7C78B2, "Wyze", ALTA},  // Wyze Labs Inc
  {0x2CAA8E, "Wyze", ALTA},  // Wyze Labs Inc
  {0x44B423, "Hanwha/Wisenet", ALTA},  // HANWHA VISION VIETNAM COMPANY LIMITED
  {0xE43022, "Hanwha/Wisenet", ALTA},  // Hanwha Vision VietNam
  {0xB8A44F, "Axis", ALTA},  // Axis Communications AB
  {0x00408C, "Axis", ALTA},  // Axis Communications AB
  {0xE82725, "Axis", ALTA},  // Axis Communications AB
  {0xACCC8E, "Axis", ALTA},  // Axis Communications AB
  {0x001F54, "Lorex", ALTA},  // Lorex Technology Inc.
  {0xBC51FE, "Swann", ALTA},  // Swann communications Pty Ltd
  {0x5870C6, "YI/Kami", ALTA},  // Shanghai Xiaoyi Technology Co., Ltd.
  {0x607EA4, "Xiaomi/Imilab", MEDIA},  // Shanghai Imilab Technology Co.Ltd
  {0xB4101C, "Xiaomi/Imilab", MEDIA},  // Shanghai Imilab Technology Co.Ltd
  {0x78DF72, "Xiaomi/Imilab", MEDIA},  // Shanghai Imilab Technology Co.Ltd
  {0x94F827, "Xiaomi/Imilab", MEDIA},  // Shanghai Imilab Technology Co.Ltd
  {0xB88880, "Xiaomi/Imilab", MEDIA},  // Shanghai Imilab Technology Co.Ltd
  {0xCCEB5E, "Xiaomi/Imilab", MEDIA},  // Xiaomi Communications Co Ltd
  {0xB8EA98, "Xiaomi/Imilab", MEDIA},  // Xiaomi Communications Co Ltd
  {0x8CD0B2, "Xiaomi/Imilab", MEDIA},  // Beijing Xiaomi Mobile Software Co., Ltd
  {0xF41A9C, "Xiaomi/Imilab", MEDIA},  // Xiaomi Communications Co Ltd
  {0xDC6AE7, "Xiaomi/Imilab", MEDIA},  // Xiaomi Communications Co Ltd
  {0x34F716, "TP-Link/Tapo", MEDIA},  // TP-LINK TECHNOLOGIES CO.,LTD.
  {0x54A703, "TP-Link/Tapo", MEDIA},  // TP-LINK TECHNOLOGIES CO.,LTD.
  {0xB0BE76, "TP-Link/Tapo", MEDIA},  // TP-LINK TECHNOLOGIES CO.,LTD.
  {0x34E894, "TP-Link/Tapo", MEDIA},  // TP-LINK TECHNOLOGIES CO.,LTD.
  {0x94D9B3, "TP-Link/Tapo", MEDIA},  // TP-LINK TECHNOLOGIES CO.,LTD.
  {0xB0958E, "TP-Link/Tapo", MEDIA},  // TP-LINK TECHNOLOGIES CO.,LTD.
  {0x54E019, "Amazon/Ring", MEDIA},  // Ring LLC
  {0xAC9FC3, "Amazon/Ring", MEDIA},  // Ring LLC
  {0xCC3BFB, "Amazon/Ring", MEDIA},  // Ring LLC
  {0x5C475E, "Amazon/Ring", MEDIA},  // Ring LLC
  {0x70AD43, "Amazon/Blink", MEDIA},  // Blink by Amazon 
  {0x741348, "Amazon/Blink", MEDIA},  // Blink by Amazon 
  {0xF074C1, "Amazon/Blink", MEDIA},  // Blink by Amazon 
  {0xC819D8, "Amazon/Blink", MEDIA},  // Blink by Amazon 
  {0x842859, "Amazon", MEDIA},  // Amazon Technologies Inc.
  {0x2873F6, "Amazon", MEDIA},  // Amazon Technologies Inc.
  {0xE0CB1D, "Amazon", MEDIA},  // Amazon Technologies Inc.
  {0xFCD749, "Amazon", MEDIA},  // Amazon Technologies Inc.
  {0x8C8580, "Anker/Eufy", MEDIA},  // Smart Innovation LLC
  {0x641666, "Google/Nest", MEDIA},  // Nest Labs Inc.
  {0x18B430, "Google/Nest", MEDIA},  // Nest Labs Inc.
  {0x60706C, "Google/Nest", MEDIA},  // Google, Inc.
  {0xC82ADD, "Google/Nest", MEDIA},  // Google, Inc.
  {0x242934, "Google/Nest", MEDIA},  // Google, Inc.
  {0x08B4B1, "Google/Nest", MEDIA},  // Google, Inc.
  {0x1C90FF, "Tuya", MEDIA},  // Tuya Smart Inc.
  {0xFC3CD7, "Tuya", MEDIA},  // Tuya Smart Inc.
  {0xE4AEE4, "Tuya", MEDIA},  // Tuya Smart Inc.
  {0xFC671F, "Tuya", MEDIA},  // Tuya Smart Inc.
  {0xCC8CBF, "Tuya", MEDIA},  // Tuya Smart Inc.
  {0xD48AFC, "Espressif", BAJA},  // Espressif Inc.
  {0xE465B8, "Espressif", BAJA},  // Espressif Inc.
  {0xB48A0A, "Espressif", BAJA},  // Espressif Inc.
  {0x94E686, "Espressif", BAJA},  // Espressif Inc.
  {0x34B472, "Espressif", BAJA},  // Espressif Inc.
  {0x34AB95, "Espressif", BAJA},  // Espressif Inc.
  {0x3C6105, "Espressif", BAJA},  // Espressif Inc.
  {0x807D3A, "Espressif", BAJA},  // Espressif Inc.
  {0x600194, "Espressif", BAJA},  // Espressif Inc.
  {0x9097D5, "Espressif", BAJA},  // Espressif Inc.
  {0xC8478C, "Beken", BAJA},  // Beken Corporation
  {0x70879E, "Beken", BAJA},  // Beken Corporation
  {0x806DDE, "Beken", BAJA},  // Beken Corporation
  {0x00E04C, "Realtek", BAJA},  // REALTEK SEMICONDUCTOR CORP.
  {0xFC934E, "Realtek", BAJA},  // REALTEK SEMICONDUCTOR CORP.
  {0xD03110, "Ingenic", BAJA},  // Ingenic Semiconductor Co.,Ltd
  {0x848A59, "Hisilicon", BAJA},  // Hisilicon Technologies Co., Ltd
  {0x000C43, "MediaTek", BAJA},  // MediaTek Inc
  {0x000CE7, "MediaTek", BAJA},  // MediaTek Inc
  {0x0017A5, "MediaTek", BAJA},  // MediaTek Inc
  {0x000A00, "MediaTek", BAJA},  // MediaTek Inc
  {0x4472AC, "AltoBeam", BAJA},  // AltoBeam Inc.
  {0x3854F5, "AltoBeam", BAJA},  // AltoBeam Inc.
  {0x48D890, "FN-Link", BAJA},  // FN-LINK TECHNOLOGY LIMITED
  {0xAC64CF, "FN-Link", BAJA},  // FN-LINK TECHNOLOGY LIMITED
  {0x24A160, "Espressif", BAJA},  // Espressif Inc.
  {0x246F28, "Espressif", BAJA},  // Espressif Inc.
  {0x30AEA4, "Espressif", BAJA},  // Espressif Inc.
  {0xA4CF12, "Espressif", BAJA},  // Espressif Inc.
  {0xAC67B2, "Espressif", BAJA},  // Espressif Inc.
  {0x7CDFA1, "Espressif", BAJA},  // Espressif Inc.
  {0xECFABC, "Espressif", BAJA},  // Espressif Inc.
  {0xDC4F22, "Espressif", BAJA},  // Espressif Inc.
  {0x847AB6, "AltoBeam", BAJA},  // AltoBeam Inc.
  {0x2CD26B, "FN-Link", BAJA},  // FN-LINK TECHNOLOGY LIMITED
  {0xC056E3, "Hikvision", ALTA},  // Hangzhou Hikvision Digital Technology Co.,Ltd.
  {0xBCAD28, "Hikvision", ALTA},  // Hangzhou Hikvision Digital Technology Co.,Ltd.
};
inline const OuiEntry* lookupOui(const uint8_t* mac) {
  if (mac[0] & 0x03) return nullptr;  // local o multidifusión: no atribuir fabricante
  uint32_t prefix = ((uint32_t)mac[0] << 16) | ((uint32_t)mac[1] << 8) | mac[2];
  for (const auto& e : OUI_TABLE) if (e.prefix == prefix) return &e;
  return nullptr;
}
