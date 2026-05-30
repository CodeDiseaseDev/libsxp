#include <project/ProjectDocument.h>
#include <project/ProjectFile.h>

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#include "project/ProjectGenerator.h"

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

    std::exit(1);
  }

  sxp::ProjectDocument CreateTestProject() {
    sxp::ProjectDocument project;

    project.header.projectName = "My Synthem Project";
    project.header.bpm = 128.0;
    project.header.timeSigNumerator = 4;
    project.header.timeSigDenominator = 4;
    project.header.swingAmount = 0.35;
    project.header.swingSubdivisionBeats = 0.25;

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

    for (int i = 0; i < 4; ++i) {
      sxp::ProjectPatternNote note;
      note.id = static_cast<std::uint64_t>(i + 1);
      note.midiNote = static_cast<std::uint8_t>(60 + i);
      note.velocity = 1.0f;
      note.startBeat = static_cast<double>(i);
      note.lengthBeats = 1.0;

      pattern.notes.push_back(note);
    }

    project.patterns.push_back(pattern);

    sxp::ProjectArrangementClip clipA;
    clipA.id = 1;
    clipA.trackId = track.id;
    clipA.patternId = pattern.id;
    clipA.startBeat = 0.0;
    clipA.lengthBeats = 4.0;
    clipA.patternStartBeat = 0.0;
    clipA.muted = false;

    sxp::ProjectArrangementClip clipB;
    clipB.id = 2;
    clipB.trackId = track.id;
    clipB.patternId = pattern.id;
    clipB.startBeat = 4.0;
    clipB.lengthBeats = 4.0;
    clipB.patternStartBeat = 1.0;
    clipB.muted = true;

    project.arrangementClips.push_back(clipA);
    project.arrangementClips.push_back(clipB);

    sxp::ProjectGenerator generatorA;
    generatorA.nodeId = 2;
    generatorA.name = "Kick sampler";
    generatorA.type = sxp::ProjectGeneratorType::Sampler;
    generatorA.version = 1;
    generatorA.payload = { 1, 2, 3, 4 };

    sxp::ProjectGenerator generatorB;
    generatorB.nodeId = 3;
    generatorB.name = "Bass oscillator";
    generatorB.type = sxp::ProjectGeneratorType::Oscillator;
    generatorB.version = 1;
    generatorB.payload = { 9, 8, 7 };

    project.generators.push_back(generatorA);
    project.generators.push_back(generatorB);

    return project;
  }

  void ValidateLoadedProject(const sxp::ProjectDocument& loaded) {
    CheckEqual(
      loaded.header.projectName,
      std::string("My Synthem Project"),
      "project name"
    );

    CheckNear(loaded.header.bpm, 128.0, "project bpm");
    CheckEqual(loaded.header.timeSigNumerator, 4u, "time signature numerator");
    CheckEqual(loaded.header.timeSigDenominator, 4u, "time signature denominator");
    CheckNear(loaded.header.swingAmount, 0.35, "swing amount");
    CheckNear(loaded.header.swingSubdivisionBeats, 0.25, "swing subdivision beats");

    CheckEqual(loaded.tracks.size(), std::size_t{1}, "track count");
    CheckEqual(loaded.tracks[0].id, std::uint64_t{1}, "track id");
    CheckEqual(loaded.tracks[0].name, std::string("Track 1"), "track name");

    CheckEqual(
      loaded.tracks[0].generatorNodeIds.size(),
      std::size_t{2},
      "track generator node count"
    );

    CheckEqual(
      loaded.tracks[0].generatorNodeIds[0],
      std::uint64_t{2},
      "track generator node 0"
    );

    CheckEqual(
      loaded.tracks[0].generatorNodeIds[1],
      std::uint64_t{3},
      "track generator node 1"
    );

    CheckEqual(loaded.patterns.size(), std::size_t{1}, "pattern count");
    CheckEqual(loaded.patterns[0].id, std::uint64_t{1}, "pattern id");
    CheckEqual(loaded.patterns[0].name, std::string("Pattern 1"), "pattern name");
    CheckNear(loaded.patterns[0].lengthBeats, 8.0, "pattern length beats");

    CheckEqual(
      loaded.patterns[0].notes.size(),
      std::size_t{4},
      "pattern note count"
    );

    for (std::size_t i = 0; i < loaded.patterns[0].notes.size(); ++i) {
      const auto& note = loaded.patterns[0].notes[i];

      CheckEqual(
        note.id,
        static_cast<std::uint64_t>(i + 1),
        "note id"
      );

      CheckEqual(
        note.midiNote,
        static_cast<std::int32_t>(60 + i),
        "note midi note"
      );

      CheckNear(note.velocity, 1.0, "note velocity");
      CheckNear(note.startBeat, static_cast<double>(i), "note start beat");
      CheckNear(note.lengthBeats, 1.0, "note length beats");
    }

    CheckEqual(
      loaded.arrangementClips.size(),
      std::size_t{2},
      "arrangement clip count"
    );

    const auto& clipA = loaded.arrangementClips[0];

    CheckEqual(clipA.id, std::uint64_t{1}, "clip 0 id");
    CheckEqual(clipA.trackId, std::uint64_t{1}, "clip 0 track id");
    CheckEqual(clipA.patternId, std::uint64_t{1}, "clip 0 pattern id");
    CheckNear(clipA.startBeat, 0.0, "clip 0 start beat");
    CheckNear(clipA.lengthBeats, 4.0, "clip 0 length beats");
    CheckNear(clipA.patternStartBeat, 0.0, "clip 0 pattern offset beat");
    CheckEqual(clipA.muted, false, "clip 0 muted");

    const auto& clipB = loaded.arrangementClips[1];

    CheckEqual(clipB.id, std::uint64_t{2}, "clip 1 id");
    CheckEqual(clipB.trackId, std::uint64_t{1}, "clip 1 track id");
    CheckEqual(clipB.patternId, std::uint64_t{1}, "clip 1 pattern id");
    CheckNear(clipB.startBeat, 4.0, "clip 1 start beat");
    CheckNear(clipB.lengthBeats, 4.0, "clip 1 length beats");
    CheckNear(clipB.patternStartBeat, 1.0, "clip 1 pattern offset beat");
    CheckEqual(clipB.muted, true, "clip 1 muted");

    CheckEqual(loaded.generators.size(), std::size_t{2}, "generator count");

    CheckEqual(loaded.generators[0].nodeId, std::uint64_t{2}, "generator 0 node id");
    CheckEqual(loaded.generators[0].name, std::string("Kick sampler"), "generator 0 name");
    CheckEqual(loaded.generators[0].version, std::uint32_t{1}, "generator 0 version");
    CheckEqual(
      static_cast<std::uint32_t>(loaded.generators[0].type),
      static_cast<std::uint32_t>(sxp::ProjectGeneratorType::Sampler),
      "generator 0 type"
    );
    CheckBytesEqual(
      loaded.generators[0].payload,
      std::vector<std::uint8_t>{ 1, 2, 3, 4 },
      "generator 0 payload"
    );

    CheckEqual(loaded.generators[1].nodeId, std::uint64_t{3}, "generator 1 node id");
    CheckEqual(loaded.generators[1].name, std::string("Bass oscillator"), "generator 1 name");
    CheckEqual(loaded.generators[1].version, std::uint32_t{1}, "generator 1 version");
    CheckEqual(
      static_cast<std::uint32_t>(loaded.generators[1].type),
      static_cast<std::uint32_t>(sxp::ProjectGeneratorType::Oscillator),
      "generator 1 type"
    );
    CheckBytesEqual(
      loaded.generators[1].payload,
      std::vector<std::uint8_t>{ 9, 8, 7 },
      "generator 1 payload"
    );
  }
}

int main() {
  const std::string path = "full-house-beat.sxp";

  sxp::ProjectDocument project = CreateTestProject();

  Require(
    sxp::ProjectFile::Save(path, project),
    "save project"
  );

  auto result = sxp::ProjectFile::Load(path);

  if (!result.Ok()) {
    Require(result.error, "load project");
  }

  ValidateLoadedProject(result.value);

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
    << result.value.patterns[0].notes.size()
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