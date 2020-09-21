//===- CodeCoverage.cpp - Coverage tool based on profiling instrumentation-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// The 'CodeCoverageTool' class implements a command line tool to analyze and
// report coverage information using the profiling instrumentation and code
// coverage mapping.
//
//===----------------------------------------------------------------------===//

#include "CoverageFilters.h"
#include "CoverageReport.h"
#include "CoverageViewOptions.h"
#include "RenderingSupport.h"
#include "SourceCoverageView.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/Triple.h"
#include "llvm/ProfileData/Coverage/CoverageMapping.h"
#include "llvm/ProfileData/InstrProfReader.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Format.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/Process.h"
#include "llvm/Support/ThreadPool.h"
#include <functional>
#include <system_error>

using namespace llvm;
using namespace coverage;

namespace {
/// \brief The implementation of the coverage tool.
class CodeCoverageTool {
public:
  enum Command {
    /// \brief The show command.
    Show,
    /// \brief The report command.
    Report
  };

  /// \brief Print the error message to the error output stream.
  void error(const Twine &Message, StringRef Whence = "");

  /// \brief Record (but do not print) an error message in a thread-safe way.
  void deferError(const Twine &Message, StringRef Whence = "");

  /// \brief Record (but do not print) a warning message in a thread-safe way.
  void deferWarning(const Twine &Message, StringRef Whence = "");

  /// \brief Print (and then clear) all deferred error and warning messages.
  void consumeDeferredMessages();

  /// \brief Append a reference to a private copy of \p Path into SourceFiles.
  void addCollectedPath(const std::string &Path);

  /// \brief Return a memory buffer for the given source file.
  ErrorOr<const MemoryBuffer &> getSourceFile(StringRef SourceFile);

  /// \brief Create source views for the expansions of the view.
  void attachExpansionSubViews(SourceCoverageView &View,
                               ArrayRef<ExpansionRecord> Expansions,
                               CoverageMapping &Coverage);

  /// \brief Create the source view of a particular function.
  std::unique_ptr<SourceCoverageView>
  createFunctionView(const FunctionRecord &Function, CoverageMapping &Coverage);

  /// \brief Create the main source view of a particular source file.
  std::unique_ptr<SourceCoverageView>
  createSourceFileView(StringRef SourceFile, CoverageMapping &Coverage);

  /// \brief Load the coverage mapping data. Return true if an error occured.
  std::unique_ptr<CoverageMapping> load();

  int run(Command Cmd, int argc, const char **argv);

  typedef llvm::function_ref<int(int, const char **)> CommandLineParserType;

  int show(int argc, const char **argv,
           CommandLineParserType commandLineParser);

  int report(int argc, const char **argv,
             CommandLineParserType commandLineParser);

  std::string ObjectFilename;
  CoverageViewOptions ViewOpts;
  std::string PGOFilename;
  CoverageFiltersMatchAll Filters;
  std::vector<StringRef> SourceFiles;
  bool CompareFilenamesOnly;
  StringMap<std::string> RemappedFilenames;
  std::string CoverageArch;

private:
  std::vector<std::string> CollectedPaths;

  std::mutex DeferredMessagesLock;
  std::vector<std::string> DeferredMessages;

  std::mutex LoadedSourceFilesLock;
  std::vector<std::pair<std::string, std::unique_ptr<MemoryBuffer>>>
      LoadedSourceFiles;
};
}

static std::string getErrorString(const Twine &Message, StringRef Whence,
                                  bool Warning) {
  std::string Str = (Warning ? "warning" : "error");
  Str += ": ";
  if (!Whence.empty())
    Str += Whence;
  Str += Message.str() + "\n";
  return Str;
}

void CodeCoverageTool::error(const Twine &Message, StringRef Whence) {
  errs() << getErrorString(Message, Whence, false);
}

void CodeCoverageTool::deferError(const Twine &Message, StringRef Whence) {
  std::unique_lock<std::mutex> Guard{DeferredMessagesLock};
  DeferredMessages.emplace_back(getErrorString(Message, Whence, false));
}

void CodeCoverageTool::deferWarning(const Twine &Message, StringRef Whence) {
  std::unique_lock<std::mutex> Guard{DeferredMessagesLock};
  DeferredMessages.emplace_back(getErrorString(Message, Whence, true));
}

void CodeCoverageTool::consumeDeferredMessages() {
  std::unique_lock<std::mutex> Guard{DeferredMessagesLock};
  for (const std::string &Message : DeferredMessages)
    ViewOpts.colored_ostream(errs(), raw_ostream::RED) << Message;
  DeferredMessages.clear();
}

void CodeCoverageTool::addCollectedPath(const std::string &Path) {
  CollectedPaths.push_back(Path);
  SourceFiles.emplace_back(CollectedPaths.back());
}

ErrorOr<const MemoryBuffer &>
CodeCoverageTool::getSourceFile(StringRef SourceFile) {
  // If we've remapped filenames, look up the real location for this file.
  if (!RemappedFilenames.empty()) {
    auto Loc = RemappedFilenames.find(SourceFile);
    if (Loc != RemappedFilenames.end())
      SourceFile = Loc->second;
  }
  for (const auto &Files : LoadedSourceFiles)
    if (sys::fs::equivalent(SourceFile, Files.first))
      return *Files.second;
  auto Buffer = MemoryBuffer::getFile(SourceFile);
  if (auto EC = Buffer.getError()) {
    deferError(EC.message(), SourceFile);
    return EC;
  }
  std::unique_lock<std::mutex> Guard{LoadedSourceFilesLock};
  LoadedSourceFiles.emplace_back(SourceFile, std::move(Buffer.get()));
  return *LoadedSourceFiles.back().second;
}

void
CodeCoverageTool::attachExpansionSubViews(SourceCoverageView &View,
                                          ArrayRef<ExpansionRecord> Expansions,
                                          CoverageMapping &Coverage) {
  if (!ViewOpts.ShowExpandedRegions)
    return;
  for (const auto &Expansion : Expansions) {
    auto ExpansionCoverage = Coverage.getCoverageForExpansion(Expansion);
    if (ExpansionCoverage.empty())
      continue;
    auto SourceBuffer = getSourceFile(ExpansionCoverage.getFilename());
    if (!SourceBuffer)
      continue;

    auto SubViewExpansions = ExpansionCoverage.getExpansions();
    auto SubView =
        SourceCoverageView::create(Expansion.Function.Name, SourceBuffer.get(),
                                   ViewOpts, std::move(ExpansionCoverage));
    attachExpansionSubViews(*SubView, SubViewExpansions, Coverage);
    View.addExpansion(Expansion.Region, std::move(SubView));
  }
}

std::unique_ptr<SourceCoverageView>
CodeCoverageTool::createFunctionView(const FunctionRecord &Function,
                                     CoverageMapping &Coverage) {
  auto FunctionCoverage = Coverage.getCoverageForFunction(Function);
  if (FunctionCoverage.empty())
    return nullptr;
  auto SourceBuffer = getSourceFile(FunctionCoverage.getFilename());
  if (!SourceBuffer)
    return nullptr;

  auto Expansions = FunctionCoverage.getExpansions();
  auto View = SourceCoverageView::create(Function.Name, SourceBuffer.get(),
                                         ViewOpts, std::move(FunctionCoverage));
  attachExpansionSubViews(*View, Expansions, Coverage);

  return View;
}

std::unique_ptr<SourceCoverageView>
CodeCoverageTool::createSourceFileView(StringRef SourceFile,
                                       CoverageMapping &Coverage) {
  auto SourceBuffer = getSourceFile(SourceFile);
  if (!SourceBuffer)
    return nullptr;
  auto FileCoverage = Coverage.getCoverageForFile(SourceFile);
  if (FileCoverage.empty())
    return nullptr;

  auto Expansions = FileCoverage.getExpansions();
  auto View = SourceCoverageView::create(SourceFile, SourceBuffer.get(),
                                         ViewOpts, std::move(FileCoverage));
  attachExpansionSubViews(*View, Expansions, Coverage);

  for (auto Function : Coverage.getInstantiations(SourceFile)) {
    auto SubViewCoverage = Coverage.getCoverageForFunction(*Function);
    auto SubViewExpansions = SubViewCoverage.getExpansions();
    auto SubView =
        SourceCoverageView::create(Function->Name, SourceBuffer.get(), ViewOpts,
                                   std::move(SubViewCoverage));
    attachExpansionSubViews(*SubView, SubViewExpansions, Coverage);

    if (SubView) {
      unsigned FileID = Function->CountedRegions.front().FileID;
      unsigned Line = 0;
      for (const auto &CR : Function->CountedRegions)
        if (CR.FileID == FileID)
          Line = std::max(CR.LineEnd, Line);
      View->addInstantiation(Function->Name, Line, std::move(SubView));
    }
  }
  return View;
}

static bool modifiedTimeGT(StringRef LHS, StringRef RHS) {
  sys::fs::file_status Status;
  if (sys::fs::status(LHS, Status))
    return false;
  auto LHSTime = Status.getLastModificationTime();
  if (sys::fs::status(RHS, Status))
    return false;
  auto RHSTime = Status.getLastModificationTime();
  return LHSTime > RHSTime;
}

std::unique_ptr<CoverageMapping> CodeCoverageTool::load() {
  if (modifiedTimeGT(ObjectFilename, PGOFilename))
    errs() << "warning: profile data may be out of date - object is newer\n";
  auto CoverageOrErr = CoverageMapping::load(ObjectFilename, PGOFilename,
                                             CoverageArch);
  if (Error E = CoverageOrErr.takeError()) {
    colored_ostream(errs(), raw_ostream::RED)
        << "error: Failed to load coverage: " << toString(std::move(E)) << "\n";
    return nullptr;
  }
  auto Coverage = std::move(CoverageOrErr.get());
  unsigned Mismatched = Coverage->getMismatchedCount();
  if (Mismatched) {
    colored_ostream(errs(), raw_ostream::RED)
        << "warning: " << Mismatched << " functions have mismatched data. ";
    errs() << "\n";
  }

  if (CompareFilenamesOnly) {
    auto CoveredFiles = Coverage.get()->getUniqueSourceFiles();
    for (auto &SF : SourceFiles) {
      StringRef SFBase = sys::path::filename(SF);
      for (const auto &CF : CoveredFiles)
        if (SFBase == sys::path::filename(CF)) {
          RemappedFilenames[CF] = SF;
          SF = CF;
          break;
        }
    }
  }

  return Coverage;
}

int CodeCoverageTool::run(Command Cmd, int argc, const char **argv) {
  cl::opt<std::string, true> ObjectFilename(
      cl::Positional, cl::Required, cl::location(this->ObjectFilename),
      cl::desc("Covered executable or object file."));

  cl::list<std::string> InputSourceFiles(
      cl::Positional, cl::desc("<Source files>"), cl::ZeroOrMore);

  cl::opt<std::string, true> PGOFilename(
      "instr-profile", cl::Required, cl::location(this->PGOFilename),
      cl::desc(
          "File with the profile data obtained after an instrumented run"));

  cl::opt<std::string> Arch(
      "arch", cl::desc("architecture of the coverage mapping binary"));

  cl::opt<bool> DebugDump("dump", cl::Optional,
                          cl::desc("Show internal debug dump"));

  cl::opt<CoverageViewOptions::OutputFormat> Format(
      "format", cl::desc("Output format for line-based coverage reports"),
      cl::values(clEnumValN(CoverageViewOptions::OutputFormat::Text, "text",
                            "Text output"),
                 clEnumValN(CoverageViewOptions::OutputFormat::HTML, "html",
                            "HTML output"),
                 clEnumValEnd),
      cl::init(CoverageViewOptions::OutputFormat::Text));

  cl::opt<bool> FilenameEquivalence(
      "filename-equivalence", cl::Optional,
      cl::desc("Treat source files as equivalent to paths in the coverage data "
               "when the file names match, even if the full paths do not"));

  cl::OptionCategory FilteringCategory("Function filtering options");

  cl::list<std::string> NameFilters(
      "name", cl::Optional,
      cl::desc("Show code coverage only for functions with the given name"),
      cl::ZeroOrMore, cl::cat(FilteringCategory));

  cl::list<std::string> NameRegexFilters(
      "name-regex", cl::Optional,
      cl::desc("Show code coverage only for functions that match the given "
               "regular expression"),
      cl::ZeroOrMore, cl::cat(FilteringCategory));

  cl::opt<double> RegionCoverageLtFilter(
      "region-coverage-lt", cl::Optional,
      cl::desc("Show code coverage only for functions with region coverage "
               "less than the given threshold"),
      cl::cat(FilteringCategory));

  cl::opt<double> RegionCoverageGtFilter(
      "region-coverage-gt", cl::Optional,
      cl::desc("Show code coverage only for functions with region coverage "
               "greater than the given threshold"),
      cl::cat(FilteringCategory));

  cl::opt<double> LineCoverageLtFilter(
      "line-coverage-lt", cl::Optional,
      cl::desc("Show code coverage only for functions with line coverage less "
               "than the given threshold"),
      cl::cat(FilteringCategory));

  cl::opt<double> LineCoverageGtFilter(
      "line-coverage-gt", cl::Optional,
      cl::desc("Show code coverage only for functions with line coverage "
               "greater than the given threshold"),
      cl::cat(FilteringCategory));

  cl::opt<cl::boolOrDefault> UseColor(
      "use-color", cl::desc("Emit colored output (default=autodetect)"),
      cl::init(cl::BOU_UNSET));

  auto commandLineParser = [&, this](int argc, const char **argv) -> int {
    cl::ParseCommandLineOptions(argc, argv, "LLVM code coverage tool\n");
    ViewOpts.Debug = DebugDump;
    CompareFilenamesOnly = FilenameEquivalence;

    ViewOpts.Format = Format;
    switch (ViewOpts.Format) {
    case CoverageViewOptions::OutputFormat::Text:
      ViewOpts.Colors = UseColor == cl::BOU_UNSET
                            ? sys::Process::StandardOutHasColors()
                            : UseColor == cl::BOU_TRUE;
      break;
    case CoverageViewOptions::OutputFormat::HTML:
      if (UseColor == cl::BOU_FALSE)
        error("Color output cannot be disabled when generating html.");
      ViewOpts.Colors = true;
      break;
    }

    // Create the function filters
    if (!NameFilters.empty() || !NameRegexFilters.empty()) {
      auto NameFilterer = new CoverageFilters;
      for (const auto &Name : NameFilters)
        NameFilterer->push_back(llvm::make_unique<NameCoverageFilter>(Name));
      for (const auto &Regex : NameRegexFilters)
        NameFilterer->push_back(
            llvm::make_unique<NameRegexCoverageFilter>(Regex));
      Filters.push_back(std::unique_ptr<CoverageFilter>(NameFilterer));
    }
    if (RegionCoverageLtFilter.getNumOccurrences() ||
        RegionCoverageGtFilter.getNumOccurrences() ||
        LineCoverageLtFilter.getNumOccurrences() ||
        LineCoverageGtFilter.getNumOccurrences()) {
      auto StatFilterer = new CoverageFilters;
      if (RegionCoverageLtFilter.getNumOccurrences())
        StatFilterer->push_back(llvm::make_unique<RegionCoverageFilter>(
            RegionCoverageFilter::LessThan, RegionCoverageLtFilter));
      if (RegionCoverageGtFilter.getNumOccurrences())
        StatFilterer->push_back(llvm::make_unique<RegionCoverageFilter>(
            RegionCoverageFilter::GreaterThan, RegionCoverageGtFilter));
      if (LineCoverageLtFilter.getNumOccurrences())
        StatFilterer->push_back(llvm::make_unique<LineCoverageFilter>(
            LineCoverageFilter::LessThan, LineCoverageLtFilter));
      if (LineCoverageGtFilter.getNumOccurrences())
        StatFilterer->push_back(llvm::make_unique<LineCoverageFilter>(
            RegionCoverageFilter::GreaterThan, LineCoverageGtFilter));
      Filters.push_back(std::unique_ptr<CoverageFilter>(StatFilterer));
    }

    if (!Arch.empty() &&
        Triple(Arch).getArch() == llvm::Triple::ArchType::UnknownArch) {
      errs() << "error: Unknown architecture: " << Arch << "\n";
      return 1;
    }
    CoverageArch = Arch;

    for (const auto &File : InputSourceFiles) {
      SmallString<128> Path(File);
      if (!CompareFilenamesOnly)
        if (std::error_code EC = sys::fs::make_absolute(Path)) {
          errs() << "error: " << File << ": " << EC.message();
          return 1;
        }
      addCollectedPath(Path.str());
    }
    return 0;
  };

  switch (Cmd) {
  case Show:
    return show(argc, argv, commandLineParser);
  case Report:
    return report(argc, argv, commandLineParser);
  }
  return 0;
}

int CodeCoverageTool::show(int argc, const char **argv,
                           CommandLineParserType commandLineParser) {

  cl::OptionCategory ViewCategory("Viewing options");

  cl::opt<bool> ShowLineExecutionCounts(
      "show-line-counts", cl::Optional,
      cl::desc("Show the execution counts for each line"), cl::init(true),
      cl::cat(ViewCategory));

  cl::opt<bool> ShowRegions(
      "show-regions", cl::Optional,
      cl::desc("Show the execution counts for each region"),
      cl::cat(ViewCategory));

  cl::opt<bool> ShowBestLineRegionsCounts(
      "show-line-counts-or-regions", cl::Optional,
      cl::desc("Show the execution counts for each line, or the execution "
               "counts for each region on lines that have multiple regions"),
      cl::cat(ViewCategory));

  cl::opt<bool> ShowExpansions("show-expansions", cl::Optional,
                               cl::desc("Show expanded source regions"),
                               cl::cat(ViewCategory));

  cl::opt<bool> ShowInstantiations("show-instantiations", cl::Optional,
                                   cl::desc("Show function instantiations"),
                                   cl::cat(ViewCategory));

  cl::opt<std::string> ShowOutputDirectory(
      "output-dir", cl::init(""),
      cl::desc("Directory in which coverage information is written out"));
  cl::alias ShowOutputDirectoryA("o", cl::desc("Alias for --output-dir"),
                                 cl::aliasopt(ShowOutputDirectory));

  auto Err = commandLineParser(argc, argv);
  if (Err)
    return Err;

  ViewOpts.ShowLineNumbers = true;
  ViewOpts.ShowLineStats = ShowLineExecutionCounts.getNumOccurrences() != 0 ||
                           !ShowRegions || ShowBestLineRegionsCounts;
  ViewOpts.ShowRegionMarkers = ShowRegions || ShowBestLineRegionsCounts;
  ViewOpts.ShowLineStatsOrRegionMarkers = ShowBestLineRegionsCounts;
  ViewOpts.ShowExpandedRegions = ShowExpansions;
  ViewOpts.ShowFunctionInstantiations = ShowInstantiations;
  ViewOpts.ShowOutputDirectory = ShowOutputDirectory;

  if (ViewOpts.hasOutputDirectory()) {
    if (auto E = sys::fs::create_directories(ViewOpts.ShowOutputDirectory)) {
      error("Could not create output directory!", E.message());
      return 1;
    }
  }

  auto Coverage = load();
  if (!Coverage)
    return 1;

  auto Printer = CoveragePrinter::create(ViewOpts);

  if (!Filters.empty()) {
    auto OSOrErr = Printer->createViewFile("functions", /*InToplevel=*/true);
    if (Error E = OSOrErr.takeError()) {
      error(toString(std::move(E)));
      return 1;
    }
    auto OS = std::move(OSOrErr.get());

    // Show functions.
    for (const auto &Function : Coverage->getCoveredFunctions()) {
      if (!Filters.matches(Function))
        continue;

      auto mainView = createFunctionView(Function, *Coverage);
      if (!mainView) {
        ViewOpts.colored_ostream(errs(), raw_ostream::RED)
            << "warning: Could not read coverage for '" << Function.Name << "'."
            << "\n";
        continue;
      }

      mainView->print(*OS.get(), /*WholeFile=*/false, /*ShowSourceName=*/true);
    }

    Printer->closeViewFile(std::move(OS));
    return 0;
  }

  // Show files
  bool ShowFilenames = SourceFiles.size() != 1;

  if (SourceFiles.empty())
    // Get the source files from the function coverage mapping.
    for (StringRef Filename : Coverage->getUniqueSourceFiles())
      SourceFiles.push_back(Filename);

  // Create an index out of the source files.
  if (ViewOpts.hasOutputDirectory()) {
    if (Error E = Printer->createIndexFile(SourceFiles)) {
      error(toString(std::move(E)));
      return 1;
    }
  }

  // In -output-dir mode, it's safe to use multiple threads to print files.
  unsigned ThreadCount = 1;
  if (ViewOpts.hasOutputDirectory())
    ThreadCount = std::thread::hardware_concurrency();
  ThreadPool Pool(ThreadCount);

  for (StringRef &SourceFile : SourceFiles) {
    Pool.async([this, &SourceFile, &Coverage, &Printer, ShowFilenames] {
      auto View = createSourceFileView(SourceFile, *Coverage);
      if (!View) {
        deferWarning("The file '" + SourceFile.str() + "' isn't covered.");
        return;
      }

      auto OSOrErr = Printer->createViewFile(SourceFile, /*InToplevel=*/false);
      if (Error E = OSOrErr.takeError()) {
        deferError(toString(std::move(E)));
        return;
      }
      auto OS = std::move(OSOrErr.get());

      View->print(*OS.get(), /*Wholefile=*/true,
                  /*ShowSourceName=*/ShowFilenames);
      Printer->closeViewFile(std::move(OS));
    });
  }

  Pool.wait();

  consumeDeferredMessages();

  return 0;
}

int CodeCoverageTool::report(int argc, const char **argv,
                             CommandLineParserType commandLineParser) {
  auto Err = commandLineParser(argc, argv);
  if (Err)
    return Err;

  if (ViewOpts.Format == CoverageViewOptions::OutputFormat::HTML)
    error("HTML output for summary reports is not yet supported.");

  auto Coverage = load();
  if (!Coverage)
    return 1;

  CoverageReport Report(ViewOpts, std::move(Coverage));
  if (SourceFiles.empty())
    Report.renderFileReports(llvm::outs());
  else
    Report.renderFunctionReports(SourceFiles, llvm::outs());
  return 0;
}

int showMain(int argc, const char *argv[]) {
  CodeCoverageTool Tool;
  return Tool.run(CodeCoverageTool::Show, argc, argv);
}

int reportMain(int argc, const char *argv[]) {
  CodeCoverageTool Tool;
  return Tool.run(CodeCoverageTool::Report, argc, argv);
}
