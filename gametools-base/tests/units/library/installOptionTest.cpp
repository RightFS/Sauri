#include "common/common.h"
#include "libraryInstallOption.h"
#include "libraryPathHelper.h"

#include "gtest/gtest.h"

namespace leigod {
namespace nngame {
namespace test {

/**
 * @brief Test class for LibraryInstallOption
 */
class LibraryInstallOptionTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup a valid install_options object for tests
        validOption.install_path = "C:/Program Files/TestApp";
        validOption.accept_license = true;
        validOption.create_desktop_shortcuts = true;
        validOption.create_start_menu_shortcuts = true;
        validOption.add_to_path = true;
        validOption.run_as_admin = false;
        validOption.auto_start = false;
        validOption.create_restore_point = true;
        validOption.custom_args = {{"INSTALLDIR", "C:/Custom/Path"}, {"LANGUAGE", "en-US"}};
        validOption.verbose_logging = true;
    }

    library::install_options::LibraryInstallOption validOption;
};

// Test normal serialization and deserialization
TEST_F(LibraryInstallOptionTest, SerializationDeserialization) {
    nlohmann::json j;
    to_json(j, validOption);

    library::install_options::LibraryInstallOption deserialized;
    EXPECT_NO_THROW(from_json(j, deserialized));

    EXPECT_EQ(deserialized.install_path, validOption.install_path);
    EXPECT_EQ(deserialized.accept_license, validOption.accept_license);
    EXPECT_EQ(deserialized.create_desktop_shortcuts, validOption.create_desktop_shortcuts);
    EXPECT_EQ(deserialized.create_start_menu_shortcuts, validOption.create_start_menu_shortcuts);
    EXPECT_EQ(deserialized.add_to_path, validOption.add_to_path);
    EXPECT_EQ(deserialized.run_as_admin, validOption.run_as_admin);
    EXPECT_EQ(deserialized.auto_start, validOption.auto_start);
    EXPECT_EQ(deserialized.create_restore_point, validOption.create_restore_point);
    EXPECT_EQ(deserialized.custom_args, validOption.custom_args);
    EXPECT_EQ(deserialized.verbose_logging, validOption.verbose_logging);
}

// Test missing required field (install_path)
TEST_F(LibraryInstallOptionTest, MissingRequiredFields) {
    // Missing required install_path
    nlohmann::json emptyJson = {};
    library::install_options::LibraryInstallOption option;
    EXPECT_THROW(from_json(emptyJson, option), std::invalid_argument);

    // Create JSON with all fields except install_path
    nlohmann::json j;
    to_json(j, validOption);
    j.erase(library::install_options::INSTALL_PATH);

    library::install_options::LibraryInstallOption testOption;
    EXPECT_THROW(from_json(j, testOption), std::invalid_argument);

    // Verify error message contains the missing field name
    try {
        from_json(j, testOption);
        FAIL() << "Expected std::invalid_argument";
    } catch (const std::invalid_argument& e) {
        std::string error = e.what();
        EXPECT_NE(error.find(library::install_options::INSTALL_PATH), std::string::npos);
    }
}

// Test default values for optional fields
TEST_F(LibraryInstallOptionTest, DefaultValues) {
    // Create JSON with only the required field
    nlohmann::json j = {{library::install_options::INSTALL_PATH, "D:/Apps/Minimal"}};

    library::install_options::LibraryInstallOption option;
    EXPECT_NO_THROW(from_json(j, option));

    // Verify the required field was set
    EXPECT_EQ(option.install_path.string(), "D:/Apps/Minimal");

    // Verify default values for optional fields
    EXPECT_FALSE(option.accept_license);
    EXPECT_TRUE(option.create_desktop_shortcuts);
    EXPECT_TRUE(option.create_start_menu_shortcuts);
    EXPECT_FALSE(option.add_to_path);
    EXPECT_FALSE(option.run_as_admin);
    EXPECT_FALSE(option.auto_start);
    EXPECT_TRUE(option.create_restore_point);
    EXPECT_TRUE(option.custom_args.empty());
    EXPECT_FALSE(option.verbose_logging);
}

// Test handling of special path formats
TEST_F(LibraryInstallOptionTest, SpecialPaths) {
    std::vector<std::filesystem::path> specialPaths = {
        "",                                    // Empty path
        ".",                                   // Current directory
        "..",                                  // Parent directory
        "\\\\network\\share",                  // Network path
        "C:\\Program Files\\App With Spaces",  // Path with spaces
        "relative/path",                       // Relative path
        "/usr/local/bin"                       // Unix-style path
    };

    for (const auto& path : specialPaths) {
        validOption.install_path = path;

        nlohmann::json j;
        to_json(j, validOption);

        library::install_options::LibraryInstallOption deserialized;
        EXPECT_NO_THROW(from_json(j, deserialized));

        EXPECT_EQ(deserialized.install_path, path);
    }
}

// Test custom arguments handling
TEST_F(LibraryInstallOptionTest, CustomArgs) {
    // Test with empty custom args
    validOption.custom_args.clear();
    nlohmann::json j;
    to_json(j, validOption);

    library::install_options::LibraryInstallOption deserialized;
    EXPECT_NO_THROW(from_json(j, deserialized));
    EXPECT_TRUE(deserialized.custom_args.empty());

    // Test with various custom arg values
    std::map<std::string, std::string> testArgs = {{"EMPTY", ""},
                                                   {"SPACES", "value with spaces"},
                                                   {"QUOTES", "value \"with\" quotes"},
                                                   {"SPECIAL", "value\nwith\tspecial\rchars"},
                                                   {"UNICODE", "üñîçødé value テスト"},
                                                   {"PATH", "C:\\Program Files\\App"}};

    validOption.custom_args = testArgs;
    j = nlohmann::json();
    to_json(j, validOption);

    deserialized = library::install_options::LibraryInstallOption();
    EXPECT_NO_THROW(from_json(j, deserialized));

    EXPECT_EQ(deserialized.custom_args.size(), testArgs.size());
    for (const auto& [key, value] : testArgs) {
        EXPECT_TRUE(deserialized.custom_args.count(key) > 0);
        EXPECT_EQ(deserialized.custom_args[key], value);
    }
}

// Test boolean field variations
TEST_F(LibraryInstallOptionTest, BooleanFields) {
    // Try all combinations of boolean fields
    std::vector<bool> boolValues = {true, false};

    for (bool accept : boolValues) {
        for (bool desktop : boolValues) {
            for (bool startmenu : boolValues) {
                validOption.accept_license = accept;
                validOption.create_desktop_shortcuts = desktop;
                validOption.create_start_menu_shortcuts = startmenu;

                nlohmann::json j;
                to_json(j, validOption);

                library::install_options::LibraryInstallOption deserialized;
                EXPECT_NO_THROW(from_json(j, deserialized));

                EXPECT_EQ(deserialized.accept_license, accept);
                EXPECT_EQ(deserialized.create_desktop_shortcuts, desktop);
                EXPECT_EQ(deserialized.create_start_menu_shortcuts, startmenu);
            }
        }
    }
}

// Test handling of extraneous fields
TEST_F(LibraryInstallOptionTest, ExtraneousFields) {
    nlohmann::json j;
    to_json(j, validOption);

    // Add extraneous fields that don't exist in the struct
    j["non_existent_field"] = "should be ignored";
    j["another_field"] = 123;
    j["nested"] = {{"key", "value"}};

    library::install_options::LibraryInstallOption deserialized;
    // Should not throw, extraneous fields should be ignored
    EXPECT_NO_THROW(from_json(j, deserialized));

    // Verify all original fields were parsed correctly
    EXPECT_EQ(deserialized.install_path, validOption.install_path);
    EXPECT_EQ(deserialized.accept_license, validOption.accept_license);
}

// Test error cases with incorrect types
TEST_F(LibraryInstallOptionTest, TypeErrors) {
    // Test with install_path as boolean instead of string
    nlohmann::json j1 = {
        {library::install_options::INSTALL_PATH, true}  // Should be string
    };

    library::install_options::LibraryInstallOption option1;
    EXPECT_THROW(from_json(j1, option1), nlohmann::json::type_error);

    // Test with boolean field as string
    nlohmann::json j2;
    to_json(j2, validOption);
    j2[library::install_options::ACCEPT_LICENSE] = "true";  // Should be boolean

    library::install_options::LibraryInstallOption option2;
    EXPECT_THROW(from_json(j2, option2), nlohmann::json::type_error);

    // Test with custom_args as array instead of object
    nlohmann::json j3;
    to_json(j3, validOption);
    j3[library::install_options::CUSTOM_ARGS] = nlohmann::json::array();  // Should be object

    library::install_options::LibraryInstallOption option3;
    EXPECT_THROW(from_json(j3, option3), nlohmann::json::type_error);
}

// Test with null values
TEST_F(LibraryInstallOptionTest, NullValues) {
    nlohmann::json j;
    to_json(j, validOption);

    // Set custom_args to null
    j[library::install_options::CUSTOM_ARGS] = nullptr;

    library::install_options::LibraryInstallOption deserialized;
    EXPECT_NO_THROW(from_json(j, deserialized));

    // Custom args should be empty when null in JSON
    EXPECT_TRUE(deserialized.custom_args.empty());
}

// Test with very long path
TEST_F(LibraryInstallOptionTest, LongPath) {
    // Create a very long path (over 260 characters which is Windows MAX_PATH)
    std::string longPathStr = "C:/";
    for (int i = 0; i < 20; i++) {
        longPathStr += "very_long_directory_name_";
    }
    longPathStr += "/file.exe";

    std::filesystem::path longPath(longPathStr);
    validOption.install_path = longPath;

    nlohmann::json j;
    to_json(j, validOption);

    library::install_options::LibraryInstallOption deserialized;
    EXPECT_NO_THROW(from_json(j, deserialized));

    EXPECT_EQ(deserialized.install_path, longPath);
}

// Test serialization determinism
TEST_F(LibraryInstallOptionTest, SerializationDeterminism) {
    nlohmann::json j1;
    to_json(j1, validOption);

    nlohmann::json j2;
    to_json(j2, validOption);

    // Same input should produce identical JSON
    EXPECT_EQ(j1.dump(), j2.dump());
}

// Test creating and validating installation config from external source
TEST_F(LibraryInstallOptionTest, ExternalSource) {
    // Simulate JSON coming from external source (e.g., config file)
    const std::string jsonStr = R"({
        "install_path": "E:/Games/TestGame",
        "accept_license": true,
        "create_desktop_shortcuts": false,
        "run_as_admin": true,
        "custom_args": {
            "LANG": "en_US",
            "MODE": "silent"
        }
    })";

    nlohmann::json j = nlohmann::json::parse(jsonStr);

    library::install_options::LibraryInstallOption option;
    EXPECT_NO_THROW(from_json(j, option));

    // Verify parsed values
    EXPECT_EQ(option.install_path.string(), "E:/Games/TestGame");
    EXPECT_TRUE(option.accept_license);
    EXPECT_FALSE(option.create_desktop_shortcuts);
    EXPECT_TRUE(option.run_as_admin);
    EXPECT_EQ(option.custom_args.size(), 2);
    EXPECT_EQ(option.custom_args["LANG"], "en_US");

    // Verify default values for missing fields
    EXPECT_TRUE(option.create_start_menu_shortcuts);  // default is true
    EXPECT_FALSE(option.auto_start);                  // default is false
}

// Test creating installation config with proper metadata
TEST_F(LibraryInstallOptionTest, CreateWithMetadata) {
    // Helper function to create installation config with metadata
    auto createConfigWithMetadata =
        [](const library::install_options::LibraryInstallOption& config) -> nlohmann::json {
        // First serialize the config
        nlohmann::json result;
        to_json(result, config);

        result["version"] = "1.0.0";

        return result;
    };

    // Test the helper function
    nlohmann::json configWithMeta = createConfigWithMetadata(validOption);

    // Verify config can still be parsed
    library::install_options::LibraryInstallOption parsedConfig;
    EXPECT_NO_THROW(from_json(configWithMeta, parsedConfig));
    EXPECT_EQ(parsedConfig.install_path, validOption.install_path);
}

// Test handling of malformed path
TEST_F(LibraryInstallOptionTest, MalformedPath) {
    // Test with invalid Windows path characters (e.g., |, *, ?, etc.)
    std::vector<std::string> invalidPaths = {"C:\\Invalid|Path", "C:\\Invalid*Path",
                                             "C:\\Invalid?Path", "C:\\Invalid\"Path",
                                             "C:\\Invalid<Path", "C:\\Invalid>Path"};

    for (const auto& invalidPath : invalidPaths) {
        nlohmann::json j = {{library::install_options::INSTALL_PATH, invalidPath}};

        library::install_options::LibraryInstallOption option;

        // The from_json method doesn't validate path syntax, only converts strings to paths
        // So this should not throw, but the path might not be valid for file operations
        EXPECT_NO_THROW(from_json(j, option));
        EXPECT_EQ(option.install_path.string(), invalidPath);
    }
}

// Test path normalization behavior
TEST_F(LibraryInstallOptionTest, PathNormalization) {
    // Test with various path formats that should normalize to the same path
    std::vector<std::pair<std::string, std::string>> pathTests = {
        {"C:\\Windows\\System32", "C:\\Windows\\System32"},
        {"C:/Windows/System32", "C:\\Windows\\System32"},
        {"C:\\Windows\\System32\\..\\.\\System32", "C:\\Windows\\System32"}};

    for (const auto& [input, expected] : pathTests) {
        validOption.install_path = input;

        nlohmann::json j;
        to_json(j, validOption);

        library::install_options::LibraryInstallOption deserialized;
        from_json(j, deserialized);

        // Note: Path normalization depends on std::filesystem behavior,
        // so this test might need adjustment based on actual behavior
        std::filesystem::path normalizedPath = deserialized.install_path.lexically_normal();
        EXPECT_EQ(normalizedPath.string(), expected);
    }
}

// Test with international/non-ASCII paths
TEST_F(LibraryInstallOptionTest, InternationalPaths) {
    std::vector<std::string> internationalPaths = {
        "C:\\程序文件\\测试应用",                // Chinese
        "C:\\Archivos de programa\\Aplicación",  // Spanish
        "C:\\Программы\\Тест",                   // Russian
        "C:\\プログラム\\テスト",                // Japanese
        "C:\\مجلد\\تطبيق",                       // Arabic
        "C:\\Documents\\résumé.txt"              // Accented characters
    };

    for (const auto& path : internationalPaths) {
        validOption.install_path = common::utils::toPath(path);

        nlohmann::json j;
        to_json(j, validOption);

        // Verify serialized JSON contains the path
        std::string jsonStr = j.dump();

        // Create a copy of the path with escaped backslashes for JSON comparison
        std::string escapedPath = path;
        size_t pos = 0;
        while ((pos = escapedPath.find('\\', pos)) != std::string::npos) {
            escapedPath.replace(pos, 1, "\\\\");
            pos += 2;  // Skip the inserted backslashes
        }

        EXPECT_NE(jsonStr.find(escapedPath), std::string::npos);

        library::install_options::LibraryInstallOption deserialized;
        EXPECT_NO_THROW(from_json(j, deserialized));

        EXPECT_EQ(deserialized.install_path, validOption.install_path);
    }
}

// Test with very large custom args map
TEST_F(LibraryInstallOptionTest, LargeCustomArgsMap) {
    // Create a large map with hundreds of entries
    std::map<std::string, std::string> largeMap;
    for (int i = 0; i < 500; i++) {
        largeMap[std::string("KEY_") + std::to_string(i)] =
            std::string("VALUE_") + std::to_string(i);
    }

    validOption.custom_args = largeMap;

    nlohmann::json j;
    to_json(j, validOption);

    library::install_options::LibraryInstallOption deserialized;
    EXPECT_NO_THROW(from_json(j, deserialized));

    EXPECT_EQ(deserialized.custom_args.size(), 500);
    EXPECT_EQ(deserialized.custom_args["KEY_123"], "VALUE_123");
    EXPECT_EQ(deserialized.custom_args["KEY_499"], "VALUE_499");
}

// Test platform-specific path behavior
TEST_F(LibraryInstallOptionTest, PlatformSpecificPaths) {
    // Test with both Windows and Unix-style paths
    std::vector<std::pair<std::string, bool>> platformPaths = {
        {"C:\\Windows\\System32", true},  // Windows path
        {"/usr/local/bin", true},         // Unix path
        {"relative/path", true},          // Relative path (both platforms)
        {"\\\\server\\share", true},      // UNC path (Windows)
        {"C:", true}                      // Drive letter only (Windows)
    };

    for (const auto& [path, shouldWork] : platformPaths) {
        validOption.install_path = path;

        nlohmann::json j;
        to_json(j, validOption);

        library::install_options::LibraryInstallOption deserialized;
        if (shouldWork) {
            EXPECT_NO_THROW(from_json(j, deserialized));
            EXPECT_EQ(deserialized.install_path.string(), path);
        } else {
            EXPECT_THROW(from_json(j, deserialized), std::exception);
        }
    }
}

// Test multiple instances and bulk operations
TEST_F(LibraryInstallOptionTest, BulkOperations) {
    // Create multiple configuration instances
    std::vector<library::install_options::LibraryInstallOption> configs;
    for (int i = 0; i < 100; i++) {
        library::install_options::LibraryInstallOption config;
        config.install_path = "C:/Install/App_" + std::to_string(i);
        config.accept_license = (i % 2 == 0);
        config.create_desktop_shortcuts = (i % 3 == 0);
        config.custom_args = {{"INDEX", std::to_string(i)},
                              {"EVEN", (i % 2 == 0) ? "true" : "false"}};
        configs.push_back(config);
    }

    // Serialize all instances
    std::vector<nlohmann::json> jsonConfigs;
    for (const auto& config : configs) {
        nlohmann::json j;
        to_json(j, config);
        jsonConfigs.push_back(j);
    }

    // Deserialize all instances
    std::vector<library::install_options::LibraryInstallOption> deserializedConfigs;
    for (const auto& j : jsonConfigs) {
        library::install_options::LibraryInstallOption config;
        EXPECT_NO_THROW(from_json(j, config));
        deserializedConfigs.push_back(config);
    }

    // Verify results
    EXPECT_EQ(deserializedConfigs.size(), configs.size());
    for (size_t i = 0; i < configs.size(); i++) {
        EXPECT_EQ(deserializedConfigs[i].install_path, configs[i].install_path);
        EXPECT_EQ(deserializedConfigs[i].accept_license, configs[i].accept_license);
        EXPECT_EQ(deserializedConfigs[i].custom_args["INDEX"], configs[i].custom_args["INDEX"]);
    }
}

// Test version compatibility scenario
TEST_F(LibraryInstallOptionTest, VersionCompatibility) {
    // Simulate an older version of the configuration format
    // that might be missing some newer fields
    nlohmann::json legacyJson = {
        {library::install_options::INSTALL_PATH, "D:/Legacy/App"},
        {library::install_options::ACCEPT_LICENSE, true},
        {library::install_options::CREATE_DESKTOP_SHORTCUTS, false}
        // Newer fields like auto_start might be missing
    };

    // Should still parse correctly with defaults for missing fields
    library::install_options::LibraryInstallOption config;
    EXPECT_NO_THROW(from_json(legacyJson, config));

    EXPECT_EQ(config.install_path.string(), "D:/Legacy/App");
    EXPECT_TRUE(config.accept_license);
    EXPECT_FALSE(config.create_desktop_shortcuts);
    EXPECT_FALSE(config.auto_start);  // Default value for missing field

    // Add a version field to track format version
    nlohmann::json configWithVersion;
    to_json(configWithVersion, config);
    configWithVersion["format_version"] = "2.0";

    std::string serialized = configWithVersion.dump();
    EXPECT_NE(serialized.find("format_version"), std::string::npos);

    // Parsing should ignore unknown fields
    library::install_options::LibraryInstallOption parsedConfig;
    EXPECT_NO_THROW(from_json(configWithVersion, parsedConfig));
}

// Test edge case: Empty JSON with only required fields
TEST_F(LibraryInstallOptionTest, MinimalValidConfiguration) {
    // Test with the absolute minimum required configuration
    nlohmann::json minimalJson = {
        {library::install_options::INSTALL_PATH, ""}  // Empty path is syntactically valid
    };

    library::install_options::LibraryInstallOption config;
    EXPECT_NO_THROW(from_json(minimalJson, config));
    EXPECT_EQ(config.install_path, std::filesystem::path(""));

    // All other fields should have default values
    EXPECT_FALSE(config.accept_license);
    EXPECT_TRUE(config.create_desktop_shortcuts);
    EXPECT_FALSE(config.verbose_logging);
}

}  // namespace test
}  // namespace nngame
}  // namespace leigod