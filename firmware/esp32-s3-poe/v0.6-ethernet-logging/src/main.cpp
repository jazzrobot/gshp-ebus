#include <Arduino.h>
#include <ETH.h>
#include <SPI.h>
#include <WebServer.h>

extern "C" {
#include "driver/uart.h"
#include "esp_err.h"
#include "esp_idf_version.h"
#include "esp_timer.h"
}

#ifndef EBUS_UART_RX_PIN
#define EBUS_UART_RX_PIN 16
#endif

#ifndef EBUS_UART_PORT
#define EBUS_UART_PORT 1
#endif

#ifndef EBUS_UART_BAUD
#define EBUS_UART_BAUD 2400
#endif

#ifndef USB_LOG_BAUD
#define USB_LOG_BAUD 115200
#endif

#ifndef EBUS_IDLE_GAP_US
#define EBUS_IDLE_GAP_US 20000
#endif

#ifndef EBUS_SUMMARY_INTERVAL_MS
#define EBUS_SUMMARY_INTERVAL_MS 5000
#endif

#ifndef EBUS_DRIVER_RX_BUFFER
#define EBUS_DRIVER_RX_BUFFER 1024
#endif

#ifndef EBUS_FRAME_BUFFER_BYTES
#define EBUS_FRAME_BUFFER_BYTES 256
#endif

#ifndef EBUS_EVENT_QUEUE_LENGTH
#define EBUS_EVENT_QUEUE_LENGTH 32
#endif

#ifndef EBUS_LOG_RX_BYTES
#define EBUS_LOG_RX_BYTES 0
#endif

#ifndef EBUS_ETH_ENABLED
#define EBUS_ETH_ENABLED 1
#endif

#ifndef EBUS_ETH_HOSTNAME
#define EBUS_ETH_HOSTNAME "gshp-ebus"
#endif

#ifndef EBUS_ETH_HTTP_PORT
#define EBUS_ETH_HTTP_PORT 80
#endif

#ifndef EBUS_ETH_RECENT_LOG_LINES
#define EBUS_ETH_RECENT_LOG_LINES 96
#endif

#ifndef EBUS_ETH_PHY_TYPE
#define EBUS_ETH_PHY_TYPE ETH_PHY_W5500
#endif

#ifndef EBUS_ETH_PHY_ADDR
#define EBUS_ETH_PHY_ADDR 1
#endif

#ifndef EBUS_ETH_PHY_CS
#define EBUS_ETH_PHY_CS 14
#endif

#ifndef EBUS_ETH_PHY_IRQ
#define EBUS_ETH_PHY_IRQ 10
#endif

#ifndef EBUS_ETH_PHY_RST
#define EBUS_ETH_PHY_RST 9
#endif

#ifndef EBUS_ETH_SPI_SCK
#define EBUS_ETH_SPI_SCK 13
#endif

#ifndef EBUS_ETH_SPI_MISO
#define EBUS_ETH_SPI_MISO 12
#endif

#ifndef EBUS_ETH_SPI_MOSI
#define EBUS_ETH_SPI_MOSI 11
#endif

namespace {

constexpr uart_port_t kEbusUart = static_cast<uart_port_t>(EBUS_UART_PORT);
constexpr int kEbusRxPin = EBUS_UART_RX_PIN;
constexpr uint32_t kEbusBaud = EBUS_UART_BAUD;
constexpr uint32_t kUsbLogBaud = USB_LOG_BAUD;
constexpr uint32_t kIdleGapUs = EBUS_IDLE_GAP_US;
constexpr uint32_t kSummaryIntervalMs = EBUS_SUMMARY_INTERVAL_MS;
constexpr int kDriverRxBuffer = EBUS_DRIVER_RX_BUFFER;
constexpr size_t kFrameBufferBytes = EBUS_FRAME_BUFFER_BYTES;
constexpr int kEventQueueLength = EBUS_EVENT_QUEUE_LENGTH;
constexpr uint8_t kEbusSyncByte = 0xAA;
constexpr bool kLogRxBytes = EBUS_LOG_RX_BYTES != 0;
constexpr size_t kSignatureBytes = 8;
constexpr size_t kFrameFamilyCapacity = 24;
constexpr size_t kTrackedFamilyBytes = 32;

constexpr bool kEthernetEnabled = EBUS_ETH_ENABLED != 0;
constexpr uint16_t kHttpPort = EBUS_ETH_HTTP_PORT;
constexpr size_t kRecentLogCapacity = EBUS_ETH_RECENT_LOG_LINES;
constexpr const char *kEthHostname = EBUS_ETH_HOSTNAME;
constexpr eth_phy_type_t kEthPhyType = static_cast<eth_phy_type_t>(EBUS_ETH_PHY_TYPE);
constexpr int kEthPhyAddr = EBUS_ETH_PHY_ADDR;
constexpr int kEthPhyCs = EBUS_ETH_PHY_CS;
constexpr int kEthPhyIrq = EBUS_ETH_PHY_IRQ;
constexpr int kEthPhyRst = EBUS_ETH_PHY_RST;
constexpr int kEthSpiSck = EBUS_ETH_SPI_SCK;
constexpr int kEthSpiMiso = EBUS_ETH_SPI_MISO;
constexpr int kEthSpiMosi = EBUS_ETH_SPI_MOSI;

QueueHandle_t g_uartQueue = nullptr;
uint8_t g_frameBuffer[kFrameBufferBytes];
size_t g_frameLength = 0;
int64_t g_frameStartUs = 0;
int64_t g_lastByteUs = 0;
uint32_t g_lastSummaryMs = 0;

WebServer g_webServer(kHttpPort);
bool g_httpServerStarted = false;
bool g_ethConnected = false;
volatile int32_t g_pendingNetworkEvent = -1;

struct FrameFamily {
  bool inUse = false;
  uint8_t signature[kSignatureBytes] = {};
  size_t signatureLength = 0;
  uint8_t routeSrc = 0;
  uint8_t routeDst = 0;
  uint8_t routeType = 0;
  uint32_t seenCount = 0;
  size_t lastFrameLength = 0;
  int64_t lastSeenUs = 0;
  uint32_t lastGapUs = 0;
  uint64_t totalGapUs = 0;
  bool hasReferenceFrame = false;
  uint8_t referenceFrame[kTrackedFamilyBytes] = {};
  size_t referenceLength = 0;
  bool hasLastFrame = false;
  uint8_t lastFrame[kTrackedFamilyBytes] = {};
  size_t lastStoredLength = 0;
  uint32_t varyingMask = 0;
  bool varyingBeyondMask = false;
  bool varyingLength = false;
};

struct ListenerStats {
  uint64_t totalBytes = 0;
  uint64_t frameCount = 0;
  uint32_t syncOnlyFrames = 0;
  uint32_t suppressedRxByteLogs = 0;
  uint32_t frameTruncations = 0;
  uint32_t dataEvents = 0;
  uint32_t fifoOverflows = 0;
  uint32_t bufferFullEvents = 0;
  uint32_t frameErrors = 0;
  uint32_t parityErrors = 0;
  uint32_t breakEvents = 0;
  uint32_t unknownEvents = 0;
};

struct RecentLogEntry {
  uint64_t seq = 0;
  String line;
};

ListenerStats g_stats;
FrameFamily g_frameFamilies[kFrameFamilyCapacity];
RecentLogEntry g_recentLogs[kRecentLogCapacity];
size_t g_recentLogHead = 0;
size_t g_recentLogCount = 0;
uint64_t g_logSequence = 0;
String g_lastStatsLine;

void waitForUsbSerial() {
  const uint32_t startMs = millis();
  while (!Serial && (millis() - startMs) < 3000) {
    delay(10);
  }
}

void appendUnsigned(String &out, uint32_t value) {
  out += String(value);
}

void appendUnsigned64(String &out, uint64_t value) {
  char buffer[24];
  snprintf(buffer, sizeof(buffer), "%llu", static_cast<unsigned long long>(value));
  out += buffer;
}

void appendSigned64(String &out, int64_t value) {
  char buffer[24];
  snprintf(buffer, sizeof(buffer), "%lld", static_cast<long long>(value));
  out += buffer;
}

void appendHexByte(String &out, uint8_t value) {
  char buffer[4];
  snprintf(buffer, sizeof(buffer), "%02X", value);
  out += buffer;
}

void appendHexBytes(String &out, const uint8_t *buffer, size_t length) {
  for (size_t index = 0; index < length; ++index) {
    appendHexByte(out, buffer[index]);
    if (index + 1 < length) {
      out += ' ';
    }
  }
}

bool isSyncOnlyFrame(const uint8_t *buffer, size_t length) {
  if (length == 0) {
    return false;
  }

  for (size_t index = 0; index < length; ++index) {
    if (buffer[index] != kEbusSyncByte) {
      return false;
    }
  }

  return true;
}

size_t getSignatureLength(size_t frameLength) {
  return min(frameLength, kSignatureBytes);
}

void appendSignature(String &out, const uint8_t *buffer, size_t length) {
  appendHexBytes(out, buffer, getSignatureLength(length));
}

void appendRouteBytes(String &out, uint8_t src, uint8_t dst, uint8_t type) {
  appendHexByte(out, src);
  out += "->";
  appendHexByte(out, dst);
  out += " type=";
  appendHexByte(out, type);
}

void appendRouteForFrame(String &out, const uint8_t *buffer, size_t length) {
  if (length >= 4) {
    appendRouteBytes(out, buffer[1], buffer[2], buffer[3]);
  } else {
    out += "--";
  }
}

void appendIndexMask(String &out, uint32_t mask, bool beyondMask, bool varyingLength) {
  bool printedAny = false;

  for (size_t index = 0; index < kTrackedFamilyBytes; ++index) {
    if ((mask & (1UL << index)) == 0) {
      continue;
    }

    if (printedAny) {
      out += ',';
    }

    appendUnsigned(out, static_cast<uint32_t>(index));
    printedAny = true;
  }

  if (beyondMask) {
    if (printedAny) {
      out += ',';
    }
    out += "32+";
    printedAny = true;
  }

  if (varyingLength) {
    if (printedAny) {
      out += ',';
    }
    out += "len";
    printedAny = true;
  }

  if (!printedAny) {
    out += '-';
  }
}

void storeRecentLogLine(const String &line) {
  if (kRecentLogCapacity == 0 || line.isEmpty()) {
    return;
  }

  ++g_logSequence;
  g_recentLogs[g_recentLogHead].seq = g_logSequence;
  g_recentLogs[g_recentLogHead].line = line;
  g_recentLogHead = (g_recentLogHead + 1) % kRecentLogCapacity;
  if (g_recentLogCount < kRecentLogCapacity) {
    ++g_recentLogCount;
  }
}

void emitLine(const String &line, bool storeRecent = true) {
  Serial.println(line);
  if (storeRecent) {
    storeRecentLogLine(line);
  }
}

uint64_t parseUnsignedArg(const String &value) {
  if (value.isEmpty()) {
    return 0;
  }

  return strtoull(value.c_str(), nullptr, 10);
}

size_t countFrameFamilies() {
  size_t count = 0;
  for (const FrameFamily &family : g_frameFamilies) {
    if (family.inUse) {
      ++count;
    }
  }

  return count;
}

size_t findFrameFamily(const uint8_t *buffer, size_t length) {
  const size_t signatureLength = getSignatureLength(length);

  for (size_t index = 0; index < kFrameFamilyCapacity; ++index) {
    const FrameFamily &family = g_frameFamilies[index];
    if (!family.inUse || family.signatureLength != signatureLength) {
      continue;
    }

    bool matches = true;
    for (size_t sigIndex = 0; sigIndex < signatureLength; ++sigIndex) {
      if (family.signature[sigIndex] != buffer[sigIndex]) {
        matches = false;
        break;
      }
    }

    if (matches) {
      return index;
    }
  }

  return kFrameFamilyCapacity;
}

size_t createFrameFamily(const uint8_t *buffer, size_t length) {
  const size_t signatureLength = getSignatureLength(length);

  for (size_t index = 0; index < kFrameFamilyCapacity; ++index) {
    FrameFamily &family = g_frameFamilies[index];
    if (family.inUse) {
      continue;
    }

    family.inUse = true;
    family.signatureLength = signatureLength;
    for (size_t sigIndex = 0; sigIndex < signatureLength; ++sigIndex) {
      family.signature[sigIndex] = buffer[sigIndex];
    }
    if (length >= 4) {
      family.routeSrc = buffer[1];
      family.routeDst = buffer[2];
      family.routeType = buffer[3];
    }

    return index;
  }

  return kFrameFamilyCapacity;
}

size_t getOrCreateFrameFamily(const uint8_t *buffer, size_t length) {
  const size_t existingIndex = findFrameFamily(buffer, length);
  if (existingIndex < kFrameFamilyCapacity) {
    return existingIndex;
  }

  return createFrameFamily(buffer, length);
}

String buildFamilyLine(size_t index) {
  const FrameFamily &family = g_frameFamilies[index];
  String line;
  line.reserve(224);
  line += "[family] id=";
  appendUnsigned(line, static_cast<uint32_t>(index + 1));
  line += " route=";
  appendRouteBytes(line, family.routeSrc, family.routeDst, family.routeType);
  line += " sig=";
  appendHexBytes(line, family.signature, family.signatureLength);
  line += " seen=";
  appendUnsigned(line, family.seenCount);
  line += " last_len=";
  appendUnsigned(line, static_cast<uint32_t>(family.lastFrameLength));
  line += " last_gap_us=";
  appendUnsigned(line, family.lastGapUs);
  line += " avg_gap_us=";

  uint32_t avgGapUs = 0;
  if (family.seenCount > 1) {
    avgGapUs = static_cast<uint32_t>(
        family.totalGapUs / static_cast<uint64_t>(family.seenCount - 1));
  }

  appendUnsigned(line, avgGapUs);
  line += " vary_idx=";
  appendIndexMask(
      line, family.varyingMask, family.varyingBeyondMask, family.varyingLength);
  return line;
}

String buildStatsLine(uint32_t uptimeMs) {
  String line;
  line.reserve(224);
  line += "[stats] up_ms=";
  appendUnsigned(line, uptimeMs);
  line += " bytes=";
  appendUnsigned64(line, g_stats.totalBytes);
  line += " frames=";
  appendUnsigned64(line, g_stats.frameCount);
  line += " families=";
  appendUnsigned(line, static_cast<uint32_t>(countFrameFamilies()));
  line += " sync_only=";
  appendUnsigned(line, g_stats.syncOnlyFrames);
  line += " rx_supp=";
  appendUnsigned(line, g_stats.suppressedRxByteLogs);
  line += " trunc=";
  appendUnsigned(line, g_stats.frameTruncations);
  line += " frame_err=";
  appendUnsigned(line, g_stats.frameErrors);
  line += " parity_err=";
  appendUnsigned(line, g_stats.parityErrors);
  line += " fifo_ovf=";
  appendUnsigned(line, g_stats.fifoOverflows);
  line += " buffer_full=";
  appendUnsigned(line, g_stats.bufferFullEvents);
  line += " breaks=";
  appendUnsigned(line, g_stats.breakEvents);
  line += " unknown=";
  appendUnsigned(line, g_stats.unknownEvents);
  return line;
}

void updateFamilyVariation(FrameFamily &family, const uint8_t *buffer, size_t length) {
  if (!family.hasReferenceFrame) {
    family.referenceLength = min(length, kTrackedFamilyBytes);
    for (size_t index = 0; index < family.referenceLength; ++index) {
      family.referenceFrame[index] = buffer[index];
    }
    family.hasReferenceFrame = true;
    return;
  }

  if (length != family.referenceLength) {
    family.varyingLength = true;
  }

  const size_t compareLength = max(length, family.referenceLength);
  for (size_t index = 0; index < compareLength; ++index) {
    const uint8_t currentByte = index < length ? buffer[index] : 0;
    const uint8_t referenceByte =
        index < family.referenceLength ? family.referenceFrame[index] : 0;
    if (currentByte == referenceByte) {
      continue;
    }

    if (index < kTrackedFamilyBytes) {
      family.varyingMask |= (1UL << index);
    } else {
      family.varyingBeyondMask = true;
    }
  }
}

uint32_t buildLastChangeMask(
    const FrameFamily &family, const uint8_t *buffer, size_t length, bool *lengthChanged) {
  *lengthChanged = false;
  if (!family.hasLastFrame) {
    return 0;
  }

  if (length != family.lastStoredLength) {
    *lengthChanged = true;
  }

  const size_t compareLength = max(length, family.lastStoredLength);
  uint32_t mask = 0;
  for (size_t index = 0; index < compareLength && index < kTrackedFamilyBytes; ++index) {
    const uint8_t currentByte = index < length ? buffer[index] : 0;
    const uint8_t previousByte = index < family.lastStoredLength ? family.lastFrame[index] : 0;
    if (currentByte != previousByte) {
      mask |= (1UL << index);
    }
  }

  if (compareLength > kTrackedFamilyBytes) {
    *lengthChanged = true;
  }

  return mask;
}

void storeLastFrame(FrameFamily &family, const uint8_t *buffer, size_t length) {
  family.lastStoredLength = min(length, kTrackedFamilyBytes);
  for (size_t index = 0; index < family.lastStoredLength; ++index) {
    family.lastFrame[index] = buffer[index];
  }
  family.hasLastFrame = true;
}

String buildFrameLine(
    const char *reason,
    size_t familyIndex,
    uint32_t familySeenCount,
    uint32_t familyDeltaUs,
    uint32_t familyAvgGapUs,
    uint32_t lastChangeMask,
    bool lastLengthChanged,
    int64_t endUs) {
  String line;
  line.reserve(320);
  line += "[frame] reason=";
  line += reason;
  line += " family=";
  appendUnsigned(
      line,
      familyIndex < kFrameFamilyCapacity ? static_cast<uint32_t>(familyIndex + 1) : 0U);
  line += " seen=";
  appendUnsigned(line, familySeenCount);
  if (familyDeltaUs > 0) {
    line += " delta_us=";
    appendUnsigned(line, familyDeltaUs);
    line += " avg_gap_us=";
    appendUnsigned(line, familyAvgGapUs);
  }

  line += " route=";
  appendRouteForFrame(line, g_frameBuffer, g_frameLength);
  line += " start_us=";
  appendSigned64(line, g_frameStartUs);
  line += " end_us=";
  appendSigned64(line, endUs);
  line += " len=";
  appendUnsigned(line, static_cast<uint32_t>(g_frameLength));
  line += " sig=";
  appendSignature(line, g_frameBuffer, g_frameLength);
  line += " changed_idx=";
  appendIndexMask(line, lastChangeMask, false, lastLengthChanged);
  line += " data=";
  appendHexBytes(line, g_frameBuffer, g_frameLength);
  return line;
}

void flushFrame(const char *reason) {
  if (g_frameLength == 0) {
    return;
  }

  if (isSyncOnlyFrame(g_frameBuffer, g_frameLength)) {
    ++g_stats.syncOnlyFrames;
    g_frameLength = 0;
    g_frameStartUs = 0;
    return;
  }

  ++g_stats.frameCount;
  const int64_t endUs = g_lastByteUs;
  const size_t familyIndex = getOrCreateFrameFamily(g_frameBuffer, g_frameLength);
  uint32_t familySeenCount = 0;
  uint32_t familyDeltaUs = 0;
  uint32_t familyAvgGapUs = 0;
  uint32_t lastChangeMask = 0;
  bool lastLengthChanged = false;

  if (familyIndex < kFrameFamilyCapacity) {
    FrameFamily &family = g_frameFamilies[familyIndex];
    lastChangeMask = buildLastChangeMask(
        family, g_frameBuffer, g_frameLength, &lastLengthChanged);
    if (family.seenCount > 0 && family.lastSeenUs > 0) {
      familyDeltaUs = static_cast<uint32_t>(g_frameStartUs - family.lastSeenUs);
      family.lastGapUs = familyDeltaUs;
      family.totalGapUs += familyDeltaUs;
    }

    family.lastSeenUs = g_frameStartUs;
    family.lastFrameLength = g_frameLength;
    familySeenCount = ++family.seenCount;

    if (family.seenCount > 1) {
      familyAvgGapUs = static_cast<uint32_t>(
          family.totalGapUs / static_cast<uint64_t>(family.seenCount - 1));
    }

    updateFamilyVariation(family, g_frameBuffer, g_frameLength);
    storeLastFrame(family, g_frameBuffer, g_frameLength);
  }

  emitLine(buildFrameLine(
      reason,
      familyIndex,
      familySeenCount,
      familyDeltaUs,
      familyAvgGapUs,
      lastChangeMask,
      lastLengthChanged,
      endUs));

  g_frameLength = 0;
  g_frameStartUs = 0;
}

void appendByteToFrame(uint8_t value, int64_t timestampUs) {
  if (g_frameLength == 0) {
    g_frameStartUs = timestampUs;
  } else if ((timestampUs - g_lastByteUs) > static_cast<int64_t>(kIdleGapUs)) {
    flushFrame("gap_before_byte");
    g_frameStartUs = timestampUs;
  } else if (value == kEbusSyncByte && g_frameLength > 1) {
    flushFrame("sync_boundary");
    g_frameStartUs = timestampUs;
  }

  if (g_frameLength < kFrameBufferBytes) {
    g_frameBuffer[g_frameLength++] = value;
  } else {
    ++g_stats.frameTruncations;
  }

  g_lastByteUs = timestampUs;
}

void handleIncomingBytes(size_t byteCount) {
  g_stats.dataEvents++;

  while (byteCount > 0) {
    uint8_t buffer[64];
    const size_t chunkSize = min(byteCount, sizeof(buffer));
    const int readCount = uart_read_bytes(
        kEbusUart, buffer, chunkSize, pdMS_TO_TICKS(20));

    if (readCount <= 0) {
      return;
    }

    for (int index = 0; index < readCount; ++index) {
      const int64_t timestampUs = esp_timer_get_time();
      appendByteToFrame(buffer[index], timestampUs);
      g_stats.totalBytes++;

      const bool suppressSyncByteLog =
          (buffer[index] == kEbusSyncByte) &&
          (g_frameLength == 1) &&
          (g_frameBuffer[0] == kEbusSyncByte);

      if (!kLogRxBytes || suppressSyncByteLog) {
        ++g_stats.suppressedRxByteLogs;
      } else {
        String line;
        line.reserve(80);
        line += "[rx] t_us=";
        appendSigned64(line, timestampUs);
        line += " byte=0x";
        appendHexByte(line, buffer[index]);
        line += " total=";
        appendUnsigned64(line, g_stats.totalBytes);
        emitLine(line);
      }
    }

    byteCount -= static_cast<size_t>(readCount);
  }
}

void printFrameFamilySummaries() {
  for (size_t index = 0; index < kFrameFamilyCapacity; ++index) {
    if (!g_frameFamilies[index].inUse) {
      continue;
    }

    emitLine(buildFamilyLine(index));
  }
}

void printSummary() {
  const uint32_t uptimeMs = millis();
  g_lastStatsLine = buildStatsLine(uptimeMs);
  emitLine(g_lastStatsLine);
  printFrameFamilySummaries();
}

void logEvent(const char *label, uint32_t count) {
  String line;
  line.reserve(48);
  line += "[uart] event=";
  line += label;
  line += " count=";
  appendUnsigned(line, count);
  emitLine(line);
}

void installUartDriver() {
  uart_config_t config = {};
  config.baud_rate = static_cast<int>(kEbusBaud);
  config.data_bits = UART_DATA_8_BITS;
  config.parity = UART_PARITY_DISABLE;
  config.stop_bits = UART_STOP_BITS_1;
  config.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
  config.rx_flow_ctrl_thresh = 0;
#if ESP_IDF_VERSION_MAJOR >= 5
  config.source_clk = UART_SCLK_DEFAULT;
#endif

  esp_err_t err = uart_param_config(kEbusUart, &config);
  if (err != ESP_OK) {
    String line("[boot] uart_param_config failed: ");
    line += esp_err_to_name(err);
    emitLine(line);
    return;
  }

  err = uart_set_pin(
      kEbusUart,
      UART_PIN_NO_CHANGE,
      kEbusRxPin,
      UART_PIN_NO_CHANGE,
      UART_PIN_NO_CHANGE);
  if (err != ESP_OK) {
    String line("[boot] uart_set_pin failed: ");
    line += esp_err_to_name(err);
    emitLine(line);
    return;
  }

  err = uart_driver_install(
      kEbusUart, kDriverRxBuffer, 0, kEventQueueLength, &g_uartQueue, 0);
  if (err != ESP_OK) {
    String line("[boot] uart_driver_install failed: ");
    line += esp_err_to_name(err);
    emitLine(line);
    return;
  }

  uart_set_rx_full_threshold(kEbusUart, 1);
  uart_set_rx_timeout(kEbusUart, 2);

  String line;
  line.reserve(96);
  line += "[boot] listener armed on UART";
  appendUnsigned(line, static_cast<uint32_t>(kEbusUart));
  line += " RX=GPIO";
  appendUnsigned(line, static_cast<uint32_t>(kEbusRxPin));
  line += " baud=";
  appendUnsigned(line, kEbusBaud);
  line += " mode=2400-8N1 tx=unassigned";
  emitLine(line);
}

String buildRecentLogText(uint64_t sinceSeq = 0) {
  String out;
  out.reserve(g_recentLogCount * 112U + 128U);
  out += "recent_logs\n";
  out += "-----------\n";
  out += "latest_seq=";
  appendUnsigned64(out, g_logSequence);
  out += '\n';
  out += "since_seq=";
  appendUnsigned64(out, sinceSeq);
  out += '\n';

  if (g_recentLogCount == 0) {
    out += "(none)\n";
    return out;
  }

  const size_t firstIndex =
      (g_recentLogHead + kRecentLogCapacity - g_recentLogCount) % kRecentLogCapacity;
  for (size_t index = 0; index < g_recentLogCount; ++index) {
    const size_t slot = (firstIndex + index) % kRecentLogCapacity;
    const RecentLogEntry &entry = g_recentLogs[slot];
    if (entry.seq <= sinceSeq || entry.line.isEmpty()) {
      continue;
    }

    appendUnsigned64(out, entry.seq);
    out += '\t';
    out += entry.line;
    out += '\n';
  }

  return out;
}

String buildFamilySummaryText() {
  String out;
  out.reserve(2048);
  out += "families\n";
  out += "--------\n";

  bool printedAny = false;
  for (size_t index = 0; index < kFrameFamilyCapacity; ++index) {
    if (!g_frameFamilies[index].inUse) {
      continue;
    }

    out += buildFamilyLine(index);
    out += '\n';
    printedAny = true;
  }

  if (!printedAny) {
    out += "(none)\n";
  }

  return out;
}

String buildStatusText() {
  String out;
  out.reserve(4096);
  out += "gshp-ebus proto-v1 passive listener\n";
  out += "build=v0.6-ethernet-logging\n";
  out += "board=waveshare_esp32_s3_eth\n";
  out += "rx_pin=GPIO";
  appendUnsigned(out, static_cast<uint32_t>(kEbusRxPin));
  out += '\n';
  out += "eth_enabled=";
  out += kEthernetEnabled ? "yes" : "no";
  out += '\n';
  out += "eth_connected=";
  out += g_ethConnected ? "yes" : "no";
  out += '\n';
  out += "latest_seq=";
  appendUnsigned64(out, g_logSequence);
  out += '\n';
  if (kEthernetEnabled) {
    out += "hostname=";
    out += kEthHostname;
    out += '\n';
    out += "http_port=";
    appendUnsigned(out, kHttpPort);
    out += '\n';
    out += "ip=";
    out += ETH.localIP().toString();
    out += '\n';
    out += "mac=";
    out += ETH.macAddress();
    out += '\n';
    out += "link_speed_mbps=";
    appendUnsigned(out, ETH.linkSpeed());
    out += '\n';
    out += "duplex=";
    out += ETH.fullDuplex() ? "full" : "half";
    out += '\n';
  }

  out += '\n';
  out += "last_stats\n";
  out += "----------\n";
  if (g_lastStatsLine.isEmpty()) {
    out += "(none yet)\n";
  } else {
    out += g_lastStatsLine;
    out += '\n';
  }

  out += '\n';
  out += buildFamilySummaryText();
  out += '\n';
  out += buildRecentLogText();
  return out;
}

void handleHttpRoot() {
  g_webServer.send(200, "text/plain; charset=utf-8", buildStatusText());
}

void handleHttpStatus() {
  String out;
  out.reserve(768);
  out += "gshp-ebus status\n";
  out += "----------------\n";
  if (g_lastStatsLine.isEmpty()) {
    out += "(no stats yet)\n";
  } else {
    out += g_lastStatsLine;
    out += '\n';
  }
  out += "eth_connected=";
  out += g_ethConnected ? "yes" : "no";
  out += '\n';
  out += "latest_seq=";
  appendUnsigned64(out, g_logSequence);
  out += '\n';
  out += "ip=";
  out += ETH.localIP().toString();
  out += '\n';
  out += "mac=";
  out += ETH.macAddress();
  out += '\n';
  g_webServer.send(200, "text/plain; charset=utf-8", out);
}

void handleHttpFamilies() {
  g_webServer.send(200, "text/plain; charset=utf-8", buildFamilySummaryText());
}

void handleHttpLogs() {
  uint64_t sinceSeq = 0;
  if (g_webServer.hasArg("since")) {
    sinceSeq = parseUnsignedArg(g_webServer.arg("since"));
  }

  g_webServer.send(200, "text/plain; charset=utf-8", buildRecentLogText(sinceSeq));
}

void configureHttpServer() {
  if (!kEthernetEnabled || g_httpServerStarted) {
    return;
  }

  g_webServer.on("/", HTTP_GET, handleHttpRoot);
  g_webServer.on("/status", HTTP_GET, handleHttpStatus);
  g_webServer.on("/families", HTTP_GET, handleHttpFamilies);
  g_webServer.on("/logs", HTTP_GET, handleHttpLogs);
  g_webServer.begin();
  g_httpServerStarted = true;

  String line("[net] http server ready on port ");
  appendUnsigned(line, kHttpPort);
  line += " paths=/,/status,/families,/logs";
  emitLine(line);
}

void onNetworkEvent(arduino_event_id_t event, arduino_event_info_t info) {
  (void)info;

  if (!kEthernetEnabled) {
    return;
  }

  switch (event) {
    case ARDUINO_EVENT_ETH_START:
      ETH.setHostname(kEthHostname);
      break;

    case ARDUINO_EVENT_ETH_GOT_IP:
      g_ethConnected = true;
      break;

    case ARDUINO_EVENT_ETH_LOST_IP:
    case ARDUINO_EVENT_ETH_DISCONNECTED:
    case ARDUINO_EVENT_ETH_STOP:
      g_ethConnected = false;
      break;

    default:
      break;
  }

  g_pendingNetworkEvent = static_cast<int32_t>(event);
}

void servicePendingNetworkEvent() {
  if (!kEthernetEnabled) {
    return;
  }

  const int32_t pendingEvent = g_pendingNetworkEvent;
  if (pendingEvent < 0) {
    return;
  }

  g_pendingNetworkEvent = -1;
  String line("[net] ");

  switch (static_cast<arduino_event_id_t>(pendingEvent)) {
    case ARDUINO_EVENT_ETH_START:
      line += "ethernet started";
      break;

    case ARDUINO_EVENT_ETH_CONNECTED:
      line += "link up";
      break;

    case ARDUINO_EVENT_ETH_GOT_IP:
      line += "got_ip ip=";
      line += ETH.localIP().toString();
      line += " mac=";
      line += ETH.macAddress();
      line += " speed_mbps=";
      appendUnsigned(line, ETH.linkSpeed());
      line += " duplex=";
      line += ETH.fullDuplex() ? "full" : "half";
      break;

    case ARDUINO_EVENT_ETH_LOST_IP:
      line += "lost_ip";
      break;

    case ARDUINO_EVENT_ETH_DISCONNECTED:
      line += "link down";
      break;

    case ARDUINO_EVENT_ETH_STOP:
      line += "ethernet stopped";
      break;

    default:
      line += "event=";
      appendUnsigned(line, static_cast<uint32_t>(pendingEvent));
      break;
  }

  emitLine(line);
}

void startEthernet() {
  if (!kEthernetEnabled) {
    emitLine("[net] ethernet disabled at compile time");
    return;
  }

  Network.onEvent(onNetworkEvent);
  SPI.begin(kEthSpiSck, kEthSpiMiso, kEthSpiMosi);

  String line;
  line.reserve(96);
  line += "[net] initialising W5500 cs=";
  appendUnsigned(line, static_cast<uint32_t>(kEthPhyCs));
  line += " irq=";
  appendUnsigned(line, static_cast<uint32_t>(kEthPhyIrq));
  line += " rst=";
  appendUnsigned(line, static_cast<uint32_t>(kEthPhyRst));
  line += " sck=";
  appendUnsigned(line, static_cast<uint32_t>(kEthSpiSck));
  line += " miso=";
  appendUnsigned(line, static_cast<uint32_t>(kEthSpiMiso));
  line += " mosi=";
  appendUnsigned(line, static_cast<uint32_t>(kEthSpiMosi));
  emitLine(line);

  if (!ETH.begin(kEthPhyType, kEthPhyAddr, kEthPhyCs, kEthPhyIrq, kEthPhyRst, SPI)) {
    emitLine("[net] ETH.begin failed");
    return;
  }

  configureHttpServer();
}

}  // namespace

void setup() {
  Serial.begin(kUsbLogBaud);
  waitForUsbSerial();
  delay(250);

  emitLine("");
  emitLine("gshp-ebus proto-v1 passive listener");

  String boardLine;
  boardLine.reserve(72);
  boardLine += "[boot] board=waveshare_esp32_s3_eth rx_pin=GPIO";
  appendUnsigned(boardLine, static_cast<uint32_t>(kEbusRxPin));
  boardLine += " usb_baud=";
  appendUnsigned(boardLine, kUsbLogBaud);
  emitLine(boardLine);
  emitLine("[boot] passive mode only; no TX pin configured");

  installUartDriver();
  startEthernet();
  g_lastSummaryMs = millis();
}

void loop() {
  servicePendingNetworkEvent();

  if (g_httpServerStarted) {
    g_webServer.handleClient();
  }

  uart_event_t event = {};
  const bool haveEvent =
      (g_uartQueue != nullptr) &&
      (xQueueReceive(g_uartQueue, &event, pdMS_TO_TICKS(10)) == pdTRUE);

  if (haveEvent) {
    switch (event.type) {
      case UART_DATA:
        handleIncomingBytes(event.size);
        break;

      case UART_FIFO_OVF:
        g_stats.fifoOverflows++;
        logEvent("fifo_overflow", g_stats.fifoOverflows);
        flushFrame("fifo_overflow");
        uart_flush_input(kEbusUart);
        xQueueReset(g_uartQueue);
        break;

      case UART_BUFFER_FULL:
        g_stats.bufferFullEvents++;
        logEvent("buffer_full", g_stats.bufferFullEvents);
        flushFrame("buffer_full");
        uart_flush_input(kEbusUart);
        xQueueReset(g_uartQueue);
        break;

      case UART_BREAK:
        g_stats.breakEvents++;
        logEvent("break", g_stats.breakEvents);
        break;

      case UART_PARITY_ERR:
        g_stats.parityErrors++;
        logEvent("parity_error", g_stats.parityErrors);
        break;

      case UART_FRAME_ERR:
        g_stats.frameErrors++;
        logEvent("frame_error", g_stats.frameErrors);
        break;

      default:
        g_stats.unknownEvents++;
        logEvent("unknown", g_stats.unknownEvents);
        break;
    }
  }

  const int64_t nowUs = esp_timer_get_time();
  if (g_frameLength > 0 &&
      (nowUs - g_lastByteUs) > static_cast<int64_t>(kIdleGapUs)) {
    flushFrame("idle_timeout");
  }

  const uint32_t nowMs = millis();
  if ((nowMs - g_lastSummaryMs) >= kSummaryIntervalMs) {
    printSummary();
    g_lastSummaryMs = nowMs;
  }
}
