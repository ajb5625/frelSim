#include <gtest/gtest.h>
#include <atomic>
#include <filesystem>
#include <fstream>
#include <sstream>
#include "frelsim/recorder/Recorder.hpp"

namespace frelsim::recorder {
namespace {

class TempFile {
    public:
        TempFile() : path_(nextPath()) {}

        ~TempFile() {
            std::remove(path_.c_str());
        }

        std::string const& path() const { return path_; }

        std::string readContents() const {
            std::ifstream file(path_);
            std::ostringstream contents;
            contents << file.rdbuf();
            return contents.str();
        }

    private:
        static std::string nextPath() {
            static std::atomic<int> counter{0};
            return (std::filesystem::temp_directory_path()
                / ("frelsim_recorder_test_" + std::to_string(counter++) + ".csv")).string();
        }

        std::string path_;
};

util::Identifier makeId(std::string const& domain, std::string const& scope, std::string const& name) {
    util::Identifier id;
    id.setDomain(domain);
    id.setScope(scope);
    id.setName(name);
    return id;
}

TEST(RecorderTest, RecordsOneRowPerCallAndReportsRowCount) {
    Recorder recorder({makeId("plant", "Output", "position")});
    EXPECT_EQ(recorder.rowCount(), 0u);

    recorder.record(0.0, [](std::string const&, Identifiers const&) {
        return Values{type::core::Value::makeDouble(1.0)};
    });
    recorder.record(0.1, [](std::string const&, Identifiers const&) {
        return Values{type::core::Value::makeDouble(2.0)};
    });

    EXPECT_EQ(recorder.rowCount(), 2u);
}

TEST(RecorderTest, WritesExpectedCsvHeaderAndRows) {
    Recorder recorder({makeId("plant", "Output", "position"), makeId("pid", "Output", "output")});

    recorder.record(0.0, [](std::string const& domain, Identifiers const&) {
        if (domain == "plant") return Values{type::core::Value::makeDouble(1.5)};
        return Values{type::core::Value::makeDouble(-2.5)};
    });
    recorder.record(0.1, [](std::string const& domain, Identifiers const&) {
        if (domain == "plant") return Values{type::core::Value::makeDouble(1.6)};
        return Values{type::core::Value::makeDouble(-2.4)};
    });

    TempFile const file;
    recorder.writeCsv(file.path());

    std::istringstream contents(file.readContents());
    std::string line;
    std::getline(contents, line);
    EXPECT_EQ(line, "time,plant.Output.position,pid.Output.output");

    std::getline(contents, line);
    EXPECT_EQ(line, "0,1.5,-2.5");

    std::getline(contents, line);
    EXPECT_EQ(line, "0.1,1.6,-2.4");
}

TEST(RecorderTest, GroupsMultipleWatchedIdentifiersInTheSameDomainIntoOneGetterCall) {
    Recorder recorder({makeId("plant", "Output", "position"), makeId("plant", "Output", "velocity")});

    int getterCallCount = 0;
    recorder.record(0.0, [&getterCallCount](std::string const&, Identifiers const& ids) {
        ++getterCallCount;
        EXPECT_EQ(ids.size(), 2u);
        return Values{type::core::Value::makeDouble(1.0), type::core::Value::makeDouble(2.0)};
    });

    EXPECT_EQ(getterCallCount, 1);
}

TEST(RecorderTest, CoercesIntegerAndBoolValuesToDouble) {
    Recorder recorder({makeId("a", "Output", "intField"), makeId("a", "Output", "boolField")});

    recorder.record(0.0, [](std::string const&, Identifiers const&) {
        return Values{type::core::Value::makeInt(42), type::core::Value::makeBool(true)};
    });

    TempFile const file;
    recorder.writeCsv(file.path());

    std::istringstream contents(file.readContents());
    std::string line;
    std::getline(contents, line); // header
    std::getline(contents, line);
    EXPECT_EQ(line, "0,42,1");
}

TEST(RecorderTest, ThrowsOnANonNumericWatchedValue) {
    Recorder recorder({makeId("a", "Output", "stringField")});

    EXPECT_THROW(recorder.record(0.0, [](std::string const&, Identifiers const&) {
        return Values{type::core::Value::makeString("hello")};
    }), std::logic_error);
}

TEST(RecorderTest, ThrowsIfPathCannotBeOpenedForWriting) {
    Recorder recorder({makeId("a", "Output", "x")});
    EXPECT_THROW(recorder.writeCsv("/nonexistent/directory/log.csv"), std::invalid_argument);
}

} // namespace
} // namespace frelsim::recorder
