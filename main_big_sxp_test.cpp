#include <sxp/project/ProjectDocument.h>
#include <sxp/project/ProjectFile.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "sxp/asset/AssetId.h"
#include "sxp/project/ProjectGenerator.h"
#include "sxp/project/generator_payload/GeneratorPayloadCodec.h"
#include "sxp/project/generator_payload/OscillatorPayload.h"


namespace {
  constexpr double Epsilon = 0.000001;

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

  constexpr const char* CymaticsDrumPath =
      "/home/code/Downloads/cymatics-phoenixrebirt/"
      "Cymatics - Phoenix Rebirth/5 Drum One Shots - Electronic/";

  struct TestAssetSource {
    sxp::AssetId id;
    std::string displayName;
    std::string path;
    std::string mimeType;
  };

  sxp::AssetId MakeAssetId(
    std::uint64_t high,
    std::uint64_t low
  ) {
    sxp::AssetId id;
    id.high = high;
    id.low = low;
    return id;
  }

  std::string JoinSamplePath(const std::string& fileName) {
    return std::string(CymaticsDrumPath) + fileName;
  }

  std::vector<std::uint8_t> ReadFileBytes(
    const std::string& path
  ) {
    std::ifstream file(
      path,
      std::ios::binary | std::ios::ate
    );

    if (!file.is_open()) {
      Fail("Could not open asset file: " + path);
    }

    const auto size = file.tellg();

    if (size < 0) {
      Fail("Could not get asset file size: " + path);
    }

    std::vector<std::uint8_t> bytes(
      static_cast<std::size_t>(size)
    );

    file.seekg(0, std::ios::beg);

    if (!bytes.empty()) {
      file.read(
        reinterpret_cast<char*>(bytes.data()),
        static_cast<std::streamsize>(bytes.size())
      );
    }

    if (!file.good() && !file.eof()) {
      Fail("Could not read asset file: " + path);
    }

    return bytes;
  }

  std::vector<TestAssetSource> MakeTestAssetSources() {
    return {
      {
        MakeAssetId(1, 1),
        "Cymatics - Kick (Driven).wav",
        JoinSamplePath("Cymatics - Kick (Driven).wav"),
        "audio/wav"
      },
      {
        MakeAssetId(1, 2),
        "Cymatics - Hihat (Farther).wav",
        JoinSamplePath("Cymatics - Hihat (Farther).wav"),
        "audio/wav"
      },
      {
        MakeAssetId(1, 3),
        "Cymatics - Open Hihat (Feather).wav",
        JoinSamplePath("Cymatics - Open Hihat (Feather).wav"),
        "audio/wav"
      },
      {
        MakeAssetId(1, 4),
        "Cymatics - Clap (Process).wav",
        JoinSamplePath("Cymatics - Clap (Process).wav"),
        "audio/wav"
      }
    };
  }

  void CheckOscillatorPayloadEqual(
    const sxp::ProjectOscillatorPayloadV1& actual,
    const sxp::ProjectOscillatorPayloadV1& expected,
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
    const sxp::ProjectSamplerPayloadV1& actual,
    const sxp::ProjectSamplerPayloadV1& expected,
    const std::string& name
  ) {
    CheckEqual(
      actual.sampleAssetId.high,
      expected.sampleAssetId.high,
      name + " sample asset id high"
    );

    CheckEqual(
      actual.sampleAssetId.low,
      expected.sampleAssetId.low,
      name + " sample asset id low"
    );

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
    const sxp::ProjectOscillatorPayloadV1& payload
  ) {
    auto payloadResult =
      sxp::ProjectGeneratorPayloadCodec::EncodeOscillatorV1(payload);

    if (!payloadResult.Ok()) {
      Require(payloadResult.error, "encode oscillator payload");
    }

    sxp::ProjectGenerator generator;
    generator.nodeId = nodeId;
    generator.name = std::move(name);
    generator.type = sxp::ProjectGeneratorType::Oscillator;
    generator.version = 1;
    generator.payload = std::move(payloadResult.value);

    return generator;
  }

  sxp::ProjectGenerator MakeSamplerGenerator(
    std::uint64_t nodeId,
    std::string name,
    const sxp::ProjectSamplerPayloadV1& payload
  ) {
    auto payloadResult =
      sxp::ProjectGeneratorPayloadCodec::EncodeSamplerV1(payload);

    if (!payloadResult.Ok()) {
      Require(payloadResult.error, "encode sampler payload");
    }

    sxp::ProjectGenerator generator;
    generator.nodeId = nodeId;
    generator.name = std::move(name);
    generator.type = sxp::ProjectGeneratorType::Sampler;
    generator.version = 1;
    generator.payload = std::move(payloadResult.value);

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

  void AddTestAssets(sxp::ProjectDocument& project) {
    for (const auto& source : MakeTestAssetSources()) {
      sxp::ProjectAsset asset;
      asset.id = source.id;
      asset.kind = sxp::AssetKind::AudioSample;
      asset.displayName = source.displayName;
      asset.originalPathHint = source.path;
      asset.mimeType = source.mimeType;
      asset.data = ReadFileBytes(source.path);

      project.assets.push_back(std::move(asset));
    }
  }

  sxp::ProjectPattern MakeKickPattern(std::uint64_t& nextNoteId) {
    sxp::ProjectPattern pattern;
    pattern.id = 1;
    pattern.name = "Kick - Four On The Floor";
    pattern.lengthBeats = 16.0;

    for (int beat = 0; beat < 16; ++beat) {
      AddNote(
        pattern,
        nextNoteId,
        36,
        beat % 4 == 0 ? 1.0f : 0.92f,
        static_cast<double>(beat),
        0.125
      );
    }

    return pattern;
  }

  sxp::ProjectPattern MakeClapPattern(std::uint64_t& nextNoteId) {
    sxp::ProjectPattern pattern;
    pattern.id = 2;
    pattern.name = "Clap - House Backbeat";
    pattern.lengthBeats = 16.0;

    for (int bar = 0; bar < 4; ++bar) {
      AddNote(pattern, nextNoteId, 38, 0.95f, bar * 4.0 + 1.0, 0.125);
      AddNote(pattern, nextNoteId, 38, 1.00f, bar * 4.0 + 3.0, 0.125);
    }

    return pattern;
  }

  sxp::ProjectPattern MakeHatPattern(std::uint64_t& nextNoteId) {
    sxp::ProjectPattern pattern;
    pattern.id = 3;
    pattern.name = "Closed Hats - Offbeat Push";
    pattern.lengthBeats = 16.0;

    for (int beat = 0; beat < 16; ++beat) {
      AddNote(
        pattern,
        nextNoteId,
        42,
        beat % 4 == 2 ? 0.72f : 0.58f,
        static_cast<double>(beat) + 0.5,
        0.08
      );
    }

    for (int step = 0; step < 64; ++step) {
      if (step % 4 == 2) {
        continue;
      }

      const bool accent = step % 8 == 0;

      AddNote(
        pattern,
        nextNoteId,
        42,
        accent ? 0.34f : 0.22f,
        static_cast<double>(step) * 0.25,
        0.04
      );
    }

    return pattern;
  }

  sxp::ProjectPattern MakeOpenHatPattern(std::uint64_t& nextNoteId) {
    sxp::ProjectPattern pattern;
    pattern.id = 4;
    pattern.name = "Open Hat Lift";
    pattern.lengthBeats = 16.0;

    for (int bar = 0; bar < 4; ++bar) {
      AddNote(pattern, nextNoteId, 46, 0.68f, bar * 4.0 + 3.75, 0.125);
    }

    return pattern;
  }

  sxp::ProjectPattern MakeBassPattern(std::uint64_t& nextNoteId) {
    sxp::ProjectPattern pattern;
    pattern.id = 5;
    pattern.name = "Bass - Rolling House";
    pattern.lengthBeats = 16.0;

    struct BassNote {
      int note;
      double start;
      double length;
      float velocity;
    };

    const std::array<BassNote, 16> notes {
      BassNote { 36, 0.50, 0.35, 0.88f },
      BassNote { 36, 1.50, 0.35, 0.78f },
      BassNote { 43, 2.50, 0.35, 0.84f },
      BassNote { 39, 3.50, 0.35, 0.76f },

      BassNote { 36, 4.50, 0.35, 0.90f },
      BassNote { 36, 5.50, 0.35, 0.76f },
      BassNote { 43, 6.25, 0.25, 0.72f },
      BassNote { 46, 6.75, 0.25, 0.68f },

      BassNote { 34, 8.50, 0.35, 0.86f },
      BassNote { 34, 9.50, 0.35, 0.76f },
      BassNote { 41, 10.50, 0.35, 0.82f },
      BassNote { 38, 11.50, 0.35, 0.76f },

      BassNote { 36, 12.50, 0.35, 0.90f },
      BassNote { 43, 13.50, 0.35, 0.80f },
      BassNote { 39, 14.50, 0.35, 0.78f },
      BassNote { 34, 15.50, 0.35, 0.74f }
    };

    for (const auto& note : notes) {
      AddNote(
        pattern,
        nextNoteId,
        note.note,
        note.velocity,
        note.start,
        note.length
      );
    }

    return pattern;
  }

  sxp::ProjectPattern MakeChordStabPattern(std::uint64_t& nextNoteId) {
    sxp::ProjectPattern pattern;
    pattern.id = 6;
    pattern.name = "Chord Stabs";
    pattern.lengthBeats = 16.0;

    const std::array<std::array<int, 4>, 4> chords {
      std::array<int, 4> { 60, 64, 67, 71 },
      std::array<int, 4> { 58, 62, 65, 69 },
      std::array<int, 4> { 55, 59, 62, 67 },
      std::array<int, 4> { 63, 67, 70, 74 }
    };

    for (std::size_t bar = 0; bar < chords.size(); ++bar) {
      const double barStart = static_cast<double>(bar) * 4.0;

      for (double offset : { 0.0, 1.5, 2.5 }) {
        for (int note : chords[bar]) {
          AddNote(
            pattern,
            nextNoteId,
            note,
            offset == 0.0 ? 0.62f : 0.48f,
            barStart + offset,
            0.22
          );
        }
      }
    }

    return pattern;
  }

  sxp::ProjectPattern MakeBreakPattern(std::uint64_t& nextNoteId) {
    sxp::ProjectPattern pattern;
    pattern.id = 7;
    pattern.name = "Mini Drum Fill";
    pattern.lengthBeats = 4.0;

    for (int step = 0; step < 16; ++step) {
      const int note = step % 2 == 0 ? 38 : 42;

      AddNote(
        pattern,
        nextNoteId,
        note,
        0.35f + static_cast<float>(step) * 0.025f,
        static_cast<double>(step) * 0.25,
        0.06
      );
    }

    AddNote(pattern, nextNoteId, 46, 0.75f, 3.50, 0.125);
    AddNote(pattern, nextNoteId, 36, 1.00f, 3.75, 0.125);

    return pattern;
  }

  sxp::ProjectDocument CreateHouseProject() {
    sxp::ProjectDocument project;

    project.header.projectName = "Embedded Sample House Groove";
    project.header.bpm = 128.0;
    project.header.timeSigNumerator = 4;
    project.header.timeSigDenominator = 4;
    project.header.swingAmount = 0.18;
    project.header.swingSubdivisionBeats = 0.25;

    const auto assetSources = MakeTestAssetSources();

    const auto kickAsset = assetSources[0];
    const auto closedHatAsset = assetSources[1];
    const auto openHatAsset = assetSources[2];
    const auto clapAsset = assetSources[3];

    project.generators.push_back(MakeSamplerGenerator(
      10,
      "Kick Sampler",
      sxp::ProjectSamplerPayloadV1 {
        .sampleAssetId = kickAsset.id,
        .samplePath = kickAsset.path,
        .rootNote = 36,
        .pitchFollow = false,
        .oneShot = true,
        .mixerChannelId = 1
      }
    ));

    project.generators.push_back(MakeSamplerGenerator(
      11,
      "Clap Sampler",
      sxp::ProjectSamplerPayloadV1 {
        .sampleAssetId = clapAsset.id,
        .samplePath = clapAsset.path,
        .rootNote = 38,
        .pitchFollow = false,
        .oneShot = true,
        .mixerChannelId = 2
      }
    ));

    project.generators.push_back(MakeSamplerGenerator(
      12,
      "Closed Hat Sampler",
      sxp::ProjectSamplerPayloadV1 {
        .sampleAssetId = closedHatAsset.id,
        .samplePath = closedHatAsset.path,
        .rootNote = 42,
        .pitchFollow = false,
        .oneShot = true,
        .mixerChannelId = 3
      }
    ));

    project.generators.push_back(MakeSamplerGenerator(
      13,
      "Open Hat Sampler",
      sxp::ProjectSamplerPayloadV1 {
        .sampleAssetId = openHatAsset.id,
        .samplePath = openHatAsset.path,
        .rootNote = 46,
        .pitchFollow = false,
        .oneShot = true,
        .mixerChannelId = 3
      }
    ));

    project.generators.push_back(MakeOscillatorGenerator(
      20,
      "House Bass",
      sxp::ProjectOscillatorPayloadV1 {
        .waveShape = 2,
        .octave = -1,
        .semitone = 0,
        .attackSeconds = 0.004f,
        .decaySeconds = 0.180f,
        .sustainLevel = 0.70f,
        .releaseSeconds = 0.080f,
        .mixerChannelId = 4
      }
    ));

    project.generators.push_back(MakeOscillatorGenerator(
      21,
      "Chord Stab",
      sxp::ProjectOscillatorPayloadV1 {
        .waveShape = 1,
        .octave = 0,
        .semitone = 0,
        .attackSeconds = 0.002f,
        .decaySeconds = 0.130f,
        .sustainLevel = 0.28f,
        .releaseSeconds = 0.160f,
        .mixerChannelId = 5
      }
    ));

    sxp::ProjectTrack kickTrack;
    kickTrack.id = 1;
    kickTrack.name = "Kick";
    kickTrack.generatorNodeIds = { 10 };
    project.tracks.push_back(kickTrack);

    sxp::ProjectTrack clapTrack;
    clapTrack.id = 2;
    clapTrack.name = "Clap";
    clapTrack.generatorNodeIds = { 11 };
    project.tracks.push_back(clapTrack);

    sxp::ProjectTrack hatTrack;
    hatTrack.id = 3;
    hatTrack.name = "Closed Hats";
    hatTrack.generatorNodeIds = { 12 };
    project.tracks.push_back(hatTrack);

    sxp::ProjectTrack openHatTrack;
    openHatTrack.id = 4;
    openHatTrack.name = "Open Hat";
    openHatTrack.generatorNodeIds = { 13 };
    project.tracks.push_back(openHatTrack);

    sxp::ProjectTrack bassTrack;
    bassTrack.id = 5;
    bassTrack.name = "Bass";
    bassTrack.generatorNodeIds = { 20 };
    project.tracks.push_back(bassTrack);

    sxp::ProjectTrack chordTrack;
    chordTrack.id = 6;
    chordTrack.name = "Chord Stabs";
    chordTrack.generatorNodeIds = { 21 };
    project.tracks.push_back(chordTrack);

    std::uint64_t nextNoteId = 1;

    project.patterns.push_back(MakeKickPattern(nextNoteId));
    project.patterns.push_back(MakeClapPattern(nextNoteId));
    project.patterns.push_back(MakeHatPattern(nextNoteId));
    project.patterns.push_back(MakeOpenHatPattern(nextNoteId));
    project.patterns.push_back(MakeBassPattern(nextNoteId));
    project.patterns.push_back(MakeChordStabPattern(nextNoteId));
    project.patterns.push_back(MakeBreakPattern(nextNoteId));

    // Intro: drums and hats only.
    AddClip(project, 1, 1, 1,  0.0, 16.0, 0.0, false);
    AddClip(project, 2, 3, 3,  0.0, 16.0, 0.0, false);
    AddClip(project, 3, 4, 4, 12.0,  4.0, 0.0, false);

    // Main groove.
    AddClip(project, 4, 1, 1, 16.0, 16.0, 0.0, false);
    AddClip(project, 5, 2, 2, 16.0, 16.0, 0.0, false);
    AddClip(project, 6, 3, 3, 16.0, 16.0, 0.0, false);
    AddClip(project, 7, 4, 4, 16.0, 16.0, 0.0, false);
    AddClip(project, 8, 5, 5, 16.0, 16.0, 0.0, false);

    // Full section.
    AddClip(project, 9,  1, 1, 32.0, 16.0, 0.0, false);
    AddClip(project, 10, 2, 2, 32.0, 16.0, 0.0, false);
    AddClip(project, 11, 3, 3, 32.0, 16.0, 0.0, false);
    AddClip(project, 12, 4, 4, 32.0, 16.0, 0.0, false);
    AddClip(project, 13, 5, 5, 32.0, 16.0, 0.0, false);
    AddClip(project, 14, 6, 6, 32.0, 16.0, 0.0, false);

    // Fill into final loop.
    AddClip(project, 15, 2, 7, 48.0,  4.0, 0.0, false);
    AddClip(project, 16, 4, 4, 48.0,  4.0, 0.0, false);

    // Final groove.
    AddClip(project, 17, 1, 1, 48.0, 16.0, 0.0, false);
    AddClip(project, 18, 2, 2, 52.0, 12.0, 4.0, false);
    AddClip(project, 19, 3, 3, 48.0, 16.0, 0.0, false);
    AddClip(project, 20, 4, 4, 48.0, 16.0, 0.0, false);
    AddClip(project, 21, 5, 5, 48.0, 16.0, 0.0, false);
    AddClip(project, 22, 6, 6, 48.0, 16.0, 0.0, false);

    return project;
  }

  sxp::ProjectDocument CreateOldHouseProject() {
    sxp::ProjectDocument project;

    project.header.projectName = "My Synthem Project";
    project.header.bpm = 128.0;
    project.header.timeSigNumerator = 4;
    project.header.timeSigDenominator = 4;
    project.header.swingAmount = 0.35;
    project.header.swingSubdivisionBeats = 0.25;

    const auto assetSources = MakeTestAssetSources();
    const auto kickAsset = assetSources[0];

    project.generators.push_back(MakeSamplerGenerator(
      2,
      "Kick sampler",
      sxp::ProjectSamplerPayloadV1 {
        .sampleAssetId = kickAsset.id,
        .samplePath = kickAsset.path,
        .rootNote = 36,
        .pitchFollow = false,
        .oneShot = true,
        .mixerChannelId = 1
      }
    ));

    project.generators.push_back(MakeOscillatorGenerator(
      3,
      "Bass oscillator",
      sxp::ProjectOscillatorPayloadV1 {
        .waveShape = 2,
        .octave = -1,
        .semitone = 0,
        .attackSeconds = 0.005f,
        .decaySeconds = 0.25f,
        .sustainLevel = 0.85f,
        .releaseSeconds = 0.08f,
        .mixerChannelId = 1
      }
    ));

    sxp::ProjectTrack track;
    track.id = 1;
    track.name = "Track 1";
    track.generatorNodeIds.push_back(2);
    track.generatorNodeIds.push_back(3);

    project.tracks.push_back(track);

    sxp::ProjectPattern pattern;
    pattern.id = 1;
    pattern.name = "Pattern 1";
    pattern.lengthBeats = 8.0;

    std::uint64_t nextNoteId = 1;

    for (int i = 0; i < 4; ++i) {
      AddNote(
        pattern,
        nextNoteId,
        60 + i,
        1.0f,
        static_cast<double>(i),
        1.0
      );
    }

    project.patterns.push_back(pattern);

    AddClip(
      project,
      1,
      track.id,
      pattern.id,
      0.0,
      4.0,
      0.0,
      false
    );

    AddClip(
      project,
      2,
      track.id,
      pattern.id,
      4.0,
      4.0,
      1.0,
      true
    );

    return project;
  }

  sxp::ProjectDocument CreateTestProject() {
    sxp::ProjectDocument project;

    project.header.projectName = "Big Synthem SXP Stress Project";
    project.header.bpm = 174.0;
    project.header.timeSigNumerator = 4;
    project.header.timeSigDenominator = 4;
    project.header.swingAmount = 0.18;
    project.header.swingSubdivisionBeats = 0.25;

    const auto assetSources = MakeTestAssetSources();

    const auto kickAsset = assetSources[0];
    const auto closedHatAsset = assetSources[1];
    const auto openHatAsset = assetSources[2];
    const auto clapAsset = assetSources[3];

    project.generators.push_back(MakeOscillatorGenerator(
      10,
      "Sub Sine",
      sxp::ProjectOscillatorPayloadV1 {
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
      sxp::ProjectOscillatorPayloadV1 {
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
      sxp::ProjectOscillatorPayloadV1 {
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
      sxp::ProjectOscillatorPayloadV1 {
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
      sxp::ProjectSamplerPayloadV1 {
        .sampleAssetId = kickAsset.id,
        .samplePath = kickAsset.path,
        .rootNote = 36,
        .pitchFollow = false,
        .oneShot = true,
        .mixerChannelId = 2
      }
    ));

    project.generators.push_back(MakeSamplerGenerator(
      21,
      "Clap Sampler",
      sxp::ProjectSamplerPayloadV1 {
        .sampleAssetId = clapAsset.id,
        .samplePath = clapAsset.path,
        .rootNote = 38,
        .pitchFollow = false,
        .oneShot = true,
        .mixerChannelId = 2
      }
    ));

    project.generators.push_back(MakeSamplerGenerator(
      22,
      "Closed Hat Sampler",
      sxp::ProjectSamplerPayloadV1 {
        .sampleAssetId = closedHatAsset.id,
        .samplePath = closedHatAsset.path,
        .rootNote = 42,
        .pitchFollow = false,
        .oneShot = true,
        .mixerChannelId = 3
      }
    ));

    project.generators.push_back(MakeSamplerGenerator(
      23,
      "Open Hat Sampler",
      sxp::ProjectSamplerPayloadV1 {
        .sampleAssetId = openHatAsset.id,
        .samplePath = openHatAsset.path,
        .rootNote = 46,
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
    drumTrack.generatorNodeIds = { 20, 21, 22, 23 };
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


  std::vector<TestAssetSource> MakeDnbAssetSources() {
    return {
      {
        MakeAssetId(2, 1),
        "Cymatics - Kick (Course).wav",
        JoinSamplePath("Cymatics - Kick (Course).wav"),
        "audio/wav"
      },
      {
        MakeAssetId(2, 2),
        "Cymatics - Snare (Smacked).wav",
        JoinSamplePath("Cymatics - Snare (Smacked).wav"),
        "audio/wav"
      },
      {
        MakeAssetId(2, 3),
        "Cymatics - Percussion (Desert).wav",
        JoinSamplePath("Cymatics - Percussion (Desert).wav"),
        "audio/wav"
      },
      {
        MakeAssetId(2, 4),
        "Cymatics - Hihat (Grant).wav",
        JoinSamplePath("Cymatics - Hihat (Grant).wav"),
        "audio/wav"
      },
      {
        MakeAssetId(2, 5),
        "Cymatics - Open Hihat (Keep).wav",
        JoinSamplePath("Cymatics - Open Hihat (Keep).wav"),
        "audio/wav"
      }
    };
  }

  void AddDnbAssets(sxp::ProjectDocument& project) {
    for (const auto& source : MakeDnbAssetSources()) {
      sxp::ProjectAsset asset;
      asset.id = source.id;
      asset.kind = sxp::AssetKind::AudioSample;
      asset.displayName = source.displayName;
      asset.originalPathHint = source.path;
      asset.mimeType = source.mimeType;
      asset.data = ReadFileBytes(source.path);

      project.assets.push_back(std::move(asset));
    }
  }

  sxp::ProjectPattern MakeDnbKickPattern(std::uint64_t& nextNoteId) {
    sxp::ProjectPattern pattern;
    pattern.id = 1;
    pattern.name = "DnB Kick - Course Step";
    pattern.lengthBeats = 16.0;

    const std::array<double, 13> kicks {
      0.0, 1.50, 2.75,
      4.0, 5.50, 6.75,
      8.0, 9.25, 10.75,
      12.0, 13.50, 14.50, 15.25
    };

    for (std::size_t i = 0; i < kicks.size(); ++i) {
      AddNote(
        pattern,
        nextNoteId,
        36,
        i % 3 == 0 ? 1.0f : 0.86f,
        kicks[i],
        0.10
      );
    }

    return pattern;
  }

  sxp::ProjectPattern MakeDnbSnarePattern(std::uint64_t& nextNoteId) {
    sxp::ProjectPattern pattern;
    pattern.id = 2;
    pattern.name = "DnB Snare - Smacked Backbeat";
    pattern.lengthBeats = 16.0;

    for (int bar = 0; bar < 4; ++bar) {
      const double barStart = static_cast<double>(bar) * 4.0;

      AddNote(pattern, nextNoteId, 38, 1.00f, barStart + 1.0, 0.10);
      AddNote(pattern, nextNoteId, 38, 0.96f, barStart + 3.0, 0.10);

      if (bar == 1 || bar == 3) {
        AddNote(pattern, nextNoteId, 38, 0.38f, barStart + 2.75, 0.06);
      }
    }

    return pattern;
  }

  sxp::ProjectPattern MakeDnbPercPattern(std::uint64_t& nextNoteId) {
    sxp::ProjectPattern pattern;
    pattern.id = 3;
    pattern.name = "DnB Perc - Desert Ghosts";
    pattern.lengthBeats = 16.0;

    struct PercHit {
      double start;
      float velocity;
    };

    const std::array<PercHit, 18> hits {
      PercHit { 0.75, 0.40f },
      PercHit { 1.75, 0.33f },
      PercHit { 2.50, 0.48f },
      PercHit { 3.50, 0.36f },

      PercHit { 4.75, 0.38f },
      PercHit { 5.75, 0.35f },
      PercHit { 6.50, 0.52f },
      PercHit { 7.25, 0.40f },
      PercHit { 7.75, 0.58f },

      PercHit { 8.75, 0.42f },
      PercHit { 9.75, 0.34f },
      PercHit { 10.50, 0.50f },
      PercHit { 11.50, 0.38f },

      PercHit { 12.75, 0.40f },
      PercHit { 13.75, 0.36f },
      PercHit { 14.50, 0.55f },
      PercHit { 15.00, 0.44f },
      PercHit { 15.75, 0.66f }
    };

    for (const auto& hit : hits) {
      AddNote(
        pattern,
        nextNoteId,
        45,
        hit.velocity,
        hit.start,
        0.08
      );
    }

    return pattern;
  }

  sxp::ProjectPattern MakeDnbClosedHatPattern(std::uint64_t& nextNoteId) {
    sxp::ProjectPattern pattern;
    pattern.id = 4;
    pattern.name = "DnB Hats - Grant 16ths";
    pattern.lengthBeats = 16.0;

    for (int step = 0; step < 64; ++step) {
      const bool downbeat = step % 16 == 0;
      const bool eighth = step % 2 == 0;
      const bool skipForBreath = step == 31 || step == 63;

      if (skipForBreath) {
        continue;
      }

      AddNote(
        pattern,
        nextNoteId,
        42,
        downbeat ? 0.70f : (eighth ? 0.48f : 0.30f),
        static_cast<double>(step) * 0.25,
        0.045
      );
    }

    return pattern;
  }

  sxp::ProjectPattern MakeDnbOpenHatPattern(std::uint64_t& nextNoteId) {
    sxp::ProjectPattern pattern;
    pattern.id = 5;
    pattern.name = "DnB Open Hats - Keep Lifts";
    pattern.lengthBeats = 16.0;

    for (int bar = 0; bar < 4; ++bar) {
      const double barStart = static_cast<double>(bar) * 4.0;

      AddNote(pattern, nextNoteId, 46, 0.58f, barStart + 0.50, 0.14);
      AddNote(pattern, nextNoteId, 46, 0.48f, barStart + 2.50, 0.14);

      if (bar == 3) {
        AddNote(pattern, nextNoteId, 46, 0.72f, barStart + 3.75, 0.12);
      }
    }

    return pattern;
  }

  sxp::ProjectPattern MakeDnbBassPattern(std::uint64_t& nextNoteId) {
    sxp::ProjectPattern pattern;
    pattern.id = 6;
    pattern.name = "DnB Bass - Sub Reese Roll";
    pattern.lengthBeats = 16.0;

    struct BassNote {
      int note;
      double start;
      double length;
      float velocity;
    };

    const std::array<BassNote, 24> notes {
      BassNote { 36, 0.00, 0.50, 0.92f },
      BassNote { 36, 0.75, 0.25, 0.70f },
      BassNote { 43, 1.50, 0.50, 0.82f },
      BassNote { 39, 2.50, 0.50, 0.84f },
      BassNote { 34, 3.50, 0.25, 0.76f },

      BassNote { 36, 4.00, 0.50, 0.94f },
      BassNote { 36, 4.75, 0.25, 0.72f },
      BassNote { 46, 5.50, 0.50, 0.78f },
      BassNote { 43, 6.50, 0.50, 0.82f },
      BassNote { 41, 7.25, 0.25, 0.70f },
      BassNote { 39, 7.75, 0.20, 0.64f },

      BassNote { 34, 8.00, 0.50, 0.90f },
      BassNote { 34, 8.75, 0.25, 0.72f },
      BassNote { 41, 9.50, 0.50, 0.82f },
      BassNote { 38, 10.50, 0.50, 0.78f },
      BassNote { 41, 11.50, 0.25, 0.72f },

      BassNote { 36, 12.00, 0.50, 0.94f },
      BassNote { 36, 12.75, 0.25, 0.72f },
      BassNote { 43, 13.50, 0.35, 0.80f },
      BassNote { 46, 14.00, 0.25, 0.74f },
      BassNote { 43, 14.50, 0.25, 0.74f },
      BassNote { 39, 15.00, 0.20, 0.68f },
      BassNote { 34, 15.25, 0.20, 0.66f },
      BassNote { 36, 15.50, 0.20, 0.70f }
    };

    for (const auto& note : notes) {
      AddNote(
        pattern,
        nextNoteId,
        note.note,
        note.velocity,
        note.start,
        note.length
      );
    }

    return pattern;
  }

  sxp::ProjectPattern MakeDnbFillPattern(std::uint64_t& nextNoteId) {
    sxp::ProjectPattern pattern;
    pattern.id = 7;
    pattern.name = "DnB Fill - Desert Smack";
    pattern.lengthBeats = 4.0;

    const std::array<int, 16> notes {
      45, 42, 38, 42,
      45, 42, 38, 45,
      42, 45, 38, 42,
      45, 38, 45, 38
    };

    for (std::size_t step = 0; step < notes.size(); ++step) {
      AddNote(
        pattern,
        nextNoteId,
        notes[step],
        0.36f + static_cast<float>(step) * 0.035f,
        static_cast<double>(step) * 0.25,
        0.06
      );
    }

    AddNote(pattern, nextNoteId, 46, 0.80f, 3.50, 0.12);
    AddNote(pattern, nextNoteId, 36, 1.00f, 3.75, 0.10);

    return pattern;
  }

  sxp::ProjectDocument CreateDnbProject() {
    sxp::ProjectDocument project;

    project.header.projectName = "Embedded Sample DnB Break";
    project.header.bpm = 174.0;
    project.header.timeSigNumerator = 4;
    project.header.timeSigDenominator = 4;
    project.header.swingAmount = 0.08;
    project.header.swingSubdivisionBeats = 0.25;

    const auto assetSources = MakeDnbAssetSources();

    const auto kickAsset = assetSources[0];
    const auto snareAsset = assetSources[1];
    const auto percAsset = assetSources[2];
    const auto closedHatAsset = assetSources[3];
    const auto openHatAsset = assetSources[4];

    project.generators.push_back(MakeSamplerGenerator(
      10,
      "DnB Kick Course",
      sxp::ProjectSamplerPayloadV1 {
        .sampleAssetId = kickAsset.id,
        .samplePath = kickAsset.path,
        .rootNote = 36,
        .pitchFollow = false,
        .oneShot = true,
        .mixerChannelId = 1
      }
    ));

    project.generators.push_back(MakeSamplerGenerator(
      11,
      "DnB Snare Smacked",
      sxp::ProjectSamplerPayloadV1 {
        .sampleAssetId = snareAsset.id,
        .samplePath = snareAsset.path,
        .rootNote = 38,
        .pitchFollow = false,
        .oneShot = true,
        .mixerChannelId = 2
      }
    ));

    project.generators.push_back(MakeSamplerGenerator(
      12,
      "DnB Perc Desert",
      sxp::ProjectSamplerPayloadV1 {
        .sampleAssetId = percAsset.id,
        .samplePath = percAsset.path,
        .rootNote = 45,
        .pitchFollow = false,
        .oneShot = true,
        .mixerChannelId = 2
      }
    ));

    project.generators.push_back(MakeSamplerGenerator(
      13,
      "DnB Hihat Grant",
      sxp::ProjectSamplerPayloadV1 {
        .sampleAssetId = closedHatAsset.id,
        .samplePath = closedHatAsset.path,
        .rootNote = 42,
        .pitchFollow = false,
        .oneShot = true,
        .mixerChannelId = 3
      }
    ));

    project.generators.push_back(MakeSamplerGenerator(
      14,
      "DnB Open Hihat Keep",
      sxp::ProjectSamplerPayloadV1 {
        .sampleAssetId = openHatAsset.id,
        .samplePath = openHatAsset.path,
        .rootNote = 46,
        .pitchFollow = false,
        .oneShot = true,
        .mixerChannelId = 3
      }
    ));

    project.generators.push_back(MakeOscillatorGenerator(
      20,
      "DnB Sub Sine",
      sxp::ProjectOscillatorPayloadV1 {
        .waveShape = 0,
        .octave = -2,
        .semitone = 0,
        .attackSeconds = 0.003f,
        .decaySeconds = 0.180f,
        .sustainLevel = 0.88f,
        .releaseSeconds = 0.070f,
        .mixerChannelId = 4
      }
    ));

    project.generators.push_back(MakeOscillatorGenerator(
      21,
      "DnB Saw Bite",
      sxp::ProjectOscillatorPayloadV1 {
        .waveShape = 2,
        .octave = -1,
        .semitone = 0,
        .attackSeconds = 0.006f,
        .decaySeconds = 0.260f,
        .sustainLevel = 0.62f,
        .releaseSeconds = 0.110f,
        .mixerChannelId = 4
      }
    ));

    sxp::ProjectTrack kickTrack;
    kickTrack.id = 1;
    kickTrack.name = "Kick";
    kickTrack.generatorNodeIds = { 10 };
    project.tracks.push_back(kickTrack);

    sxp::ProjectTrack snareTrack;
    snareTrack.id = 2;
    snareTrack.name = "Snare";
    snareTrack.generatorNodeIds = { 11 };
    project.tracks.push_back(snareTrack);

    sxp::ProjectTrack percTrack;
    percTrack.id = 3;
    percTrack.name = "Perc Desert";
    percTrack.generatorNodeIds = { 12 };
    project.tracks.push_back(percTrack);

    sxp::ProjectTrack hatTrack;
    hatTrack.id = 4;
    hatTrack.name = "Closed Hats";
    hatTrack.generatorNodeIds = { 13 };
    project.tracks.push_back(hatTrack);

    sxp::ProjectTrack openHatTrack;
    openHatTrack.id = 5;
    openHatTrack.name = "Open Hat";
    openHatTrack.generatorNodeIds = { 14 };
    project.tracks.push_back(openHatTrack);

    sxp::ProjectTrack bassTrack;
    bassTrack.id = 6;
    bassTrack.name = "Bass Stack";
    bassTrack.generatorNodeIds = { 20, 21 };
    project.tracks.push_back(bassTrack);

    std::uint64_t nextNoteId = 1;

    project.patterns.push_back(MakeDnbKickPattern(nextNoteId));
    project.patterns.push_back(MakeDnbSnarePattern(nextNoteId));
    project.patterns.push_back(MakeDnbPercPattern(nextNoteId));
    project.patterns.push_back(MakeDnbClosedHatPattern(nextNoteId));
    project.patterns.push_back(MakeDnbOpenHatPattern(nextNoteId));
    project.patterns.push_back(MakeDnbBassPattern(nextNoteId));
    project.patterns.push_back(MakeDnbFillPattern(nextNoteId));

    // Intro: hats and little percussion hints.
    AddClip(project, 1, 4, 4,  0.0, 16.0, 0.0, false);
    AddClip(project, 2, 5, 5,  8.0,  8.0, 0.0, false);
    AddClip(project, 3, 3, 3, 12.0,  4.0, 12.0, false);

    // First drop.
    AddClip(project, 4, 1, 1, 16.0, 16.0, 0.0, false);
    AddClip(project, 5, 2, 2, 16.0, 16.0, 0.0, false);
    AddClip(project, 6, 3, 3, 16.0, 16.0, 0.0, false);
    AddClip(project, 7, 4, 4, 16.0, 16.0, 0.0, false);
    AddClip(project, 8, 5, 5, 16.0, 16.0, 0.0, false);
    AddClip(project, 9, 6, 6, 16.0, 16.0, 0.0, false);

    // Variation with a tiny breath before the fill.
    AddClip(project, 10, 1, 1, 32.0, 12.0, 0.0, false);
    AddClip(project, 11, 2, 2, 32.0, 12.0, 0.0, false);
    AddClip(project, 12, 3, 3, 32.0, 12.0, 0.0, false);
    AddClip(project, 13, 4, 4, 32.0, 16.0, 0.0, false);
    AddClip(project, 14, 5, 5, 32.0, 16.0, 0.0, false);
    AddClip(project, 15, 6, 6, 32.0, 16.0, 0.0, false);

    // Four-beat fill.
    AddClip(project, 16, 2, 7, 44.0,  4.0, 0.0, false);
    AddClip(project, 17, 3, 7, 44.0,  4.0, 0.0, false);

    // Final 16-beat loop.
    AddClip(project, 18, 1, 1, 48.0, 16.0, 0.0, false);
    AddClip(project, 19, 2, 2, 48.0, 16.0, 0.0, false);
    AddClip(project, 20, 3, 3, 48.0, 16.0, 0.0, false);
    AddClip(project, 21, 4, 4, 48.0, 16.0, 0.0, false);
    AddClip(project, 22, 5, 5, 48.0, 16.0, 0.0, false);
    AddClip(project, 23, 6, 6, 48.0, 16.0, 0.0, false);

    return project;
  }


  std::vector<TestAssetSource> MakeUkgAssetSources() {
    return {
      {
        MakeAssetId(3, 1),
        "Cymatics - Clap (Present).wav",
        JoinSamplePath("Cymatics - Clap (Present).wav"),
        "audio/wav"
      },
      {
        MakeAssetId(3, 2),
        "Cymatics - Hihat (Evidence).wav",
        JoinSamplePath("Cymatics - Hihat (Evidence).wav"),
        "audio/wav"
      },
      {
        MakeAssetId(3, 3),
        "Cymatics - Kick (Flare).wav",
        JoinSamplePath("Cymatics - Kick (Flare).wav"),
        "audio/wav"
      },
      {
        MakeAssetId(3, 4),
        "Cymatics - Open Hihat (Active).wav",
        JoinSamplePath("Cymatics - Open Hihat (Active).wav"),
        "audio/wav"
      },
      {
        MakeAssetId(3, 5),
        "Cymatics - Percussion (Desert).wav",
        JoinSamplePath("Cymatics - Percussion (Desert).wav"),
        "audio/wav"
      },
      {
        MakeAssetId(3, 6),
        "Cymatics - Snap (Free).wav",
        JoinSamplePath("Cymatics - Snap (Free).wav"),
        "audio/wav"
      },
      {
        MakeAssetId(3, 7),
        "Cymatics - Snare (Smacked).wav",
        JoinSamplePath("Cymatics - Snare (Smacked).wav"),
        "audio/wav"
      }
    };
  }

  void AddUkgAssets(sxp::ProjectDocument& project) {
    for (const auto& source : MakeUkgAssetSources()) {
      sxp::ProjectAsset asset;
      asset.id = source.id;
      asset.kind = sxp::AssetKind::AudioSample;
      asset.displayName = source.displayName;
      asset.originalPathHint = source.path;
      asset.mimeType = source.mimeType;
      asset.data = ReadFileBytes(source.path);

      project.assets.push_back(std::move(asset));
    }
  }

  sxp::ProjectPattern MakeUkgKickPattern(std::uint64_t& nextNoteId) {
    sxp::ProjectPattern pattern;
    pattern.id = 1;
    pattern.name = "UKG House Kick - Flare Pump";
    pattern.lengthBeats = 16.0;

    // Housey 4-on-the-floor spine, with little UKG skip kicks around it.
    for (int beat = 0; beat < 16; ++beat) {
      AddNote(
        pattern,
        nextNoteId,
        36,
        beat % 4 == 0 ? 1.0f : 0.92f,
        static_cast<double>(beat),
        0.10
      );
    }

    const std::array<double, 12> ghosts {
      1.50, 2.75, 5.50, 6.75,
      9.50, 10.75, 13.25, 14.50,
      14.75, 15.25, 15.50, 15.75
    };

    for (std::size_t i = 0; i < ghosts.size(); ++i) {
      AddNote(
        pattern,
        nextNoteId,
        36,
        i >= 8 ? 0.46f : 0.62f,
        ghosts[i],
        0.07
      );
    }

    return pattern;
  }

  sxp::ProjectPattern MakeUkgKickBuildPattern(std::uint64_t& nextNoteId) {
    sxp::ProjectPattern pattern;
    pattern.id = 2;
    pattern.name = "UKG House Kick - Build Pulse";
    pattern.lengthBeats = 16.0;

    for (int beat = 0; beat < 16; ++beat) {
      AddNote(
        pattern,
        nextNoteId,
        36,
        beat % 4 == 0 ? 0.86f : 0.62f,
        static_cast<double>(beat),
        0.08
      );

      if (beat >= 8 && beat % 2 == 1) {
        AddNote(pattern, nextNoteId, 36, 0.44f, static_cast<double>(beat) + 0.50, 0.06);
      }

      if (beat >= 12) {
        AddNote(pattern, nextNoteId, 36, 0.34f, static_cast<double>(beat) + 0.75, 0.05);
      }
    }

    return pattern;
  }

  sxp::ProjectPattern MakeUkgSnarePattern(std::uint64_t& nextNoteId) {
    sxp::ProjectPattern pattern;
    pattern.id = 3;
    pattern.name = "UKG Snare - Smacked Garage Backbeat";
    pattern.lengthBeats = 16.0;

    for (int bar = 0; bar < 4; ++bar) {
      const double barStart = static_cast<double>(bar) * 4.0;

      AddNote(pattern, nextNoteId, 38, 0.88f, barStart + 1.0, 0.10);
      AddNote(pattern, nextNoteId, 38, 0.92f, barStart + 3.0, 0.10);

      AddNote(pattern, nextNoteId, 38, 0.32f, barStart + 2.75, 0.06);

      if (bar % 2 == 1) {
        AddNote(pattern, nextNoteId, 38, 0.38f, barStart + 3.50, 0.06);
      }
    }

    return pattern;
  }

  sxp::ProjectPattern MakeUkgClapPattern(std::uint64_t& nextNoteId) {
    sxp::ProjectPattern pattern;
    pattern.id = 4;
    pattern.name = "UKG Clap - Present House Layer";
    pattern.lengthBeats = 16.0;

    for (int bar = 0; bar < 4; ++bar) {
      const double barStart = static_cast<double>(bar) * 4.0;

      AddNote(pattern, nextNoteId, 39, 0.54f, barStart + 1.0, 0.08);
      AddNote(pattern, nextNoteId, 39, 0.72f, barStart + 3.0, 0.08);

      if (bar == 1 || bar == 3) {
        AddNote(pattern, nextNoteId, 39, 0.34f, barStart + 2.50, 0.06);
      }
    }

    return pattern;
  }

  sxp::ProjectPattern MakeUkgSnapPattern(std::uint64_t& nextNoteId) {
    sxp::ProjectPattern pattern;
    pattern.id = 5;
    pattern.name = "UKG Snap - Free Flicks";
    pattern.lengthBeats = 16.0;

    for (int bar = 0; bar < 4; ++bar) {
      const double barStart = static_cast<double>(bar) * 4.0;

      AddNote(pattern, nextNoteId, 37, 0.40f, barStart + 0.75, 0.05);
      AddNote(pattern, nextNoteId, 37, 0.54f, barStart + 2.75, 0.05);

      if (bar == 3) {
        AddNote(pattern, nextNoteId, 37, 0.42f, barStart + 3.25, 0.05);
      }
    }

    return pattern;
  }

  sxp::ProjectPattern MakeUkgPercPattern(std::uint64_t& nextNoteId) {
    sxp::ProjectPattern pattern;
    pattern.id = 6;
    pattern.name = "UKG Perc - Desert House Shuffle";
    pattern.lengthBeats = 16.0;

    const std::array<double, 16> hits {
      0.50, 1.75, 2.25, 3.50,
      4.50, 5.75, 6.25, 7.50,
      8.50, 9.75, 10.25, 11.50,
      12.50, 13.75, 14.25, 15.50
    };

    for (std::size_t i = 0; i < hits.size(); ++i) {
      AddNote(
        pattern,
        nextNoteId,
        45,
        i % 4 == 2 ? 0.54f : 0.38f,
        hits[i],
        0.07
      );
    }

    return pattern;
  }

  sxp::ProjectPattern MakeUkgHatSparsePattern(std::uint64_t& nextNoteId) {
    sxp::ProjectPattern pattern;
    pattern.id = 7;
    pattern.name = "UKG Hats - Sparse Evidence";
    pattern.lengthBeats = 16.0;

    for (int beat = 0; beat < 16; ++beat) {
      AddNote(
        pattern,
        nextNoteId,
        42,
        beat % 4 == 2 ? 0.62f : 0.46f,
        static_cast<double>(beat) + 0.50,
        0.06
      );
    }

    return pattern;
  }

  sxp::ProjectPattern MakeUkgHatFullPattern(std::uint64_t& nextNoteId) {
    sxp::ProjectPattern pattern;
    pattern.id = 8;
    pattern.name = "UKG Hats - Housey Shuffler";
    pattern.lengthBeats = 16.0;

    for (int step = 0; step < 64; ++step) {
      const bool offbeat = step % 4 == 2;
      const bool sixteenth = step % 2 == 1;

      float velocity = 0.28f;

      if (offbeat) {
        velocity = 0.68f;
      } else if (sixteenth) {
        velocity = 0.36f;
      } else if (step % 8 == 0) {
        velocity = 0.44f;
      }

      if (step == 31 || step == 63) {
        continue;
      }

      AddNote(
        pattern,
        nextNoteId,
        42,
        velocity,
        static_cast<double>(step) * 0.25,
        0.045
      );
    }

    return pattern;
  }

  sxp::ProjectPattern MakeUkgOpenHatPattern(std::uint64_t& nextNoteId) {
    sxp::ProjectPattern pattern;
    pattern.id = 9;
    pattern.name = "UKG Open Hat - Active House Lift";
    pattern.lengthBeats = 16.0;

    for (int bar = 0; bar < 4; ++bar) {
      const double barStart = static_cast<double>(bar) * 4.0;

      AddNote(pattern, nextNoteId, 46, 0.56f, barStart + 0.50, 0.12);
      AddNote(pattern, nextNoteId, 46, 0.64f, barStart + 2.50, 0.12);

      if (bar == 3) {
        AddNote(pattern, nextNoteId, 46, 0.78f, barStart + 3.75, 0.10);
      }
    }

    return pattern;
  }

  sxp::ProjectPattern MakeUkgBuildHatPattern(std::uint64_t& nextNoteId) {
    sxp::ProjectPattern pattern;
    pattern.id = 10;
    pattern.name = "UKG Hats - Build Shaker Rush";
    pattern.lengthBeats = 16.0;

    for (int step = 0; step < 64; ++step) {
      float velocity = 0.24f + static_cast<float>(step) * 0.006f;

      if (step >= 48 && step % 2 == 1) {
        velocity += 0.12f;
      }

      AddNote(
        pattern,
        nextNoteId,
        42,
        std::min(velocity, 0.76f),
        static_cast<double>(step) * 0.25,
        0.04
      );
    }

    return pattern;
  }

  sxp::ProjectPattern MakeUkgBassPatternA(std::uint64_t& nextNoteId) {
    sxp::ProjectPattern pattern;
    pattern.id = 11;
    pattern.name = "UKG Bass - Dirty Offbeat A";
    pattern.lengthBeats = 16.0;

    struct BassNote {
      int note;
      double start;
      double length;
      float velocity;
    };

    const std::array<BassNote, 28> notes {
      BassNote { 36, 0.50, 0.38, 0.94f },
      BassNote { 36, 1.50, 0.25, 0.72f },
      BassNote { 43, 2.00, 0.32, 0.82f },
      BassNote { 39, 2.75, 0.22, 0.74f },
      BassNote { 34, 3.50, 0.32, 0.84f },

      BassNote { 36, 4.50, 0.38, 0.96f },
      BassNote { 48, 5.00, 0.14, 0.42f },
      BassNote { 43, 5.50, 0.28, 0.82f },
      BassNote { 46, 6.25, 0.22, 0.70f },
      BassNote { 43, 6.75, 0.20, 0.66f },
      BassNote { 39, 7.50, 0.32, 0.80f },

      BassNote { 34, 8.50, 0.38, 0.92f },
      BassNote { 34, 9.50, 0.24, 0.72f },
      BassNote { 41, 10.00, 0.32, 0.82f },
      BassNote { 38, 10.75, 0.22, 0.74f },
      BassNote { 46, 11.50, 0.26, 0.76f },

      BassNote { 36, 12.50, 0.38, 0.96f },
      BassNote { 48, 13.00, 0.14, 0.48f },
      BassNote { 43, 13.50, 0.24, 0.82f },
      BassNote { 46, 14.00, 0.18, 0.70f },
      BassNote { 43, 14.25, 0.16, 0.64f },
      BassNote { 39, 14.50, 0.18, 0.66f },
      BassNote { 34, 14.75, 0.16, 0.64f },
      BassNote { 36, 15.00, 0.16, 0.70f },
      BassNote { 39, 15.25, 0.14, 0.62f },
      BassNote { 43, 15.50, 0.14, 0.60f },
      BassNote { 46, 15.75, 0.12, 0.58f },
      BassNote { 48, 15.875, 0.08, 0.52f }
    };

    for (const auto& note : notes) {
      AddNote(pattern, nextNoteId, note.note, note.velocity, note.start, note.length);
    }

    return pattern;
  }

  sxp::ProjectPattern MakeUkgBassPatternB(std::uint64_t& nextNoteId) {
    sxp::ProjectPattern pattern;
    pattern.id = 12;
    pattern.name = "UKG Bass - Dirty Offbeat B";
    pattern.lengthBeats = 16.0;

    struct BassNote {
      int note;
      double start;
      double length;
      float velocity;
    };

    const std::array<BassNote, 34> notes {
      BassNote { 36, 0.50, 0.34, 0.96f },
      BassNote { 43, 1.00, 0.18, 0.64f },
      BassNote { 36, 1.50, 0.22, 0.72f },
      BassNote { 48, 1.75, 0.12, 0.48f },
      BassNote { 43, 2.25, 0.26, 0.82f },
      BassNote { 39, 2.75, 0.20, 0.74f },
      BassNote { 34, 3.50, 0.28, 0.84f },

      BassNote { 36, 4.50, 0.34, 0.96f },
      BassNote { 36, 5.25, 0.18, 0.70f },
      BassNote { 43, 5.50, 0.22, 0.80f },
      BassNote { 46, 6.00, 0.18, 0.72f },
      BassNote { 50, 6.25, 0.12, 0.50f },
      BassNote { 43, 6.50, 0.18, 0.68f },
      BassNote { 39, 7.25, 0.18, 0.72f },
      BassNote { 34, 7.50, 0.22, 0.74f },

      BassNote { 34, 8.50, 0.34, 0.92f },
      BassNote { 41, 9.00, 0.18, 0.62f },
      BassNote { 34, 9.50, 0.22, 0.72f },
      BassNote { 46, 9.75, 0.12, 0.48f },
      BassNote { 41, 10.25, 0.26, 0.82f },
      BassNote { 38, 10.75, 0.20, 0.74f },
      BassNote { 46, 11.50, 0.24, 0.78f },

      BassNote { 36, 12.50, 0.34, 0.96f },
      BassNote { 43, 13.00, 0.18, 0.68f },
      BassNote { 36, 13.50, 0.20, 0.74f },
      BassNote { 48, 13.75, 0.12, 0.50f },
      BassNote { 43, 14.00, 0.18, 0.72f },
      BassNote { 46, 14.25, 0.16, 0.70f },
      BassNote { 43, 14.50, 0.16, 0.68f },
      BassNote { 39, 14.75, 0.14, 0.66f },
      BassNote { 34, 15.00, 0.14, 0.64f },
      BassNote { 36, 15.25, 0.14, 0.66f },
      BassNote { 43, 15.50, 0.12, 0.62f },
      BassNote { 48, 15.75, 0.10, 0.56f }
    };

    for (const auto& note : notes) {
      AddNote(pattern, nextNoteId, note.note, note.velocity, note.start, note.length);
    }

    return pattern;
  }

  sxp::ProjectPattern MakeUkgBassPatternC(std::uint64_t& nextNoteId) {
    sxp::ProjectPattern pattern;
    pattern.id = 13;
    pattern.name = "UKG Bass - Distortion Food C";
    pattern.lengthBeats = 16.0;

    struct BassNote {
      int note;
      double start;
      double length;
      float velocity;
    };

    // More mid notes and shorter stabs so a distortion effect has something nasty to chew on.
    const std::array<BassNote, 40> notes {
      BassNote { 36, 0.50, 0.28, 1.00f },
      BassNote { 48, 0.875, 0.10, 0.58f },
      BassNote { 43, 1.25, 0.18, 0.74f },
      BassNote { 36, 1.50, 0.18, 0.76f },
      BassNote { 51, 1.75, 0.10, 0.54f },
      BassNote { 43, 2.00, 0.22, 0.82f },
      BassNote { 46, 2.50, 0.16, 0.70f },
      BassNote { 39, 2.75, 0.16, 0.74f },
      BassNote { 34, 3.50, 0.24, 0.86f },
      BassNote { 46, 3.75, 0.10, 0.54f },

      BassNote { 36, 4.50, 0.28, 1.00f },
      BassNote { 48, 4.875, 0.10, 0.60f },
      BassNote { 43, 5.25, 0.18, 0.76f },
      BassNote { 36, 5.50, 0.18, 0.76f },
      BassNote { 50, 5.75, 0.10, 0.56f },
      BassNote { 43, 6.00, 0.18, 0.78f },
      BassNote { 46, 6.25, 0.14, 0.72f },
      BassNote { 50, 6.50, 0.10, 0.56f },
      BassNote { 43, 6.75, 0.14, 0.68f },
      BassNote { 39, 7.50, 0.22, 0.82f },

      BassNote { 34, 8.50, 0.28, 0.96f },
      BassNote { 46, 8.875, 0.10, 0.58f },
      BassNote { 41, 9.25, 0.18, 0.76f },
      BassNote { 34, 9.50, 0.18, 0.74f },
      BassNote { 53, 9.75, 0.10, 0.52f },
      BassNote { 41, 10.00, 0.20, 0.82f },
      BassNote { 38, 10.50, 0.16, 0.72f },
      BassNote { 46, 10.75, 0.16, 0.76f },
      BassNote { 50, 11.25, 0.10, 0.58f },
      BassNote { 46, 11.50, 0.20, 0.84f },

      BassNote { 36, 12.50, 0.28, 1.00f },
      BassNote { 48, 12.875, 0.10, 0.62f },
      BassNote { 43, 13.25, 0.18, 0.78f },
      BassNote { 36, 13.50, 0.18, 0.78f },
      BassNote { 51, 13.75, 0.10, 0.58f },
      BassNote { 43, 14.00, 0.14, 0.76f },
      BassNote { 46, 14.25, 0.12, 0.74f },
      BassNote { 50, 14.50, 0.10, 0.64f },
      BassNote { 51, 14.75, 0.10, 0.62f },
      BassNote { 55, 15.00, 0.22, 0.70f }
    };

    for (const auto& note : notes) {
      AddNote(pattern, nextNoteId, note.note, note.velocity, note.start, note.length);
    }

    return pattern;
  }

  sxp::ProjectPattern MakeUkgChordPattern(std::uint64_t& nextNoteId) {
    sxp::ProjectPattern pattern;
    pattern.id = 14;
    pattern.name = "UKG Chords - House Stabs";
    pattern.lengthBeats = 16.0;

    const std::array<std::array<int, 4>, 4> chords {
      std::array<int, 4> { 60, 63, 67, 70 },
      std::array<int, 4> { 58, 62, 65, 69 },
      std::array<int, 4> { 55, 58, 62, 65 },
      std::array<int, 4> { 63, 67, 70, 74 }
    };

    const std::array<double, 4> stabTimes { 0.50, 1.50, 2.50, 3.50 };

    for (std::size_t bar = 0; bar < chords.size(); ++bar) {
      const double barStart = static_cast<double>(bar) * 4.0;

      for (double offset : stabTimes) {
        for (int note : chords[bar]) {
          AddNote(
            pattern,
            nextNoteId,
            note,
            offset == 0.50 ? 0.54f : 0.40f,
            barStart + offset,
            0.16
          );
        }
      }
    }

    return pattern;
  }

  sxp::ProjectPattern MakeUkgPluckPattern(std::uint64_t& nextNoteId) {
    sxp::ProjectPattern pattern;
    pattern.id = 15;
    pattern.name = "UKG Pluck - Garage Hook";
    pattern.lengthBeats = 16.0;

    const std::array<int, 16> notes {
      72, 75, 70, 72,
      74, 77, 72, 70,
      67, 70, 72, 75,
      74, 72, 70, 67
    };

    for (std::size_t i = 0; i < notes.size(); ++i) {
      AddNote(
        pattern,
        nextNoteId,
        notes[i],
        i % 4 == 0 ? 0.66f : 0.48f,
        static_cast<double>(i),
        i % 4 == 3 ? 0.34 : 0.18
      );
    }

    return pattern;
  }

  sxp::ProjectPattern MakeUkgSnareBuildPattern(std::uint64_t& nextNoteId) {
    sxp::ProjectPattern pattern;
    pattern.id = 16;
    pattern.name = "UKG Snare - Build Roll";
    pattern.lengthBeats = 16.0;

    for (int bar = 0; bar < 4; ++bar) {
      const double barStart = static_cast<double>(bar) * 4.0;
      const int steps = bar < 2 ? 4 : 8;
      const double spacing = 4.0 / static_cast<double>(steps);

      for (int step = 0; step < steps; ++step) {
        AddNote(
          pattern,
          nextNoteId,
          38,
          0.32f + static_cast<float>(bar) * 0.12f,
          barStart + static_cast<double>(step) * spacing,
          0.05
        );
      }
    }

    return pattern;
  }

  sxp::ProjectPattern MakeUkgFillPattern(std::uint64_t& nextNoteId) {
    sxp::ProjectPattern pattern;
    pattern.id = 17;
    pattern.name = "UKG Fill - Dirty Turnaround";
    pattern.lengthBeats = 4.0;

    const std::array<int, 16> notes {
      45, 42, 37, 42,
      38, 42, 45, 39,
      42, 45, 38, 42,
      39, 38, 45, 36
    };

    for (std::size_t step = 0; step < notes.size(); ++step) {
      AddNote(
        pattern,
        nextNoteId,
        notes[step],
        0.36f + static_cast<float>(step) * 0.035f,
        static_cast<double>(step) * 0.25,
        0.055
      );
    }

    return pattern;
  }

  sxp::ProjectDocument CreateUkgProject() {
    sxp::ProjectDocument project;

    project.header.projectName = "Embedded Sample UKG House Dirty Bass";
    project.header.bpm = 130.0;
    project.header.timeSigNumerator = 4;
    project.header.timeSigDenominator = 4;
    project.header.swingAmount = 0.30;
    project.header.swingSubdivisionBeats = 0.25;

    const auto assetSources = MakeUkgAssetSources();

    const auto clapAsset = assetSources[0];
    const auto closedHatAsset = assetSources[1];
    const auto kickAsset = assetSources[2];
    const auto openHatAsset = assetSources[3];
    const auto percAsset = assetSources[4];
    const auto snapAsset = assetSources[5];
    const auto snareAsset = assetSources[6];

    project.generators.push_back(MakeSamplerGenerator(
      10,
      "UKG Kick Flare",
      sxp::ProjectSamplerPayloadV1 {
        .sampleAssetId = kickAsset.id,
        .samplePath = kickAsset.path,
        .rootNote = 36,
        .pitchFollow = false,
        .oneShot = true,
        .mixerChannelId = 1
      }
    ));

    project.generators.push_back(MakeSamplerGenerator(
      11,
      "UKG Snare Smacked",
      sxp::ProjectSamplerPayloadV1 {
        .sampleAssetId = snareAsset.id,
        .samplePath = snareAsset.path,
        .rootNote = 38,
        .pitchFollow = false,
        .oneShot = true,
        .mixerChannelId = 2
      }
    ));

    project.generators.push_back(MakeSamplerGenerator(
      12,
      "UKG Clap Present",
      sxp::ProjectSamplerPayloadV1 {
        .sampleAssetId = clapAsset.id,
        .samplePath = clapAsset.path,
        .rootNote = 39,
        .pitchFollow = false,
        .oneShot = true,
        .mixerChannelId = 2
      }
    ));

    project.generators.push_back(MakeSamplerGenerator(
      13,
      "UKG Snap Free",
      sxp::ProjectSamplerPayloadV1 {
        .sampleAssetId = snapAsset.id,
        .samplePath = snapAsset.path,
        .rootNote = 37,
        .pitchFollow = false,
        .oneShot = true,
        .mixerChannelId = 2
      }
    ));

    project.generators.push_back(MakeSamplerGenerator(
      14,
      "UKG Perc Desert",
      sxp::ProjectSamplerPayloadV1 {
        .sampleAssetId = percAsset.id,
        .samplePath = percAsset.path,
        .rootNote = 45,
        .pitchFollow = false,
        .oneShot = true,
        .mixerChannelId = 3
      }
    ));

    project.generators.push_back(MakeSamplerGenerator(
      15,
      "UKG Hihat Evidence",
      sxp::ProjectSamplerPayloadV1 {
        .sampleAssetId = closedHatAsset.id,
        .samplePath = closedHatAsset.path,
        .rootNote = 42,
        .pitchFollow = false,
        .oneShot = true,
        .mixerChannelId = 4
      }
    ));

    project.generators.push_back(MakeSamplerGenerator(
      16,
      "UKG Open Hihat Active",
      sxp::ProjectSamplerPayloadV1 {
        .sampleAssetId = openHatAsset.id,
        .samplePath = openHatAsset.path,
        .rootNote = 46,
        .pitchFollow = false,
        .oneShot = true,
        .mixerChannelId = 4
      }
    ));

    project.generators.push_back(MakeOscillatorGenerator(
      20,
      "UKG Bass Sub Weight",
      sxp::ProjectOscillatorPayloadV1 {
        .waveShape = 0,
        .octave = -2,
        .semitone = 0,
        .attackSeconds = 0.002f,
        .decaySeconds = 0.140f,
        .sustainLevel = 0.90f,
        .releaseSeconds = 0.055f,
        .mixerChannelId = 5
      }
    ));

    project.generators.push_back(MakeOscillatorGenerator(
      21,
      "UKG Dirty Saw Bass",
      sxp::ProjectOscillatorPayloadV1 {
        .waveShape = 2,
        .octave = -1,
        .semitone = 0,
        .attackSeconds = 0.002f,
        .decaySeconds = 0.110f,
        .sustainLevel = 0.78f,
        .releaseSeconds = 0.045f,
        .mixerChannelId = 5
      }
    ));

    project.generators.push_back(MakeOscillatorGenerator(
      22,
      "UKG Square Bite Bass",
      sxp::ProjectOscillatorPayloadV1 {
        .waveShape = 1,
        .octave = -1,
        .semitone = 12,
        .attackSeconds = 0.001f,
        .decaySeconds = 0.075f,
        .sustainLevel = 0.46f,
        .releaseSeconds = 0.035f,
        .mixerChannelId = 5
      }
    ));

    project.generators.push_back(MakeOscillatorGenerator(
      23,
      "UKG Chord Stab",
      sxp::ProjectOscillatorPayloadV1 {
        .waveShape = 1,
        .octave = 0,
        .semitone = 0,
        .attackSeconds = 0.002f,
        .decaySeconds = 0.120f,
        .sustainLevel = 0.24f,
        .releaseSeconds = 0.140f,
        .mixerChannelId = 6
      }
    ));

    project.generators.push_back(MakeOscillatorGenerator(
      24,
      "UKG Pluck Hook",
      sxp::ProjectOscillatorPayloadV1 {
        .waveShape = 3,
        .octave = 1,
        .semitone = 0,
        .attackSeconds = 0.001f,
        .decaySeconds = 0.080f,
        .sustainLevel = 0.18f,
        .releaseSeconds = 0.120f,
        .mixerChannelId = 7
      }
    ));

    sxp::ProjectTrack kickTrack;
    kickTrack.id = 1;
    kickTrack.name = "Kick";
    kickTrack.generatorNodeIds = { 10 };
    project.tracks.push_back(kickTrack);

    sxp::ProjectTrack snareTrack;
    snareTrack.id = 2;
    snareTrack.name = "Snare";
    snareTrack.generatorNodeIds = { 11 };
    project.tracks.push_back(snareTrack);

    sxp::ProjectTrack clapTrack;
    clapTrack.id = 3;
    clapTrack.name = "Clap";
    clapTrack.generatorNodeIds = { 12 };
    project.tracks.push_back(clapTrack);

    sxp::ProjectTrack snapTrack;
    snapTrack.id = 4;
    snapTrack.name = "Snap";
    snapTrack.generatorNodeIds = { 13 };
    project.tracks.push_back(snapTrack);

    sxp::ProjectTrack percTrack;
    percTrack.id = 5;
    percTrack.name = "Perc Desert";
    percTrack.generatorNodeIds = { 14 };
    project.tracks.push_back(percTrack);

    sxp::ProjectTrack hatTrack;
    hatTrack.id = 6;
    hatTrack.name = "Closed Hats";
    hatTrack.generatorNodeIds = { 15 };
    project.tracks.push_back(hatTrack);

    sxp::ProjectTrack openHatTrack;
    openHatTrack.id = 7;
    openHatTrack.name = "Open Hat";
    openHatTrack.generatorNodeIds = { 16 };
    project.tracks.push_back(openHatTrack);

    sxp::ProjectTrack bassTrack;
    bassTrack.id = 8;
    bassTrack.name = "Dirty Bass Stack";
    bassTrack.generatorNodeIds = { 20, 21, 22 };
    project.tracks.push_back(bassTrack);

    sxp::ProjectTrack chordTrack;
    chordTrack.id = 9;
    chordTrack.name = "House Chord Stabs";
    chordTrack.generatorNodeIds = { 23 };
    project.tracks.push_back(chordTrack);

    sxp::ProjectTrack pluckTrack;
    pluckTrack.id = 10;
    pluckTrack.name = "Pluck Hook";
    pluckTrack.generatorNodeIds = { 24 };
    project.tracks.push_back(pluckTrack);

    std::uint64_t nextNoteId = 1;

    project.patterns.push_back(MakeUkgKickPattern(nextNoteId));
    project.patterns.push_back(MakeUkgKickBuildPattern(nextNoteId));
    project.patterns.push_back(MakeUkgSnarePattern(nextNoteId));
    project.patterns.push_back(MakeUkgClapPattern(nextNoteId));
    project.patterns.push_back(MakeUkgSnapPattern(nextNoteId));
    project.patterns.push_back(MakeUkgPercPattern(nextNoteId));
    project.patterns.push_back(MakeUkgHatSparsePattern(nextNoteId));
    project.patterns.push_back(MakeUkgHatFullPattern(nextNoteId));
    project.patterns.push_back(MakeUkgOpenHatPattern(nextNoteId));
    project.patterns.push_back(MakeUkgBuildHatPattern(nextNoteId));
    project.patterns.push_back(MakeUkgBassPatternA(nextNoteId));
    project.patterns.push_back(MakeUkgBassPatternB(nextNoteId));
    project.patterns.push_back(MakeUkgBassPatternC(nextNoteId));
    project.patterns.push_back(MakeUkgChordPattern(nextNoteId));
    project.patterns.push_back(MakeUkgPluckPattern(nextNoteId));
    project.patterns.push_back(MakeUkgSnareBuildPattern(nextNoteId));
    project.patterns.push_back(MakeUkgFillPattern(nextNoteId));

    std::uint64_t nextClipId = 1;

    auto AddLoop = [&](std::uint64_t trackId, std::uint64_t patternId, double startBeat, double lengthBeats) {
      AddClip(project, nextClipId++, trackId, patternId, startBeat, lengthBeats, 0.0, false);
    };

    // 64 bars total = 256 beats.
    // Bars 1-16: housey garage intro, drums creep in.
    AddLoop(6, 7,   0.0, 64.0);
    AddLoop(7, 9,   8.0, 56.0);
    AddLoop(4, 5,  16.0, 48.0);
    AddLoop(5, 6,  24.0, 40.0);
    AddLoop(9, 14,  0.0, 64.0);
    AddLoop(10, 15, 32.0, 32.0);

    // Bars 17-32: first house/UKG groove.
    AddLoop(1, 1,  64.0, 64.0);
    AddLoop(2, 3,  64.0, 64.0);
    AddLoop(3, 4,  64.0, 64.0);
    AddLoop(4, 5,  64.0, 64.0);
    AddLoop(5, 6,  64.0, 64.0);
    AddLoop(6, 8,  64.0, 64.0);
    AddLoop(7, 9,  64.0, 64.0);
    AddLoop(8, 11, 64.0, 32.0);
    AddLoop(8, 12, 96.0, 32.0);
    AddLoop(9, 14, 64.0, 64.0);

    // Bars 33-48: dirty bass section, more mid bite.
    AddLoop(1, 1,  128.0, 64.0);
    AddLoop(2, 3,  128.0, 64.0);
    AddLoop(3, 4,  128.0, 64.0);
    AddLoop(4, 5,  128.0, 64.0);
    AddLoop(5, 6,  128.0, 64.0);
    AddLoop(6, 8,  128.0, 64.0);
    AddLoop(7, 9,  128.0, 64.0);
    AddLoop(8, 13, 128.0, 64.0);
    AddLoop(9, 14, 128.0, 64.0);
    AddLoop(10, 15, 160.0, 32.0);

    // Bars 49-56: build, strip some low-end, hats rush in.
    AddLoop(1, 2,  192.0, 32.0);
    AddLoop(2, 16, 192.0, 32.0);
    AddLoop(3, 4,  192.0, 32.0);
    AddLoop(4, 5,  192.0, 32.0);
    AddLoop(5, 6,  192.0, 32.0);
    AddLoop(6, 10, 192.0, 32.0);
    AddLoop(7, 9,  192.0, 32.0);
    AddLoop(9, 14, 192.0, 32.0);
    AddLoop(10, 15, 192.0, 32.0);

    // Four-beat turnaround before the last section.
    AddLoop(2, 17, 220.0, 4.0);
    AddLoop(5, 17, 220.0, 4.0);

    // Bars 57-64: final full groove, maximum distortion snack.
    AddLoop(1, 1,  224.0, 32.0);
    AddLoop(2, 3,  224.0, 32.0);
    AddLoop(3, 4,  224.0, 32.0);
    AddLoop(4, 5,  224.0, 32.0);
    AddLoop(5, 6,  224.0, 32.0);
    AddLoop(6, 8,  224.0, 32.0);
    AddLoop(7, 9,  224.0, 32.0);
    AddLoop(8, 13, 224.0, 32.0);
    AddLoop(9, 14, 224.0, 32.0);
    AddLoop(10, 15, 224.0, 32.0);

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
        auto actualPayloadResult =
          sxp::ProjectGeneratorPayloadCodec::DecodeOscillatorV1(
            actualGenerator.payload
          );

        if (!actualPayloadResult.Ok()) {
          Require(actualPayloadResult.error, prefix + " decode actual oscillator payload");
        }

        auto expectedPayloadResult =
          sxp::ProjectGeneratorPayloadCodec::DecodeOscillatorV1(
            expectedGenerator.payload
          );

        if (!expectedPayloadResult.Ok()) {
          Require(expectedPayloadResult.error, prefix + " decode expected oscillator payload");
        }

        CheckOscillatorPayloadEqual(
          actualPayloadResult.value,
          expectedPayloadResult.value,
          prefix + " decoded oscillator payload"
        );
      } else if (actualGenerator.type == sxp::ProjectGeneratorType::Sampler) {
        auto actualPayloadResult =
          sxp::ProjectGeneratorPayloadCodec::DecodeSamplerV1(
            actualGenerator.payload
          );

        if (!actualPayloadResult.Ok()) {
          Require(actualPayloadResult.error, prefix + " decode actual sampler payload");
        }

        auto expectedPayloadResult =
          sxp::ProjectGeneratorPayloadCodec::DecodeSamplerV1(
            expectedGenerator.payload
          );

        if (!expectedPayloadResult.Ok()) {
          Require(expectedPayloadResult.error, prefix + " decode expected sampler payload");
        }

        CheckSamplerPayloadEqual(
          actualPayloadResult.value,
          expectedPayloadResult.value,
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

  void ValidateAssets(
    const sxp::ProjectDocument& actual,
    const sxp::ProjectDocument& expected
  ) {
    CheckEqual(actual.assets.size(), expected.assets.size(), "asset count");

    for (std::size_t i = 0; i < expected.assets.size(); ++i) {
      const auto& actualAsset = actual.assets[i];
      const auto& expectedAsset = expected.assets[i];

      const std::string prefix =
        "asset " + std::to_string(i);

      CheckEqual(actualAsset.id.high, expectedAsset.id.high, prefix + " id high");
      CheckEqual(actualAsset.id.low, expectedAsset.id.low, prefix + " id low");

      CheckEqual(
        static_cast<std::uint32_t>(actualAsset.kind),
        static_cast<std::uint32_t>(expectedAsset.kind),
        prefix + " kind"
      );

      CheckEqual(actualAsset.displayName, expectedAsset.displayName, prefix + " name");
      CheckEqual(actualAsset.originalPathHint, expectedAsset.originalPathHint, prefix + " path");
      CheckEqual(actualAsset.mimeType, expectedAsset.mimeType, prefix + " mime");

      CheckBytesEqual(actualAsset.data, expectedAsset.data, prefix + " data");
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
    ValidateAssets(actual, expected);
  }
}

std::uint64_t CountAssetBytes(const sxp::ProjectDocument& project) {
  std::uint64_t count = 0;

  for (const auto& asset : project.assets) {
    count += static_cast<std::uint64_t>(asset.data.size());
  }

  return count;
}

double GetProjectEndBeat(const sxp::ProjectDocument& project) {
  double endBeat = 0.0;

  for (const auto& clip : project.arrangementClips) {
    endBeat = std::max(
      endBeat,
      clip.startBeat + clip.lengthBeats
    );
  }

  return endBeat;
}

std::size_t CountGeneratorsOfType(
  const sxp::ProjectDocument& project,
  sxp::ProjectGeneratorType type
) {
  std::size_t count = 0;

  for (const auto& generator : project.generators) {
    if (generator.type == type) {
      ++count;
    }
  }

  return count;
}

int main() {
  const std::string path = "/home/code/embedded-sample-ukg-house-dirty-bass.sxp";

  sxp::ProjectDocument project = CreateUkgProject();

  AddUkgAssets(project);

  Require(
    sxp::ProjectFile::Save(path, project),
    "save project"
  );

  auto result = sxp::ProjectFile::Load(path);

  if (!result.Ok()) {
    Require(result.error, "load project");
  }

  ValidateLoadedProject(result.value, project);

  const auto assetBytes = CountAssetBytes(result.value);
  const auto projectEndBeat = GetProjectEndBeat(result.value);

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
    << "  time signature: "
    << result.value.header.timeSigNumerator
    << "/"
    << result.value.header.timeSigDenominator
    << "\n";

  std::cout
    << "  swing: "
    << result.value.header.swingAmount
    << " @ "
    << result.value.header.swingSubdivisionBeats
    << " beats"
    << "\n";

  std::cout
    << "  project length: "
    << projectEndBeat
    << " beats"
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

  std::cout
    << "    oscillators: "
    << CountGeneratorsOfType(
         result.value,
         sxp::ProjectGeneratorType::Oscillator
       )
    << "\n";

  std::cout
    << "    samplers: "
    << CountGeneratorsOfType(
         result.value,
         sxp::ProjectGeneratorType::Sampler
       )
    << "\n";

  std::cout
    << "  assets: "
    << result.value.assets.size()
    << "\n";

  std::cout
    << "  embedded asset bytes: "
    << assetBytes
    << "\n";

  for (const auto& asset : result.value.assets) {
    std::cout
      << "    asset "
      << asset.id.high
      << ":"
      << asset.id.low
      << " | "
      << asset.displayName
      << " | "
      << asset.mimeType
      << " | "
      << asset.data.size()
      << " bytes"
      << "\n";
  }

  return 0;
}
