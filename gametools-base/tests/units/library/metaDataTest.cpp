#include "libraryMetaData.h"

#include "gtest/gtest.h"

namespace leigod {
namespace nngame {
namespace test {

class LibraryMetadataTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup a valid metadata object for tests
        validMetadata.appId = 1008611;
        validMetadata.installer_type = installer::InstallerType::kNSIS;
        validMetadata.main_executable = "app.exe";
        validMetadata.name = "Test App";
        validMetadata.version = "1.0.0";
        validMetadata.publisher = "Test Publisher";
        validMetadata.description = "Test Description";
        validMetadata.minimum_os_version = "10.0";
        validMetadata.target_architecture = "x64";
        validMetadata.silent = true;
        validMetadata.requires_admin = false;
        validMetadata.custom_attributes = {{"key1", "value1"}, {"key2", "value2"}};
        validMetadata.dependencies = {"dep1", "dep2"};
        validMetadata.suggested_install_path = "C:\\Program Files\\TestApp";
    }

    library::meta_data::LibraryMetadata validMetadata;
};

// Test normal serialization and deserialization
TEST_F(LibraryMetadataTest, SerializationDeserialization) {
    nlohmann::json j;
    to_json(j, validMetadata);

    library::meta_data::LibraryMetadata deserialized;
    from_json(j, deserialized);

    EXPECT_EQ(deserialized.appId, validMetadata.appId);
    EXPECT_EQ(deserialized.installer_type, validMetadata.installer_type);
    EXPECT_EQ(deserialized.main_executable, validMetadata.main_executable);
    EXPECT_EQ(deserialized.name, validMetadata.name);
    EXPECT_EQ(deserialized.version, validMetadata.version);
    EXPECT_EQ(deserialized.publisher, validMetadata.publisher);
    EXPECT_EQ(deserialized.description, validMetadata.description);
    EXPECT_EQ(deserialized.minimum_os_version, validMetadata.minimum_os_version);
    EXPECT_EQ(deserialized.target_architecture, validMetadata.target_architecture);
    EXPECT_EQ(deserialized.silent, validMetadata.silent);
    EXPECT_EQ(deserialized.requires_admin, validMetadata.requires_admin);
    EXPECT_EQ(deserialized.custom_attributes, validMetadata.custom_attributes);
    EXPECT_EQ(deserialized.dependencies, validMetadata.dependencies);
    EXPECT_EQ(deserialized.suggested_install_path, validMetadata.suggested_install_path);
}

// Test missing required fields
TEST_F(LibraryMetadataTest, MissingRequiredFields) {
    // Missing all required fields
    nlohmann::json emptyJson = {};
    library::meta_data::LibraryMetadata metadata;
    EXPECT_THROW(from_json(emptyJson, metadata), std::invalid_argument);

    // Missing one required field at a time
    std::vector<std::string> requiredFields = {
        library::meta_data::APP_ID, library::meta_data::INSTALLER_TYPE,
        library::meta_data::MAIN_EXECUTABLE, library::meta_data::NAME, library::meta_data::VERSION};

    for (const auto& field : requiredFields) {
        nlohmann::json j;
        to_json(j, validMetadata);
        j.erase(field);

        library::meta_data::LibraryMetadata testMetadata;
        EXPECT_THROW(from_json(j, testMetadata), std::invalid_argument);
    }
}

// Test optional fields
TEST_F(LibraryMetadataTest, OptionalFields) {
    nlohmann::json j;
    to_json(j, validMetadata);

    // Remove all optional fields
    j.erase(library::meta_data::PUBLISHER);
    j.erase(library::meta_data::DESCRIPTION);
    j.erase(library::meta_data::MIN_OS_VERSION);
    j.erase(library::meta_data::TARGET_ARCH);
    j.erase(library::meta_data::CUSTOM_ATTRS);
    j.erase(library::meta_data::DEPENDENCIES);
    j.erase(library::meta_data::INSTALL_PATH);

    library::meta_data::LibraryMetadata deserialized;
    EXPECT_NO_THROW(from_json(j, deserialized));

    // Check default values for optional fields
    EXPECT_EQ(deserialized.publisher, "");
    EXPECT_EQ(deserialized.description, "");
    EXPECT_EQ(deserialized.minimum_os_version, "");
    EXPECT_EQ(deserialized.target_architecture, "");
    EXPECT_TRUE(deserialized.custom_attributes.empty());
    EXPECT_TRUE(deserialized.dependencies.empty());
    EXPECT_EQ(deserialized.suggested_install_path, "");
}

// Test path conversion
TEST_F(LibraryMetadataTest, PathConversion) {
    std::filesystem::path testPath = "C:\\test\\path\\🐸";
    nlohmann::json j = testPath;

    std::filesystem::path deserialized;
    from_json(j, deserialized);

    EXPECT_EQ(deserialized, testPath);
}

// Test edge cases
TEST_F(LibraryMetadataTest, EdgeCases) {
    // Test with empty strings
    validMetadata.appId = 1;
    validMetadata.main_executable = "";
    validMetadata.name = "";
    validMetadata.version = "";

    nlohmann::json j;
    to_json(j, validMetadata);

    // Define deserialized variable
    library::meta_data::LibraryMetadata deserialized;
    EXPECT_NO_THROW(from_json(j, deserialized));

    EXPECT_EQ(deserialized.appId, 1);
    EXPECT_EQ(deserialized.main_executable, "");
    EXPECT_EQ(deserialized.name, "");
    EXPECT_EQ(deserialized.version, "");

    // Test with special characters
    validMetadata.appId = 1008611;
    validMetadata.description = "Line1\nLine2\tTabbed";

    j = nlohmann::json();
    to_json(j, validMetadata);

    deserialized = library::meta_data::LibraryMetadata();
    EXPECT_NO_THROW(from_json(j, deserialized));

    EXPECT_EQ(deserialized.appId, 1008611);
    EXPECT_EQ(deserialized.description, "Line1\nLine2\tTabbed");
}

// Test type errors - testing incorrect data types
TEST_F(LibraryMetadataTest, TypeErrors) {
    // Create JSON with incorrect types
    nlohmann::json j = {
        {library::meta_data::APP_ID, 1008611},
        {library::meta_data::INSTALLER_TYPE, "nsis"},
        {library::meta_data::MAIN_EXECUTABLE, "app.exe"},
        {library::meta_data::NAME, 123},     // Should be string, not number
        {library::meta_data::VERSION, true}  // Should be string, not boolean
    };

    library::meta_data::LibraryMetadata metadata;
    EXPECT_THROW(from_json(j, metadata), nlohmann::json::type_error);

    // Test with array instead of object for custom attributes
    nlohmann::json j2;
    to_json(j2, validMetadata);
    j2[library::meta_data::CUSTOM_ATTRS] = nlohmann::json::array();  // Should be object

    library::meta_data::LibraryMetadata metadata2;
    EXPECT_THROW(from_json(j2, metadata2), nlohmann::json::type_error);
}

// Test handling of extraneous fields
TEST_F(LibraryMetadataTest, ExtraneousFields) {
    nlohmann::json j;
    to_json(j, validMetadata);

    // Add extraneous fields that don't exist in the struct
    j["non_existent_field"] = "should be ignored";
    j["another_field"] = 123;

    library::meta_data::LibraryMetadata deserialized;
    // Should not throw, extraneous fields should be ignored
    EXPECT_NO_THROW(from_json(j, deserialized));

    // Verify all original fields were parsed correctly
    EXPECT_EQ(deserialized.appId, validMetadata.appId);
    EXPECT_EQ(deserialized.name, validMetadata.name);
    EXPECT_EQ(deserialized.version, validMetadata.version);
}

// Test empty and null custom attributes and dependencies
TEST_F(LibraryMetadataTest, EmptyCollections) {
    nlohmann::json j;
    to_json(j, validMetadata);

    // Set empty collections
    j[library::meta_data::CUSTOM_ATTRS] = nlohmann::json::object();
    j[library::meta_data::DEPENDENCIES] = nlohmann::json::array();

    library::meta_data::LibraryMetadata deserialized;
    EXPECT_NO_THROW(from_json(j, deserialized));

    EXPECT_TRUE(deserialized.custom_attributes.empty());
    EXPECT_TRUE(deserialized.dependencies.empty());

    // Test with null values
    j[library::meta_data::CUSTOM_ATTRS] = nullptr;
    j[library::meta_data::DEPENDENCIES] = nullptr;

    library::meta_data::LibraryMetadata nullDeserialized;
    EXPECT_NO_THROW(from_json(j, nullDeserialized));

    // Should default to empty collections
    EXPECT_TRUE(nullDeserialized.custom_attributes.empty());
    EXPECT_TRUE(nullDeserialized.dependencies.empty());
}

// Test with extremely long strings
TEST_F(LibraryMetadataTest, LongStrings) {
    // Create a very long string (10KB)
    std::string longString(10240, 'X');

    validMetadata.description = longString;

    nlohmann::json j;
    to_json(j, validMetadata);

    library::meta_data::LibraryMetadata deserialized;
    EXPECT_NO_THROW(from_json(j, deserialized));

    EXPECT_EQ(deserialized.description.size(), longString.size());
}

// Test Unicode and international characters
TEST_F(LibraryMetadataTest, UnicodeSupport) {
    // Set fields with Unicode characters
    validMetadata.name = "测试应用";                    // Chinese characters
    validMetadata.description = "Описание приложения";  // Russian characters
    validMetadata.publisher = "üñîçødé テスト";         // Mixed Unicode characters

    nlohmann::json j;
    to_json(j, validMetadata);

    library::meta_data::LibraryMetadata deserialized;
    EXPECT_NO_THROW(from_json(j, deserialized));

    EXPECT_EQ(deserialized.name, "测试应用");
    EXPECT_EQ(deserialized.description, "Описание приложения");
    EXPECT_EQ(deserialized.publisher, "üñîçødé テスト");
}

// Test handling malformed JSON
TEST_F(LibraryMetadataTest, MalformedJson) {
    // This test doesn't use from_json directly but demonstrates
    // how the system handles malformed JSON
    const std::string malformedJson = R"(
        {"appId": "test.app", "name": "Test App", "version":
    )";  // Incomplete JSON

    EXPECT_THROW(auto ret = nlohmann::json::parse(malformedJson), nlohmann::json::parse_error);
}

// Test special path formats
TEST_F(LibraryMetadataTest, SpecialPaths) {
    std::vector<std::filesystem::path> specialPaths = {
        "",                                         // Empty path
        ".",                                        // Current directory
        "..",                                       // Parent directory
        "\\\\network\\share",                       // Network path
        "C:\\Program Files\\App Name With Spaces",  // Path with spaces
        "relative/path",                            // Relative path
        "/usr/local/bin"                            // Unix-style path
    };

    for (const auto& path : specialPaths) {
        validMetadata.suggested_install_path = path;

        nlohmann::json j;
        to_json(j, validMetadata);

        library::meta_data::LibraryMetadata deserialized;
        EXPECT_NO_THROW(from_json(j, deserialized));

        EXPECT_EQ(deserialized.suggested_install_path, path);
    }
}

// Test custom attributes with special cases
TEST_F(LibraryMetadataTest, CustomAttributesEdgeCases) {
    // Test with empty keys and values
    validMetadata.custom_attributes = {{"", "EmptyKey"}, {"EmptyValue", ""}, {"", ""}};

    nlohmann::json j;
    to_json(j, validMetadata);

    library::meta_data::LibraryMetadata deserialized;
    EXPECT_NO_THROW(from_json(j, deserialized));

    EXPECT_EQ(deserialized.custom_attributes.size(), 2);
    EXPECT_EQ(deserialized.custom_attributes[""], "EmptyKey");

    // Test with special characters in keys
    validMetadata.custom_attributes = {{"key.with.dots", "value1"},
                                       {"key-with-hyphens", "value2"},
                                       {"key with spaces", "value3"},
                                       {"key_with_underscores", "value4"},
                                       {"key+with+plus", "value5"}};

    j = nlohmann::json();
    to_json(j, validMetadata);

    deserialized = library::meta_data::LibraryMetadata();
    EXPECT_NO_THROW(from_json(j, deserialized));

    EXPECT_EQ(deserialized.custom_attributes.size(), 5);
    EXPECT_EQ(deserialized.custom_attributes["key.with.dots"], "value1");
    EXPECT_EQ(deserialized.custom_attributes["key with spaces"], "value3");
}

// Test very large collections
TEST_F(LibraryMetadataTest, LargeCollections) {
    // Create metadata with many dependencies and custom attributes
    std::vector<std::string> largeDependencies;
    std::map<std::string, std::string> largeCustomAttrs;

    // Add 1000 entries
    for (int i = 0; i < 1000; i++) {
        largeDependencies.push_back("dep" + std::to_string(i));
        largeCustomAttrs["key" + std::to_string(i)] = "value" + std::to_string(i);
    }

    validMetadata.dependencies = largeDependencies;
    validMetadata.custom_attributes = largeCustomAttrs;

    nlohmann::json j;
    to_json(j, validMetadata);

    library::meta_data::LibraryMetadata deserialized;
    EXPECT_NO_THROW(from_json(j, deserialized));

    EXPECT_EQ(deserialized.dependencies.size(), 1000);
    EXPECT_EQ(deserialized.custom_attributes.size(), 1000);
    EXPECT_EQ(deserialized.custom_attributes["key500"], "value500");
}

// Test version format validation (if applicable)
TEST_F(LibraryMetadataTest, VersionFormatVariations) {
    std::vector<std::string> versionFormats = {"1.0.0",
                                               "1.0",
                                               "1",
                                               "1.0.0-alpha",
                                               "1.0.0-beta.1",
                                               "1.0.0+build.123",
                                               "v1.0.0",
                                               "1.0.0-rc.1+build.123",
                                               "0.0.0",
                                               "999.999.999"};

    for (const auto& version : versionFormats) {
        validMetadata.version = version;

        nlohmann::json j;
        to_json(j, validMetadata);

        library::meta_data::LibraryMetadata deserialized;
        EXPECT_NO_THROW(from_json(j, deserialized));

        EXPECT_EQ(deserialized.version, version);
    }
}

// Test recovery from partially valid JSON
TEST_F(LibraryMetadataTest, PartiallyValidJson) {
    // Create JSON with some missing fields but all required ones
    nlohmann::json partialJson = {
        {library::meta_data::APP_ID, 1008611},
        {library::meta_data::INSTALLER_TYPE, installer::InstallerType::kNSIS},
        {library::meta_data::MAIN_EXECUTABLE, "app.exe"},
        {library::meta_data::NAME, "Test App"},
        {library::meta_data::VERSION, "1.0.0"}
        // All other fields missing
    };

    auto t = partialJson.dump();

    library::meta_data::LibraryMetadata deserialized;
    EXPECT_NO_THROW(from_json(partialJson, deserialized));

    // Check required fields are set correctly
    EXPECT_EQ(deserialized.appId, 1008611);
    EXPECT_EQ(deserialized.installer_type, installer::InstallerType::kNSIS);
    EXPECT_EQ(deserialized.name, "Test App");
    EXPECT_EQ(deserialized.version, "1.0.0");

    // Check optional fields have default values
    EXPECT_EQ(deserialized.publisher, "");
    EXPECT_TRUE(deserialized.dependencies.empty());
    EXPECT_TRUE(deserialized.custom_attributes.empty());
}

// Test serialization determinism
TEST_F(LibraryMetadataTest, SerializationDeterminism) {
    nlohmann::json j1;
    to_json(j1, validMetadata);

    nlohmann::json j2;
    to_json(j2, validMetadata);

    // Same input should produce identical JSON
    EXPECT_EQ(j1.dump(), j2.dump());

    // Deserialize both and compare
    library::meta_data::LibraryMetadata deserialized1;
    library::meta_data::LibraryMetadata deserialized2;

    from_json(j1, deserialized1);
    from_json(j2, deserialized2);

    EXPECT_EQ(deserialized1.appId, deserialized2.appId);
    EXPECT_EQ(deserialized1.name, deserialized2.name);
    EXPECT_EQ(deserialized1.version, deserialized2.version);
}

}  // namespace test
}  // namespace nngame
}  // namespace leigod