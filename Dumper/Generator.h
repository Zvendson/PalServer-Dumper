#pragma once
#include <filesystem>

#include "ObjectArray.h"
#include "DependencyManager.h"
#include "MemberManager.h"
#include "HashStringTable.h"


namespace fs = std::filesystem;

template<typename GeneratorType>
concept GeneratorImplementation = requires(GeneratorType t)
{
    /* Require static variables of type */
    GeneratorType::PredefinedMembers;
    requires(std::same_as<decltype(GeneratorType::PredefinedMembers), PredefinedMemberLookupMapType>);

    GeneratorType::MainFolderName;
    requires(std::same_as<decltype(GeneratorType::MainFolderName), std::string>);
    GeneratorType::IncludefolderName;
    requires(std::same_as<decltype(GeneratorType::IncludefolderName), std::string>);
    GeneratorType::SourcefolderName;
    requires(std::same_as<decltype(GeneratorType::SourcefolderName), std::string>);

    GeneratorType::MainFolder;
    requires(std::same_as<decltype(GeneratorType::MainFolder), fs::path>);
    GeneratorType::Includefolder;
    requires(std::same_as<decltype(GeneratorType::Includefolder), fs::path>);
    GeneratorType::Sourcefolder;
    requires(std::same_as<decltype(GeneratorType::Sourcefolder), fs::path>);
    
    /* Require static functions */
    GeneratorType::Generate();

    GeneratorType::InitPredefinedMembers();
    GeneratorType::InitPredefinedFunctions();
};

class Generator
{
private:
    friend class GeneratorTest;

private:
    static inline fs::path DumperFolder;
    static inline HMODULE  Module = nullptr;

public:
    static void InitModule(HMODULE);
    static void InitEngineCore();
    static void InitInternal();

private:
    static bool SetupDumperFolder();

    static bool SetupFolders(std::string& FolderName, fs::path& OutFolder);
    static bool SetupFolders(std::string& FolderName, fs::path& OutFolder, std::string& IncludefolderName, fs::path& OutSubFolder, std::string& SourcefolderName, fs::path& Sourcefolder);

public:
    template<GeneratorImplementation GeneratorType>
    static void Generate() 
    { 
        if (DumperFolder.empty())
        {
            if (!SetupDumperFolder())
                return;

            ObjectArray::DumpObjects(DumperFolder);
        }

        if (!SetupFolders(
            GeneratorType::MainFolderName, GeneratorType::MainFolder, 
            GeneratorType::IncludefolderName, GeneratorType::Includefolder, 
            GeneratorType::SourcefolderName, GeneratorType::Sourcefolder)
            )
        {
            return;
        }

        GeneratorType::InitPredefinedMembers();
        GeneratorType::InitPredefinedFunctions();

        MemberManager::SetPredefinedMemberLookupPtr(&GeneratorType::PredefinedMembers);

        GeneratorType::Generate();
    };
};
