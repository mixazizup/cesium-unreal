// Copyright 2020-2021 CesiumGS, Inc. and Contributors

using UnrealBuildTool;
using System.IO;
using System.Linq;

public class CesiumEditor : ModuleRules
{
    public CesiumEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicIncludePaths.AddRange(
            new string[] {
                // ... add public include paths required here ...
            }
        );


        PrivateIncludePaths.AddRange(
            new string[] {
                Path.Combine(ModuleDirectory, "../ThirdParty/include")
            }
        );

        string platform = Target.Platform == UnrealTargetPlatform.Win64
            ? "Windows-x64"
            : "Unknown";

        string libPath = Path.Combine(ModuleDirectory, "../ThirdParty/lib/" + platform);
        string debugPostfix = (Target.Configuration == UnrealTargetConfiguration.Debug || Target.Configuration == UnrealTargetConfiguration.DebugGame) ? "d" : "";

        string[] libs = new string[]
        {
            "CesiumIonClient",
            "csprng"
        };

        PublicAdditionalLibraries.AddRange(libs.Select(lib => Path.Combine(libPath, lib + debugPostfix + ".lib")));

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "UnrealEd",
                "CesiumRuntime"
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "Engine",
                "ApplicationCore",
                "Slate",
                "SlateCore",
                "MeshDescription",
                "StaticMeshDescription",
                "HTTP",
                "MikkTSpace",
                "Chaos",
                "Projects",
                "InputCore",
                "PropertyEditor",
                "DeveloperSettings"
				// ... add private dependencies that you statically link with here ...	
			}
        );

        PublicDefinitions.AddRange(
            new string[]
            {
            }
        );

        DynamicallyLoadedModuleNames.AddRange(
            new string[]
            {
                // ... add any modules that your module loads dynamically here ...
            }
        );

        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PrivatePCHHeaderFile = "PCH.h";
        CppStandard = CppStandardVersion.Cpp17;
    }
}
