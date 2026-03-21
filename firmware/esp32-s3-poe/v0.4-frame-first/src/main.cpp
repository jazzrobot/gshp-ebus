#include <Arduino.h>

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

QueueHandle_t g_uartQueue = nullptr;
uint8_t g_frameBuffer[kFrameBufferBytes];
size_t g_frameLength = 0;
int64_t g_frameStartUs = 0;
int64_t g_lastByteUs = 0;
uint32_t g_lastSummaryMs = 0;

struct FrameFamily {
  bool inUse = false;
  uint8_t signature[kSignatureBytes] = {};
  size_t signatureLength = 0;
  uint32_t seenCount = 0;
  size_t lastFrameLength = 0;
  int64_t lastSeenUs = 0;
  uint32_t lastGapUs = 0;
  uint64_t totalGapUs = 0;
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

ListenerStats g_stats;
FrameFamily g_frameFamilies[kFrameFamilyCapacity];

void waitForUsbSerial() {
  const uint32_t startMs = millis();
  while (!Serial && (millis() - startMs) < 3000) {
    delay(10);
  }
}

void printHexBytes(const uint8_t *buffer, size_t length) {
  for (size_t i = 0; i < length; ++i) {
    Serial.printf("%02X", buffer[i]);
    if (i + 1 < length) {
      Serial.print(' ');
    }
  }
}

size_t getSignatureLength(size_t frameLength) {
  return min(frameLength, kSignatureBytes);
}

void printSignature(const uint8_t *buffer, size_t length) {
  const size_t signatureLength = getSignatureLength(length);
  printHexBytes(buffer, signatureLength);
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

void printFrameFamilySummaries() {
  for (size_t index = 0; index < kFrameFamilyCapacity; ++index) {
    const FrameFamily &family = g_frameFamilies[index];
    if (!family.inUse) {
      continue;
    }

    uint32_t avgGapUs = 0;
    if (family.seenCount > 1) {
      avgGapUs = static_cast<uint32_t>(
          family.totalGapUs / static_cast<uint64_t>(family.seenCount - 1));
    }

    Serial.printf("[family] id=%u sig=", static_cast<unsigned>(index + 1));
    printSignature(family.signature, family.signatureLength);
    Serial.printf(
        " seen=%u last_len=%u last_gap_us=%u avg_gap_us=%u\n",
        family.seenCount,
        static_cast<unsigned>(family.lastFrameLength),
        family.lastGapUs,
        avgGapUs);
  }
}

void flushFrame(const char *reason) {
  if (g_frameLength == 0) {
    return;
  }

  if (g_frameLength == 1 && g_frameBuffer[0] == kEbusSyncByte) {
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

  if (familyIndex < kFrameFamilyCapacity) {
    FrameFamily &family = g_frameFamilies[familyIndex];
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
  }

  Serial.printf(
      "[frame] reason=%s family=%u seen=%u",
      reason,
      familyIndex < kFrameFamilyCapacity ? static_cast<unsigned>(familyIndex + 1) : 0U,
      familySeenCount);
  if (familyDeltaUs > 0) {
    Serial.printf(" delta_us=%u avg_gap_us=%u", familyDeltaUs, familyAvgGapUs);
  }

  Serial.printf(
      " start_us=%llu end_us=%llu len=%u sig=",
      static_cast<unsigned long long>(g_frameStartUs),
      static_cast<unsigned long long>(endUs),
      static_cast<unsigned>(g_frameLength));
  printSignature(g_frameBuffer, g_frameLength);
  Serial.print(" data=");
  printHexBytes(g_frameBuffer, g_frameLength);
  Serial.println();

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
        Serial.printf(
            "[rx] t_us=%llu byte=0x%02X total=%llu\n",
            static_cast<unsigned long long>(timestampUs),
            buffer[index],
            static_cast<unsigned long long>(g_stats.totalBytes));
      }
    }

    byteCount -= static_cast<size_t>(readCount);
  }
}

void printSummary() {
  const uint32_t uptimeMs = millis();

  Serial.printf(
      "[stats] up_ms=%lu bytes=%llu frames=%llu families=%u sync_only=%u rx_supp=%u trunc=%u frame_err=%u parity_err=%u fifo_ovf=%u buffer_full=%u breaks=%u unknown=%u\n",
      static_cast<unsigned long>(uptimeMs),
      static_cast<unsigned long long>(g_stats.totalBytes),
      static_cast<unsigned long long>(g_stats.frameCount),
      static_cast<unsigned>(countFrameFamilies()),
      g_stats.syncOnlyFrames,
      g_stats.suppressedRxByteLogs,
      g_stats.frameTruncations,
      g_stats.frameErrors,
      g_stats.parityErrors,
      g_stats.fifoOverflows,
      g_stats.bufferFullEvents,
      g_stats.breakEvents,
      g_stats.unknownEvents);

  printFrameFamilySummaries();
}

void logEvent(const char *label, uint32_t count) {
  Serial.printf("[uart] event=%s count=%u\n", label, count);
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
    Serial.printf("[boot] uart_param_config failed: %s\n", esp_err_to_name(err));
    return;
  }

  err = uart_set_pin(
      kEbusUart,
      UART_PIN_NO_CHANGE,
      kEbusRxPin,
      UART_PIN_NO_CHANGE,
      UART_PIN_NO_CHANGE);
  if (err != ESP_OK) {
    Serial.printf("[boot] uart_set_pin failed: %s\n", esp_err_to_name(err));
    return;
  }

  err = uart_driver_install(
      kEbusUart, kDriverRxBuffer, 0, kEventQueueLength, &g_uartQueue, 0);
  if (err != ESP_OK) {
    Serial.printf("[boot] uart_driver_install failed: %s\n", esp_err_to_name(err));
    return;
  }

  uart_set_rx_full_threshold(kEbusUart, 1);
  uart_set_rx_timeout(kEbusUart, 2);

  Serial.printf(
      "[boot] listener armed on UART%d RX=GPIO%d baud=%lu mode=2400-8N1 tx=unassigned\n",
      static_cast<int>(kEbusUart),
      kEbusRxPin,
      static_cast<unsigned long>(kEbusBaud));
}

}  // namespace

void setup() {
  Serial.begin(kUsbLogBaud);
  waitForUsbSerial();
  delay(250);

  Serial.println();
  Serial.println("gshp-ebus proto-v1 passive listener");
  Serial.printf(
      "[boot] board=waveshare_esp32_s3_eth rx_pin=GPIO%d usb_baud=%lu\n",
      kEbusRxPin,
      static_cast<unsigned long>(kUsbLogBaud));
  Serial.println("[boot] passive mode only; no TX pin configured");

  installUartDriver();
  g_lastSummaryMs = millis();
}

void loop() {
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
