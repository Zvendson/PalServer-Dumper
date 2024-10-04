#include "Generator.h"
#include "HashStringTable.h"
#include "StructManager.h"
#include "EnumManager.h"
#include "MemberManager.h"
#include "PackageManager.h"
#include "Utils.h"

inline void InitWeakObjectPtrSettings()
{
	UEStruct LoadAsset = ObjectArray::FindObjectFast<UEFunction>("LoadAsset", EClassCastFlags::Function);

	if (!LoadAsset)
	{
		std::cout << "\nDumper-7: 'LoadAsset' wasn't found, could not determine value for 'bIsWeakObjectPtrWithoutTag'!\n" << std::endl;
		return;
	}

	UEProperty Asset = LoadAsset.FindMember("Asset", EClassCastFlags::SoftObjectProperty);
	if (!Asset)
	{
		std::cout << "\nDumper-7: 'Asset' wasn't found, could not determine value for 'bIsWeakObjectPtrWithoutTag'!\n" << std::endl;
		return;
	}

	UEStruct SoftObjectPath = ObjectArray::FindObjectFast<UEStruct>("SoftObjectPath");

	constexpr int32 SizeOfFFWeakObjectPtr = 0x08;
	constexpr int32 OldUnrealAssetPtrSize = 0x10;
	const int32 SizeOfSoftObjectPath = SoftObjectPath ? SoftObjectPath.GetStructSize() : OldUnrealAssetPtrSize;

	Settings::Internal::bIsWeakObjectPtrWithoutTag = Asset.GetSize() <= (SizeOfSoftObjectPath + SizeOfFFWeakObjectPtr);

	//std::cout << std::format("\nDumper-7: bIsWeakObjectPtrWithoutTag = {}\n", Settings::Internal::bIsWeakObjectPtrWithoutTag) << std::endl;
}

inline void InitLargeWorldCoordinateSettings()
{
	UEStruct FVectorStruct = ObjectArray::FindObjectFast<UEStruct>("Vector", EClassCastFlags::Struct);

	if (!FVectorStruct) [[unlikely]]
	{
		std::cout << "\nSomething went horribly wrong, FVector wasn't even found!\n\n" << std::endl;
		return;
	}

	UEProperty XProperty = FVectorStruct.FindMember("X");

	if (!XProperty) [[unlikely]]
	{
		std::cout << "\nSomething went horribly wrong, FVector::X wasn't even found!\n\n" << std::endl;
		return;
	}

	/* Check the underlaying type of FVector::X. If it's double we're on UE5.0, or higher, and using large world coordinates. */
	Settings::Internal::bUseLargeWorldCoordinates = XProperty.IsA(EClassCastFlags::DoubleProperty);

	//std::cout << std::format("\nDumper-7: bUseLargeWorldCoordinates = {}\n", Settings::Internal::bUseLargeWorldCoordinates) << std::endl;
}

inline void InitSettings()
{
	InitWeakObjectPtrSettings();
	InitLargeWorldCoordinateSettings();
}


void Generator::InitModule(HMODULE module)
{
	Generator::Module = module;
}

void Generator::InitEngineCore()
{
	/* manual override */
	//ObjectArray::Init(/*GObjects*/, /*ChunkSize*/, /*bIsChunked*/);
	//FName::Init(/*FName::AppendString*/);
	//FName::Init(/*FName::ToString, FName::EOffsetOverrideType::ToString*/);
	//FName::Init(/*GNames, FName::EOffsetOverrideType::GNames, true/false*/);
	//Off::InSDK::ProcessEvent::InitPE(/*PEIndex*/);

	/* Back4Blood*/
	//InitObjectArrayDecryption([](void* ObjPtr) -> uint8* { return reinterpret_cast<uint8*>(uint64(ObjPtr) ^ 0x8375); });

	/* Multiversus [Unsupported, weird GObjects-struct]*/
	//InitObjectArrayDecryption([](void* ObjPtr) -> uint8* { return reinterpret_cast<uint8*>(uint64(ObjPtr) ^ 0x1B5DEAFD6B4068C); });

	ObjectArray::Init();
	FName::Init();
	Off::Init();
	PropertySizes::Init();
	Off::InSDK::ProcessEvent::InitPE(); //Must be at this position, relies on offsets initialized in Off::Init()

	Off::InSDK::World::InitGWorld(); //Must be at this position, relies on offsets initialized in Off::Init()

	Off::InSDK::Text::InitTextOffsets(); //Must be at this position, relies on offsets initialized in Off::InitPE()

	InitSettings();
}

void Generator::InitInternal()
{
	// Initialize PackageManager with all packages, their names, structs, classes enums, functions and dependencies
	PackageManager::Init();

	// Initialize StructManager with all structs and their names
	StructManager::Init();
	
	// Initialize EnumManager with all enums and their names
	EnumManager::Init();
	
	// Initialized all Member-Name collisions
	MemberManager::Init();

	// Post-Initialize PackageManager after StructManager has been initialized. 'PostInit()' handles Cyclic-Dependencies detection
	PackageManager::PostInit();
}

std::string GetDllDirectoryPath(HMODULE hModule)
{
	char path[MAX_PATH] = { 0 };

	// Get the full path of the module (DLL)
	GetModuleFileNameA(hModule, path, MAX_PATH);

	// Convert the full path to a std::string
	std::string fullPath(path);

	// Find the last backslash to extract the folder path
	size_t lastBackslashPos = fullPath.find_last_of("\\/");
	if (lastBackslashPos != std::string::npos)
	{
		return fullPath.substr(0, lastBackslashPos);  // Return the folder path
	}

	return "";  // If for some reason the path is invalid, return an empty string
}

bool Generator::SetupDumperFolder()
{
	try
	{
		std::string dllDir = GetDllDirectoryPath(Generator::Module);
		std::string FolderName = (Settings::Generator::GameVersion + '-' + Settings::Generator::GameName);

		FileNameHelper::MakeValidFileName(FolderName);

		DumperFolder = fs::path(dllDir) / Settings::Generator::SDKGenerationPath / FolderName;

		std::cout << "Path: " << DumperFolder.generic_string() << "\n";

		if (fs::exists(DumperFolder))
		{
			fs::path Old = DumperFolder.generic_string() + "_OLD";

			fs::remove_all(Old);

			fs::rename(DumperFolder, Old);
		}

		fs::create_directories(DumperFolder);
	}
	catch (const std::filesystem::filesystem_error& fe)
	{
		std::cout << "Could not create required folders! Info: \n";
		std::cout << fe.what() << std::endl;
		return false;
	}

	return true;
}

bool Generator::SetupFolders(std::string& FolderName, fs::path& OutFolder)
{
	fs::path Dummy;
	std::string EmptyName = "";
	return SetupFolders(FolderName, OutFolder, EmptyName, Dummy, EmptyName, Dummy);
}

bool Generator::SetupFolders(std::string& FolderName, fs::path& OutFolder, std::string& IncludefolderName, fs::path& OutIncludeFolder, std::string& SourcefolderName, fs::path& OutSourcefolder)
{
	FileNameHelper::MakeValidFileName(FolderName);
	FileNameHelper::MakeValidFileName(IncludefolderName);
	FileNameHelper::MakeValidFileName(SourcefolderName);

	try
	{
		OutFolder = DumperFolder / FolderName;
		OutIncludeFolder = OutFolder / IncludefolderName;
		OutSourcefolder = OutFolder / SourcefolderName;
				
		if (fs::exists(OutFolder))
		{
			fs::path Old = OutFolder.generic_string() + "_OLD";

			fs::remove_all(Old);

			fs::rename(OutFolder, Old);
		}

		fs::create_directories(OutFolder);

		if (!IncludefolderName.empty())
			fs::create_directories(OutIncludeFolder);

		if (!SourcefolderName.empty())
			fs::create_directories(OutSourcefolder);
	}
	catch (const std::filesystem::filesystem_error& fe)
	{
		std::cout << "Could not create required folders! Info: \n";
		std::cout << fe.what() << std::endl;
		return false;
	}

	return true;
}