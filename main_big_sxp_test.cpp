#include <sxp/project/ProjectDocument.h>
#include <sxp/project/ProjectFile.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "sxp/project/ProjectGenerator.h"

namespace {
  constexpr double Epsilon = 0.000001;

  struct OscillatorPayloadV1 {
    std::uint32_t waveShape = 0;
    std::int32_t octave = 0;
    std::int32_t semitone = 0;

    float attackSeconds = 0.005f;
    float decaySeconds = 0.25f;
    float sustainLevel = 1.0f;
    float releaseSeconds = 0.005f;

    std::uint64_t mixerChannelId = 1;
  };

  struct SamplerPayloadV1 {
    std::string samplePath;

    std::int32_t rootNote = 60;
    bool pitchFollow = true;
    bool oneShot = true;

    std::uint64_t mixerChannelId = 1;
  };

  void Fail(const std::string& message) {
    std::cerr << "FAILED: " << message << "\n";
    std::exit(1);
  }

  void Require(const sxp::Error& error, const std::string& action) {
    if (error.Ok()) {
      return;
    }

    std::cerr
      << "FAILED: " << action << "\n"
      << "  " << error.message << "\n";

    std::exit(1);
  }

  template<typename T>
  void CheckEqual(
    const T& actual,
    const T& expected,
    const std::string& name
  ) {
    if (actual == expected) {
      return;
    }

    std::cerr
      << "FAILED: " << name << "\n"
      << "  expected: " << expected << "\n"
      << "  actual:   " << actual << "\n";

    std::exit(1);
  }

  void CheckNear(
    double actual,
    double expected,
    const std::string& name
  ) {
    if (std::abs(actual - expected) <= Epsilon) {
      return;
    }

    std::cerr
      << "FAILED: " << name << "\n"
      << "  expected: " << expected << "\n"
      << "  actual:   " << actual << "\n";

    std::exit(1);
  }

  void CheckBytesEqual(
    const std::vector<std::uint8_t>& actual,
    const std::vector<std::uint8_t>& expected,
    const std::string& name
  ) {
    if (actual == expected) {
      return;
    }

    std::cerr
      << "FAILED: " << name << "\n"
      << "  expected size: " << expected.size() << "\n"
      << "  actual size:   " << actual.size() << "\n";

    const std::size_t count = std::min(actual.size(), expected.size());

    for (std::size_t i = 0; i < count; ++i) {
      if (actual[i] != expected[i]) {
        std::cerr
          << "  first mismatch at byte "
          << i
          << ": expected "
          << static_cast<int>(expected[i])
          << ", actual "
          << static_cast<int>(actual[i])
          << "\n";

        break;
      }
    }

    std::exit(1);
  }

  void AppendU8(
    std::vector<std::uint8_t>& bytes,
    std::uint8_t value
  ) {
    bytes.push_back(value);
  }

  void AppendBool(
    std::vector<std::uint8_t>& bytes,
    bool value
  ) {
    AppendU8(bytes, value ? 1 : 0);
  }

  void AppendU32(
    std::vector<std::uint8_t>& bytes,
    std::uint32_t value
  ) {
    bytes.push_back(static_cast<std::uint8_t>(value & 0xFF));
    bytes.push_back(static_cast<std::uint8_t>((value >> 8) & 0xFF));
    bytes.push_back(static_cast<std::uint8_t>((value >> 16) & 0xFF));
    bytes.push_back(static_cast<std::uint8_t>((value >> 24) & 0xFF));
  }

  void AppendI32(
    std::vector<std::uint8_t>& bytes,
    std::int32_t value
  ) {
    AppendU32(bytes, static_cast<std::uint32_t>(value));
  }

  void AppendU64(
    std::vector<std::uint8_t>& bytes,
    std::uint64_t value
  ) {
    for (int i = 0; i < 8; ++i) {
      bytes.push_back(
        static_cast<std::uint8_t>((value >> (i * 8)) & 0xFF)
      );
    }
  }

  void AppendF32(
    std::vector<std::uint8_t>& bytes,
    float value
  ) {
    std::uint32_t raw = 0;
    std::memcpy(&raw, &value, sizeof(raw));

    AppendU32(bytes, raw);
  }

  void AppendString(
    std::vector<std::uint8_t>& bytes,
    const std::string& value
  ) {
    AppendU64(bytes, static_cast<std::uint64_t>(value.size()));

    bytes.insert(
      bytes.end(),
      value.begin(),
      value.end()
    );
  }

  std::vector<std::uint8_t> EncodeOscillatorPayloadV1(
    const OscillatorPayloadV1& payload
  ) {
    std::vector<std::uint8_t> bytes;

    AppendU32(bytes, payload.waveShape);
    AppendI32(bytes, payload.octave);
    AppendI32(bytes, payload.semitone);

    AppendF32(bytes, payload.attackSeconds);
    AppendF32(bytes, payload.decaySeconds);
    AppendF32(bytes, payload.sustainLevel);
    AppendF32(bytes, payload.releaseSeconds);

    AppendU64(bytes, payload.mixerChannelId);

    return bytes;
  }

  std::vector<std::uint8_t> EncodeSamplerPayloadV1(
    const SamplerPayloadV1& payload
  ) {
    std::vector<std::uint8_t> bytes;

    AppendString(bytes, payload.samplePath);
    AppendI32(bytes, payload.rootNote);
    AppendBool(bytes, payload.pitchFollow);
    AppendBool(bytes, payload.oneShot);
    AppendU64(bytes, payload.mixerChannelId);

    return bytes;
  }

  class PayloadReader {
  public:
    explicit PayloadReader(const std::vector<std::uint8_t>& bytes)
      : bytes_(bytes) {
    }

    std::uint8_t ReadU8(const std::string& fieldName) {
      RequireAvailable(1, fieldName);

      return bytes_[position_++];
    }

    bool ReadBool(const std::string& fieldName) {
      const auto value = ReadU8(fieldName);

      if (value > 1) {
        Fail("Invalid bool value in payload field: " + fieldName);
      }

      return value == 1;
    }

    std::uint32_t ReadU32(const std::string& fieldName) {
      RequireAvailable(4, fieldName);

      const std::uint32_t value =
        static_cast<std::uint32_t>(bytes_[position_]) |
        (static_cast<std::uint32_t>(bytes_[position_ + 1]) << 8) |
        (static_cast<std::uint32_t>(bytes_[position_ + 2]) << 16) |
        (static_cast<std::uint32_t>(bytes_[position_ + 3]) << 24);

      position_ += 4;

      return value;
    }

    std::int32_t ReadI32(const std::string& fieldName) {
      const std::uint32_t raw = ReadU32(fieldName);

      std::int32_t value = 0;
      std::memcpy(&value, &raw, sizeof(value));

      return value;
    }

    std::uint64_t ReadU64(const std::string& fieldName) {
      RequireAvailable(8, fieldName);

      std::uint64_t value = 0;

      for (int i = 0; i < 8; ++i) {
        value |=
          static_cast<std::uint64_t>(bytes_[position_ + i]) << (i * 8);
      }

      position_ += 8;

      return value;
    }

    float ReadF32(const std::string& fieldName) {
      const std::uint32_t raw = ReadU32(fieldName);

      float value = 0.0f;
      std::memcpy(&value, &raw, sizeof(value));

      return value;
    }

    std::string ReadString(const std::string& fieldName) {
      const std::uint64_t size = ReadU64(fieldName + ".size");

      if (size > static_cast<std::uint64_t>(bytes_.size() - position_)) {
        Fail("Payload string is longer than remaining data: " + fieldName);
      }

      std::string value(
        reinterpret_cast<const char*>(bytes_.data() + position_),
        static_cast<std::size_t>(size)
      );

      position_ += static_cast<std::size_t>(size);

      return value;
    }

    void RequireEnd(const std::string& payloadName) const {
      if (position_ == bytes_.size()) {
        return;
      }

      std::cerr
        << "FAILED: " << payloadName << " payload had trailing bytes\n"
        << "  position: " << position_ << "\n"
        << "  size:     " << bytes_.size() << "\n";

      std::exit(1);
    }

  private:
    void RequireAvailable(
      std::size_t count,
      const std::string& fieldName
    ) const {
      if (position_ + count <= bytes_.size()) {
        return;
      }

      std::cerr
        << "FAILED: payload ended while reading " << fieldName << "\n"
        << "  needed: " << count << " byte(s)\n"
        << "  remaining: " << bytes_.size() - position_ << "\n";

      std::exit(1);
    }

    const std::vector<std::uint8_t>& bytes_;
    std::size_t position_ = 0;
  };

  OscillatorPayloadV1 DecodeOscillatorPayloadV1(
    const std::vector<std::uint8_t>& bytes
  ) {
    PayloadReader reader(bytes);

    OscillatorPayloadV1 payload;
    payload.waveShape = reader.ReadU32("waveShape");
    payload.octave = reader.ReadI32("octave");
    payload.semitone = reader.ReadI32("semitone");

    payload.attackSeconds = reader.ReadF32("attackSeconds");
    payload.decaySeconds = reader.ReadF32("decaySeconds");
    payload.sustainLevel = reader.ReadF32("sustainLevel");
    payload.releaseSeconds = reader.ReadF32("releaseSeconds");

    payload.mixerChannelId = reader.ReadU64("mixerChannelId");

    reader.RequireEnd("oscillator");

    return payload;
  }

  SamplerPayloadV1 DecodeSamplerPayloadV1(
    const std::vector<std::uint8_t>& bytes
  ) {
    PayloadReader reader(bytes);

    SamplerPayloadV1 payload;
    payload.samplePath = reader.ReadString("samplePath");
    payload.rootNote = reader.ReadI32("rootNote");
    payload.pitchFollow = reader.ReadBool("pitchFollow");
    payload.oneShot = reader.ReadBool("oneShot");
    payload.mixerChannelId = reader.ReadU64("mixerChannelId");

    reader.RequireEnd("sampler");

    return payload;
  }

  void CheckOscillatorPayloadEqual(
    const OscillatorPayloadV1& actual,
    const OscillatorPayloadV1& expected,
    const std::string& name
  ) {
    CheckEqual(actual.waveShape, expected.waveShape, name + " wave shape");
    CheckEqual(actual.octave, expected.octave, name + " octave");
    CheckEqual(actual.semitone, expected.semitone, name + " semitone");

    CheckNear(actual.attackSeconds, expected.attackSeconds, name + " attack");
    CheckNear(actual.decaySeconds, expected.decaySeconds, name + " decay");
    CheckNear(actual.sustainLevel, expected.sustainLevel, name + " sustain");
    CheckNear(actual.releaseSeconds, expected.releaseSeconds, name + " release");

    CheckEqual(
      actual.mixerChannelId,
      expected.mixerChannelId,
      name + " mixer channel"
    );
  }

  void CheckSamplerPayloadEqual(
    const SamplerPayloadV1& actual,
    const SamplerPayloadV1& expected,
    const std::string& name
  ) {
    CheckEqual(actual.samplePath, expected.samplePath, name + " sample path");
    CheckEqual(actual.rootNote, expected.rootNote, name + " root note");
    CheckEqual(actual.pitchFollow, expected.pitchFollow, name + " pitch follow");
    CheckEqual(actual.oneShot, expected.oneShot, name + " one shot");

    CheckEqual(
      actual.mixerChannelId,
      expected.mixerChannelId,
      name + " mixer channel"
    );
  }

  void AddNote(
    sxp::ProjectPattern& pattern,
    std::uint64_t& nextNoteId,
    int midiNote,
    float velocity,
    double startBeat,
    double lengthBeats
  ) {
    sxp::ProjectPatternNote note;
    note.id = nextNoteId++;
    note.midiNote = midiNote;
    note.velocity = velocity;
    note.startBeat = startBeat;
    note.lengthBeats = lengthBeats;

    pattern.notes.push_back(note);
  }

  sxp::ProjectPattern MakeBassPatternA(std::uint64_t& nextNoteId) {
    sxp::ProjectPattern pattern;
    pattern.id = 1;
    pattern.name = "Bassline A";
    pattern.lengthBeats = 8.0;

    const std::array<int, 8> notes {
      36, 36, 43, 36,
      39, 39, 46, 34
    };

    for (std::size_t i = 0; i < notes.size(); ++i) {
      AddNote(
        pattern,
        nextNoteId,
        notes[i],
        0.92f,
        static_cast<double>(i),
        0.75
      );

      AddNote(
        pattern,
        nextNoteId,
        notes[i] + 12,
        0.42f,
        static_cast<double>(i) + 0.50,
        0.25
      );
    }

    return pattern;
  }

  sxp::ProjectPattern MakeBassPatternB(std::uint64_t& nextNoteId) {
    sxp::ProjectPattern pattern;
    pattern.id = 2;
    pattern.name = "Bassline B";
    pattern.lengthBeats = 8.0;

    const std::array<int, 16> notes {
      36, 36, 36, 43,
      39, 39, 41, 43,
      34, 34, 34, 41,
      38, 38, 41, 46
    };

    for (std::size_t i = 0; i < notes.size(); ++i) {
      AddNote(
        pattern,
        nextNoteId,
        notes[i],
        0.80f + static_cast<float>(i % 3) * 0.05f,
        static_cast<double>(i) * 0.5,
        0.375
      );
    }

    return pattern;
  }

  sxp::ProjectPattern MakeDrumPatternA(std::uint64_t& nextNoteId) {
    sxp::ProjectPattern pattern;
    pattern.id = 3;
    pattern.name = "Drums A";
    pattern.lengthBeats = 8.0;

    for (int beat = 0; beat < 8; ++beat) {
      AddNote(pattern, nextNoteId, 36, 1.0f, beat, 0.125);

      if (beat == 2 || beat == 6) {
        AddNote(pattern, nextNoteId, 38, 0.95f, beat, 0.125);
      }
    }

    for (int step = 0; step < 32; ++step) {
      const bool accent = step % 4 == 0;

      AddNote(
        pattern,
        nextNoteId,
        42,
        accent ? 0.58f : 0.38f,
        static_cast<double>(step) * 0.25,
        0.08
      );
    }

    AddNote(pattern, nextNoteId, 49, 0.65f, 3.75, 0.125);
    AddNote(pattern, nextNoteId, 49, 0.70f, 7.75, 0.125);

    return pattern;
  }

  sxp::ProjectPattern MakeDrumPatternB(std::uint64_t& nextNoteId) {
    sxp::ProjectPattern pattern;
    pattern.id = 4;
    pattern.name = "Drums B";
    pattern.lengthBeats = 8.0;

    const std::array<double, 11> kicks {
      0.0, 0.75, 1.5, 2.75,
      4.0, 4.5, 5.25, 6.0,
      6.75, 7.25, 7.75
    };

    for (double start : kicks) {
      AddNote(pattern, nextNoteId, 36, 1.0f, start, 0.125);
    }

    AddNote(pattern, nextNoteId, 38, 1.0f, 2.0, 0.125);
    AddNote(pattern, nextNoteId, 38, 0.95f, 6.0, 0.125);
    AddNote(pattern, nextNoteId, 38, 0.50f, 7.5, 0.125);

    for (int step = 0; step < 64; ++step) {
      const bool accent = step % 8 == 0;

      AddNote(
        pattern,
        nextNoteId,
        42,
        accent ? 0.62f : 0.33f,
        static_cast<double>(step) * 0.125,
        0.06
      );
    }

    return pattern;
  }

  sxp::ProjectPattern MakeLeadPattern(std::uint64_t& nextNoteId) {
    sxp::ProjectPattern pattern;
    pattern.id = 5;
    pattern.name = "Lead Hook";
    pattern.lengthBeats = 16.0;

    const std::array<int, 16> notes {
      72, 74, 75, 79,
      77, 75, 74, 72,
      67, 70, 72, 75,
      74, 72, 70, 67
    };

    for (std::size_t i = 0; i < notes.size(); ++i) {
      AddNote(
        pattern,
        nextNoteId,
        notes[i],
        0.74f,
        static_cast<double>(i),
        i % 4 == 3 ? 0.90 : 0.50
      );
    }

    return pattern;
  }

  sxp::ProjectPattern MakePluckPattern(std::uint64_t& nextNoteId) {
    sxp::ProjectPattern pattern;
    pattern.id = 6;
    pattern.name = "Pluck Arp";
    pattern.lengthBeats = 16.0;

    const std::array<int, 4> chordA { 60, 64, 67, 72 };
    const std::array<int, 4> chordB { 58, 62, 65, 70 };
    const std::array<int, 4> chordC { 55, 59, 62, 67 };
    const std::array<int, 4> chordD { 63, 67, 70, 75 };

    const std::array<std::array<int, 4>, 4> chords {
      chordA,
      chordB,
      chordC,
      chordD
    };

    for (std::size_t bar = 0; bar < chords.size(); ++bar) {
      for (int step = 0; step < 16; ++step) {
        const auto& chord = chords[bar];
        const int note = chord[static_cast<std::size_t>(step) % chord.size()];

        AddNote(
          pattern,
          nextNoteId,
          note,
          step % 4 == 0 ? 0.62f : 0.48f,
          static_cast<double>(bar * 4) + static_cast<double>(step) * 0.25,
          0.18
        );
      }
    }

    return pattern;
  }

  sxp::ProjectPattern MakeBreakFillPattern(std::uint64_t& nextNoteId) {
    sxp::ProjectPattern pattern;
    pattern.id = 7;
    pattern.name = "Break Fill";
    pattern.lengthBeats = 4.0;

    for (int step = 0; step < 16; ++step) {
      AddNote(
        pattern,
        nextNoteId,
        step % 2 == 0 ? 38 : 45,
        0.45f + static_cast<float>(step) * 0.025f,
        static_cast<double>(step) * 0.25,
        0.10
      );
    }

    AddNote(pattern, nextNoteId, 49, 0.90f, 3.50, 0.125);
    AddNote(pattern, nextNoteId, 36, 1.00f, 3.75, 0.125);

    return pattern;
  }

  sxp::ProjectGenerator MakeOscillatorGenerator(
    std::uint64_t nodeId,
    std::string name,
    const OscillatorPayloadV1& payload
  ) {
    sxp::ProjectGenerator generator;
    generator.nodeId = nodeId;
    generator.name = std::move(name);
    generator.type = sxp::ProjectGeneratorType::Oscillator;
    generator.version = 1;
    generator.payload = EncodeOscillatorPayloadV1(payload);

    return generator;
  }

  sxp::ProjectGenerator MakeSamplerGenerator(
    std::uint64_t nodeId,
    std::string name,
    const SamplerPayloadV1& payload
  ) {
    sxp::ProjectGenerator generator;
    generator.nodeId = nodeId;
    generator.name = std::move(name);
    generator.type = sxp::ProjectGeneratorType::Sampler;
    generator.version = 1;
    generator.payload = EncodeSamplerPayloadV1(payload);

    return generator;
  }

  void AddClip(
    sxp::ProjectDocument& project,
    std::uint64_t id,
    std::uint64_t trackId,
    std::uint64_t patternId,
    double startBeat,
    double lengthBeats,
    double patternStartBeat,
    bool muted
  ) {
    sxp::ProjectArrangementClip clip;
    clip.id = id;
    clip.trackId = trackId;
    clip.patternId = patternId;
    clip.startBeat = startBeat;
    clip.lengthBeats = lengthBeats;
    clip.patternStartBeat = patternStartBeat;
    clip.muted = muted;

    project.arrangementClips.push_back(clip);
  }

  sxp::ProjectDocument CreateTestProject() {
    sxp::ProjectDocument project;

    project.header.projectName = "Big Synthem SXP Stress Project";
    project.header.bpm = 174.0;
    project.header.timeSigNumerator = 4;
    project.header.timeSigDenominator = 4;
    project.header.swingAmount = 0.18;
    project.header.swingSubdivisionBeats = 0.25;

    project.generators.push_back(MakeOscillatorGenerator(
      10,
      "Sub Sine",
      OscillatorPayloadV1 {
        .waveShape = 0,
        .octave = -1,
        .semitone = 0,
        .attackSeconds = 0.004f,
        .decaySeconds = 0.180f,
        .sustainLevel = 0.86f,
        .releaseSeconds = 0.055f,
        .mixerChannelId = 1
      }
    ));

    project.generators.push_back(MakeOscillatorGenerator(
      11,
      "Reese Saw",
      OscillatorPayloadV1 {
        .waveShape = 2,
        .octave = 0,
        .semitone = -5,
        .attackSeconds = 0.012f,
        .decaySeconds = 0.420f,
        .sustainLevel = 0.72f,
        .releaseSeconds = 0.210f,
        .mixerChannelId = 1
      }
    ));

    project.generators.push_back(MakeOscillatorGenerator(
      12,
      "Lead Square",
      OscillatorPayloadV1 {
        .waveShape = 1,
        .octave = 1,
        .semitone = 0,
        .attackSeconds = 0.018f,
        .decaySeconds = 0.240f,
        .sustainLevel = 0.64f,
        .releaseSeconds = 0.300f,
        .mixerChannelId = 4
      }
    ));

    project.generators.push_back(MakeOscillatorGenerator(
      13,
      "Glass Pluck",
      OscillatorPayloadV1 {
        .waveShape = 3,
        .octave = 2,
        .semitone = 7,
        .attackSeconds = 0.001f,
        .decaySeconds = 0.090f,
        .sustainLevel = 0.22f,
        .releaseSeconds = 0.130f,
        .mixerChannelId = 5
      }
    ));

    project.generators.push_back(MakeSamplerGenerator(
      20,
      "Kick Sampler",
      SamplerPayloadV1 {
        .samplePath = "samples/drums/kick_punchy_01.wav",
        .rootNote = 36,
        .pitchFollow = false,
        .oneShot = true,
        .mixerChannelId = 2
      }
    ));

    project.generators.push_back(MakeSamplerGenerator(
      21,
      "Snare Sampler",
      SamplerPayloadV1 {
        .samplePath = "samples/drums/snare_snap_03.wav",
        .rootNote = 38,
        .pitchFollow = false,
        .oneShot = true,
        .mixerChannelId = 2
      }
    ));

    project.generators.push_back(MakeSamplerGenerator(
      22,
      "Hat Sampler",
      SamplerPayloadV1 {
        .samplePath = "samples/drums/closed_hat_tight.wav",
        .rootNote = 42,
        .pitchFollow = false,
        .oneShot = true,
        .mixerChannelId = 3
      }
    ));

    sxp::ProjectTrack bassTrack;
    bassTrack.id = 1;
    bassTrack.name = "Bass Stack";
    bassTrack.generatorNodeIds = { 10, 11 };
    project.tracks.push_back(bassTrack);

    sxp::ProjectTrack drumTrack;
    drumTrack.id = 2;
    drumTrack.name = "Drum Rack";
    drumTrack.generatorNodeIds = { 20, 21, 22 };
    project.tracks.push_back(drumTrack);

    sxp::ProjectTrack leadTrack;
    leadTrack.id = 3;
    leadTrack.name = "Lead";
    leadTrack.generatorNodeIds = { 12 };
    project.tracks.push_back(leadTrack);

    sxp::ProjectTrack pluckTrack;
    pluckTrack.id = 4;
    pluckTrack.name = "Pluck Layer";
    pluckTrack.generatorNodeIds = { 13 };
    project.tracks.push_back(pluckTrack);

    std::uint64_t nextNoteId = 1;

    project.patterns.push_back(MakeBassPatternA(nextNoteId));
    project.patterns.push_back(MakeBassPatternB(nextNoteId));
    project.patterns.push_back(MakeDrumPatternA(nextNoteId));
    project.patterns.push_back(MakeDrumPatternB(nextNoteId));
    project.patterns.push_back(MakeLeadPattern(nextNoteId));
    project.patterns.push_back(MakePluckPattern(nextNoteId));
    project.patterns.push_back(MakeBreakFillPattern(nextNoteId));

    AddClip(project, 1,  1, 1,  0.0,  8.0, 0.0, false);
    AddClip(project, 2,  2, 3,  0.0,  8.0, 0.0, false);
    AddClip(project, 3,  4, 6,  0.0, 16.0, 0.0, false);

    AddClip(project, 4,  1, 1,  8.0,  8.0, 0.0, false);
    AddClip(project, 5,  2, 3,  8.0,  8.0, 0.0, false);
    AddClip(project, 6,  3, 5,  8.0,  8.0, 0.0, true);

    AddClip(project, 7,  1, 2, 16.0,  8.0, 0.0, false);
    AddClip(project, 8,  2, 4, 16.0,  8.0, 0.0, false);
    AddClip(project, 9,  3, 5, 16.0, 16.0, 0.0, false);
    AddClip(project, 10, 4, 6, 16.0, 16.0, 4.0, false);

    AddClip(project, 11, 1, 2, 24.0,  8.0, 0.0, false);
    AddClip(project, 12, 2, 7, 28.0,  4.0, 0.0, false);
    AddClip(project, 13, 4, 6, 32.0, 16.0, 0.0, true);

    return project;
  }

  std::size_t CountNotes(const sxp::ProjectDocument& project) {
    std::size_t count = 0;

    for (const auto& pattern : project.patterns) {
      count += pattern.notes.size();
    }

    return count;
  }

  void ValidateProjectHeader(
    const sxp::ProjectDocument& actual,
    const sxp::ProjectDocument& expected
  ) {
    CheckEqual(
      actual.header.projectName,
      expected.header.projectName,
      "project name"
    );

    CheckNear(actual.header.bpm, expected.header.bpm, "project bpm");

    CheckEqual(
      actual.header.timeSigNumerator,
      expected.header.timeSigNumerator,
      "time signature numerator"
    );

    CheckEqual(
      actual.header.timeSigDenominator,
      expected.header.timeSigDenominator,
      "time signature denominator"
    );

    CheckNear(
      actual.header.swingAmount,
      expected.header.swingAmount,
      "swing amount"
    );

    CheckNear(
      actual.header.swingSubdivisionBeats,
      expected.header.swingSubdivisionBeats,
      "swing subdivision beats"
    );
  }

  void ValidateGenerators(
    const sxp::ProjectDocument& actual,
    const sxp::ProjectDocument& expected
  ) {
    CheckEqual(
      actual.generators.size(),
      expected.generators.size(),
      "generator count"
    );

    for (std::size_t i = 0; i < expected.generators.size(); ++i) {
      const auto& actualGenerator = actual.generators[i];
      const auto& expectedGenerator = expected.generators[i];

      const std::string prefix =
        "generator " + std::to_string(i);

      CheckEqual(
        actualGenerator.nodeId,
        expectedGenerator.nodeId,
        prefix + " node id"
      );

      CheckEqual(
        actualGenerator.name,
        expectedGenerator.name,
        prefix + " name"
      );

      CheckEqual(
        actualGenerator.version,
        expectedGenerator.version,
        prefix + " version"
      );

      CheckEqual(
        static_cast<std::uint32_t>(actualGenerator.type),
        static_cast<std::uint32_t>(expectedGenerator.type),
        prefix + " type"
      );

      CheckBytesEqual(
        actualGenerator.payload,
        expectedGenerator.payload,
        prefix + " payload bytes"
      );

      if (actualGenerator.type == sxp::ProjectGeneratorType::Oscillator) {
        CheckOscillatorPayloadEqual(
          DecodeOscillatorPayloadV1(actualGenerator.payload),
          DecodeOscillatorPayloadV1(expectedGenerator.payload),
          prefix + " decoded oscillator payload"
        );
      } else if (actualGenerator.type == sxp::ProjectGeneratorType::Sampler) {
        CheckSamplerPayloadEqual(
          DecodeSamplerPayloadV1(actualGenerator.payload),
          DecodeSamplerPayloadV1(expectedGenerator.payload),
          prefix + " decoded sampler payload"
        );
      }
    }
  }

  void ValidateTracks(
    const sxp::ProjectDocument& actual,
    const sxp::ProjectDocument& expected
  ) {
    CheckEqual(
      actual.tracks.size(),
      expected.tracks.size(),
      "track count"
    );

    for (std::size_t i = 0; i < expected.tracks.size(); ++i) {
      const auto& actualTrack = actual.tracks[i];
      const auto& expectedTrack = expected.tracks[i];

      const std::string prefix =
        "track " + std::to_string(i);

      CheckEqual(actualTrack.id, expectedTrack.id, prefix + " id");
      CheckEqual(actualTrack.name, expectedTrack.name, prefix + " name");

      CheckEqual(
        actualTrack.generatorNodeIds.size(),
        expectedTrack.generatorNodeIds.size(),
        prefix + " generator node count"
      );

      for (std::size_t j = 0; j < expectedTrack.generatorNodeIds.size(); ++j) {
        CheckEqual(
          actualTrack.generatorNodeIds[j],
          expectedTrack.generatorNodeIds[j],
          prefix + " generator node " + std::to_string(j)
        );
      }
    }
  }

  void ValidatePatterns(
    const sxp::ProjectDocument& actual,
    const sxp::ProjectDocument& expected
  ) {
    CheckEqual(
      actual.patterns.size(),
      expected.patterns.size(),
      "pattern count"
    );

    for (std::size_t i = 0; i < expected.patterns.size(); ++i) {
      const auto& actualPattern = actual.patterns[i];
      const auto& expectedPattern = expected.patterns[i];

      const std::string prefix =
        "pattern " + std::to_string(i);

      CheckEqual(actualPattern.id, expectedPattern.id, prefix + " id");
      CheckEqual(actualPattern.name, expectedPattern.name, prefix + " name");
      CheckNear(
        actualPattern.lengthBeats,
        expectedPattern.lengthBeats,
        prefix + " length beats"
      );

      CheckEqual(
        actualPattern.notes.size(),
        expectedPattern.notes.size(),
        prefix + " note count"
      );

      for (std::size_t j = 0; j < expectedPattern.notes.size(); ++j) {
        const auto& actualNote = actualPattern.notes[j];
        const auto& expectedNote = expectedPattern.notes[j];

        const std::string notePrefix =
          prefix + " note " + std::to_string(j);

        CheckEqual(actualNote.id, expectedNote.id, notePrefix + " id");

        CheckEqual(
          static_cast<int>(actualNote.midiNote),
          static_cast<int>(expectedNote.midiNote),
          notePrefix + " midi note"
        );

        CheckNear(actualNote.velocity, expectedNote.velocity, notePrefix + " velocity");
        CheckNear(actualNote.startBeat, expectedNote.startBeat, notePrefix + " start beat");
        CheckNear(actualNote.lengthBeats, expectedNote.lengthBeats, notePrefix + " length beats");
      }
    }
  }

  void ValidateArrangementClips(
    const sxp::ProjectDocument& actual,
    const sxp::ProjectDocument& expected
  ) {
    CheckEqual(
      actual.arrangementClips.size(),
      expected.arrangementClips.size(),
      "arrangement clip count"
    );

    for (std::size_t i = 0; i < expected.arrangementClips.size(); ++i) {
      const auto& actualClip = actual.arrangementClips[i];
      const auto& expectedClip = expected.arrangementClips[i];

      const std::string prefix =
        "clip " + std::to_string(i);

      CheckEqual(actualClip.id, expectedClip.id, prefix + " id");
      CheckEqual(actualClip.trackId, expectedClip.trackId, prefix + " track id");
      CheckEqual(actualClip.patternId, expectedClip.patternId, prefix + " pattern id");

      CheckNear(actualClip.startBeat, expectedClip.startBeat, prefix + " start beat");
      CheckNear(actualClip.lengthBeats, expectedClip.lengthBeats, prefix + " length beats");

      CheckNear(
        actualClip.patternStartBeat,
        expectedClip.patternStartBeat,
        prefix + " pattern start beat"
      );

      CheckEqual(actualClip.muted, expectedClip.muted, prefix + " muted");
    }
  }

  void ValidateLoadedProject(
    const sxp::ProjectDocument& actual,
    const sxp::ProjectDocument& expected
  ) {
    ValidateProjectHeader(actual, expected);
    ValidateGenerators(actual, expected);
    ValidateTracks(actual, expected);
    ValidatePatterns(actual, expected);
    ValidateArrangementClips(actual, expected);
  }
}

int main() {
  const std::string path = "big-synthem-stress-project.sxp";

  sxp::ProjectDocument project = CreateTestProject();

  Require(
    sxp::ProjectFile::Save(path, project),
    "save project"
  );

  auto result = sxp::ProjectFile::Load(path);

  if (!result.Ok()) {
    Require(result.error, "load project");
  }

  ValidateLoadedProject(result.value, project);

  std::cout
    << "SXP project passed: "
    << path
    << "\n";

  std::cout
    << "  project: "
    << result.value.header.projectName
    << "\n";

  std::cout
    << "  bpm: "
    << result.value.header.bpm
    << "\n";

  std::cout
    << "  tracks: "
    << result.value.tracks.size()
    << "\n";

  std::cout
    << "  patterns: "
    << result.value.patterns.size()
    << "\n";

  std::cout
    << "  notes: "
    << CountNotes(result.value)
    << "\n";

  std::cout
    << "  arrangement clips: "
    << result.value.arrangementClips.size()
    << "\n";

  std::cout
    << "  generators: "
    << result.value.generators.size()
    << "\n";

  return 0;
}
