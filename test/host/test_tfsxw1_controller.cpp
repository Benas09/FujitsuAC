/*
  FujitsuAC - ESP32 libary for controlling FujitsuAC through MQTT
  Copyright (c) 2025 Benas Ragauskas. All rights reserved.

  Project home: https://github.com/Benas09/FujitsuAC
*/

// Host-native tests for the FujitsuAC::TFSXW1Controller init handshake - the
// Init1/Init2 reply validation in src/TFSXW1Controller.cpp. Drives the
// controller through a fake UART and a host clock, and observes it through the
// debug callback ("status" messages), the same channel the firmware publishes
// over MQTT. See test/README.md for how to build and run.

#include "Arduino.h"

#include <cstdint>
#include <cstdio>
#include <string>
#include <utility>
#include <vector>

#include "TFSXW1Controller.h"

using FujitsuAC::TFSXW1Controller;

// --- host clock -------------------------------------------------------------
static unsigned long g_now = 0;
unsigned long millis() { return g_now; }

// --- fake UART -------------------------------------------------------------
class FakeStream : public Stream {
    public:
        void feed(const std::vector<uint8_t> &bytes) {
            queue_.insert(queue_.end(), bytes.begin(), bytes.end());
        }

        int available() override { return static_cast<int>(queue_.size() - pos_); }
        int read() override { return pos_ < queue_.size() ? queue_[pos_++] : -1; }

        size_t write(const uint8_t *data, size_t size) override {
            sent.emplace_back(data, data + size);
            return size;
        }

        std::vector<std::vector<uint8_t>> sent;

    private:
        std::vector<uint8_t> queue_;
        std::size_t pos_ = 0;
};

// --- tiny assert harness --------------------------------------------------
static int g_checks = 0;
static int g_failed = 0;

#define CHECK(cond)                                                            \
    do {                                                                      \
        ++g_checks;                                                           \
        if (!(cond)) {                                                        \
            ++g_failed;                                                       \
            std::printf("  FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond);     \
        }                                                                     \
    } while (0)

// --- helpers -------------------------------------------------------------
// Every frame on this bus ends in a 16-bit checksum: 0xFFFF minus every
// preceding byte, big-endian. The replies below all carry a valid one, so they
// reach TFSXW1Controller::onFrame rather than being dropped by Buffer.
static std::vector<uint8_t> withChecksum(std::vector<uint8_t> frame) {
    uint16_t checksum = 0xFFFF;
    for (uint8_t b : frame) {
        checksum -= b;
    }
    frame.push_back(static_cast<uint8_t>(checksum >> 8));
    frame.push_back(static_cast<uint8_t>(checksum & 0xFF));
    return frame;
}

// The replies the controller accepts, straight from TFSXW1Controller::onFrame.
static const std::vector<uint8_t> kInit1Reply = {
    0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0xFF, 0xFD};
static const std::vector<uint8_t> kInit2Reply = {
    0x01, 0x00, 0x00, 0x00, 0x01, 0x01, 0xFF, 0xFC};

// A controller wired to a fake UART, recording every "status" debug message.
struct Harness {
    FakeStream uart;
    TFSXW1Controller controller{uart};
    std::vector<std::string> statuses;

    Harness() {
        g_now = 0;
        controller.setDebugCallback([this](const char *name, const char *message) {
            if (std::string(name) == "status") {
                statuses.push_back(message);
            }
        });
        controller.setup();
    }

    // Advance past the 400 ms request interval and let the controller send its
    // next request.
    void tick() {
        g_now += 400;
        controller.loop();
    }

    // Deliver a reply 20 ms later (a fresh frame for Buffer) and process it.
    void reply(const std::vector<uint8_t> &frame) {
        g_now += 20;
        uart.feed(frame);
        controller.loop();
    }

    bool saw(const char *status) const {
        for (const auto &s : statuses) {
            if (s == status) {
                return true;
            }
        }
        return false;
    }
};

// --- tests -------------------------------------------------------------
static void acceptsTheExpectedInit1Reply() {
    std::printf("acceptsTheExpectedInit1Reply\n");
    Harness h;

    h.tick();
    CHECK(h.saw("Init1 Send"));

    h.reply(kInit1Reply);
    h.tick();

    CHECK(!h.saw("Terminated Init1"));
    CHECK(h.saw("Init2 Send"));
}

static void acceptsTheExpectedInit2Reply() {
    std::printf("acceptsTheExpectedInit2Reply\n");
    Harness h;

    h.tick();
    h.reply(kInit1Reply);
    h.tick();
    h.reply(kInit2Reply);

    CHECK(!h.saw("Terminated Init2"));
    CHECK(h.saw("Running"));
}

// memcmp's sign only orders the buffers; a mismatch that sorts before the
// expected reply returns < 0 and must be rejected just like one that sorts
// after it. Status byte 0x00 instead of 0x01 is such a reply.
static void terminatesOnAnInit1ReplyThatSortsBeforeTheExpectedOne() {
    std::printf("terminatesOnAnInit1ReplyThatSortsBeforeTheExpectedOne\n");
    Harness h;

    h.tick();
    h.reply(withChecksum({0x00, 0x00, 0x00, 0x00, 0x01, 0x00}));
    h.tick();

    CHECK(h.saw("Terminated Init1"));
    CHECK(!h.saw("Init2 Send"));
}

static void terminatesOnAnInit1ReplyThatSortsAfterTheExpectedOne() {
    std::printf("terminatesOnAnInit1ReplyThatSortsAfterTheExpectedOne\n");
    Harness h;

    h.tick();
    h.reply(withChecksum({0x00, 0x00, 0x00, 0x00, 0x01, 0x02}));
    h.tick();

    CHECK(h.saw("Terminated Init1"));
    CHECK(!h.saw("Init2 Send"));
}

static void terminatesOnAnInit2ReplyThatSortsBeforeTheExpectedOne() {
    std::printf("terminatesOnAnInit2ReplyThatSortsBeforeTheExpectedOne\n");
    Harness h;

    h.tick();
    h.reply(kInit1Reply);
    h.tick();
    h.reply(withChecksum({0x01, 0x00, 0x00, 0x00, 0x01, 0x00}));

    CHECK(h.saw("Terminated Init2"));
    CHECK(!h.saw("Running"));
}

static void terminatesOnAnInit2ReplyThatSortsAfterTheExpectedOne() {
    std::printf("terminatesOnAnInit2ReplyThatSortsAfterTheExpectedOne\n");
    Harness h;

    h.tick();
    h.reply(kInit1Reply);
    h.tick();
    h.reply(withChecksum({0x01, 0x00, 0x00, 0x00, 0x01, 0x02}));

    CHECK(h.saw("Terminated Init2"));
    CHECK(!h.saw("Running"));
}

// After a restart the unit answers Init1 with one of two known frames first;
// the controller must keep waiting for the real reply rather than terminate.
static void waitsThroughTheRestartReplyToInit1() {
    std::printf("waitsThroughTheRestartReplyToInit1\n");
    Harness h;

    h.tick();
    h.reply({0xFE, 0x00, 0x00, 0x00, 0x01, 0x02, 0xFE, 0xFE});
    h.reply(kInit1Reply);
    h.tick();

    CHECK(!h.saw("Terminated Init1"));
    CHECK(h.saw("Init2 Send"));
}

// --- command queue ----------------------------------------------------------
// Replies the unit gives to a registry read (0x03) and to a registry write
// (0x02): status 0x01, no registries.
static const std::vector<uint8_t> kReadReply = withChecksum({0x03, 0x00, 0x00, 0x00, 0x01, 0x01});
static const std::vector<uint8_t> kWriteReply = withChecksum({0x02, 0x00, 0x00, 0x00, 0x01, 0x01});

// Runs the handshake and the three initial reads, so the next request the
// controller makes is FrameA.
static void startRunning(Harness &h) {
    h.tick();
    h.reply(kInit1Reply);
    h.tick();
    h.reply(kInit2Reply);

    for (int i = 0; i < 3; ++i) {
        h.tick();
        h.reply(kReadReply);
    }
}

// Lets the controller run through `cycles` request slots, answering each one,
// and returns every registry write it sent as (address, value) pairs.
static std::vector<std::pair<uint16_t, uint16_t>> runAndCollectWrites(Harness &h, int cycles) {
    std::vector<std::pair<uint16_t, uint16_t>> writes;

    for (int i = 0; i < cycles; ++i) {
        std::size_t before = h.uart.sent.size();
        h.tick();

        if (h.uart.sent.size() == before) {
            continue;
        }

        const std::vector<uint8_t> &frame = h.uart.sent.back();

        if (frame[0] == 0x02) {
            // Write frame: 02 00 00 00 <4 * n> then n (address, value) pairs.
            for (std::size_t j = 5; j + 4 <= 5u + frame[4]; j += 4) {
                writes.emplace_back(
                    static_cast<uint16_t>((frame[j] << 8) | frame[j + 1]),
                    static_cast<uint16_t>((frame[j + 2] << 8) | frame[j + 3]));
            }
            h.reply(kWriteReply);
        } else {
            h.reply(kReadReply);
        }
    }

    return writes;
}

static bool wrote(const std::vector<std::pair<uint16_t, uint16_t>> &writes, uint16_t address, uint16_t value) {
    for (const auto &w : writes) {
        if (w.first == address && w.second == value) {
            return true;
        }
    }
    return false;
}

static const uint16_t kModeAddress = 0x1001;
static const uint16_t kSetpointAddress = 0x1002;
static const uint16_t kFanSpeedAddress = 0x1003;

// Home Assistant's climate.set_temperature with an hvac_mode sends the mode and
// the setpoint a few milliseconds apart. Both must reach the unit
// (https://github.com/Benas09/FujitsuAC/issues/67).
static void sendsCommandsGivenBeforeThePreviousOneWasWritten() {
    std::printf("sendsCommandsGivenBeforeThePreviousOneWasWritten\n");
    Harness h;
    startRunning(h);

    h.controller.setMode(FujitsuAC::TFSXW1Enums::Mode::Heat);
    h.controller.setTemp("21.5");

    auto writes = runAndCollectWrites(h, 20);

    CHECK(wrote(writes, kModeAddress, 0x0004));
    CHECK(wrote(writes, kSetpointAddress, 215));
    CHECK(writes.size() == 2);
    CHECK(writes.size() == 2 && writes[0].first == kModeAddress);
}

// A command given while the previous write is waiting for its read-back must
// not be lost either.
static void sendsACommandGivenRightAfterAWrite() {
    std::printf("sendsACommandGivenRightAfterAWrite\n");
    Harness h;
    startRunning(h);

    h.controller.setMode(FujitsuAC::TFSXW1Enums::Mode::Heat);
    h.tick();                 // the mode write goes out
    h.reply(kWriteReply);
    h.controller.setFanSpeed(FujitsuAC::TFSXW1Enums::FanSpeed::Quiet);

    auto writes = runAndCollectWrites(h, 20);

    CHECK(wrote(writes, kFanSpeedAddress, 0x0002));
}

// Repeated commands for the same registry collapse into the latest one.
static void keepsOnlyTheLatestQueuedValueForARegistry() {
    std::printf("keepsOnlyTheLatestQueuedValueForARegistry\n");
    Harness h;
    startRunning(h);

    h.controller.setTemp("20");
    h.controller.setTemp("21");
    h.controller.setTemp("22");

    auto writes = runAndCollectWrites(h, 20);

    CHECK(writes.size() == 1);
    CHECK(wrote(writes, kSetpointAddress, 220));
}

int main() {
    acceptsTheExpectedInit1Reply();
    acceptsTheExpectedInit2Reply();
    terminatesOnAnInit1ReplyThatSortsBeforeTheExpectedOne();
    terminatesOnAnInit1ReplyThatSortsAfterTheExpectedOne();
    terminatesOnAnInit2ReplyThatSortsBeforeTheExpectedOne();
    terminatesOnAnInit2ReplyThatSortsAfterTheExpectedOne();
    waitsThroughTheRestartReplyToInit1();
    sendsCommandsGivenBeforeThePreviousOneWasWritten();
    sendsACommandGivenRightAfterAWrite();
    keepsOnlyTheLatestQueuedValueForARegistry();

    std::printf("\n%d checks, %d failed\n", g_checks, g_failed);
    return g_failed == 0 ? 0 : 1;
}
