#include <gtest/gtest.h>
#include <atomic>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include "frelsim/compiler/Compiler.hpp"

namespace frelsim::compiler {
namespace {

// Writes contents to a fresh temp file and returns its path; the file is
// removed when the returned guard goes out of scope.
class TempFile {
    public:
        explicit TempFile(std::string const& contents) : path_(nextPath()) {
            std::ofstream file(path_);
            file << contents;
        }

        ~TempFile() {
            std::remove(path_.c_str());
        }

        std::string const& path() const { return path_; }

    private:
        static std::string nextPath() {
            static std::atomic<int> counter{0};
            return (std::filesystem::temp_directory_path()
                / ("frelsim_compiler_test_" + std::to_string(counter++) + ".json")).string();
        }

        std::string path_;
};

TEST(CompilerTest, ParsesWellFormedJsonIntoASystem) {
    TempFile const config(R"({
        "identifier": {"domain": "myDomain", "scope": "myScope", "name": "myName"},
        "stop_time": 5.0,
        "max_step_size": 0.1
    })");

    sim::proto::System const system = Compiler().compile(config.path());

    EXPECT_EQ(system.identifier().domain(), "myDomain");
    EXPECT_DOUBLE_EQ(system.stop_time(), 5.0);
    EXPECT_DOUBLE_EQ(system.max_step_size(), 0.1);
}

TEST(CompilerTest, ThrowsOnAMissingFile) {
    EXPECT_THROW(Compiler().compile("/nonexistent/path/does-not-exist.json"), std::invalid_argument);
}

TEST(CompilerTest, ThrowsOnMalformedJson) {
    TempFile const config("{ this is not valid json");

    EXPECT_THROW(Compiler().compile(config.path()), std::invalid_argument);
}

TEST(CompilerTest, ThrowsOnJsonThatDoesNotConformToTheSystemShape) {
    // Well-formed JSON, but stop_time is a string where the schema expects
    // a number - a genuine schema mismatch, not just malformed syntax.
    TempFile const config(R"({"stop_time": "not a number"})");

    EXPECT_THROW(Compiler().compile(config.path()), std::invalid_argument);
}

} // namespace
} // namespace frelsim::compiler
