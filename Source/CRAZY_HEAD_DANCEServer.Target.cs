// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class CRAZY_HEAD_DANCEServerTarget : TargetRules //Change this line according to the name of your project
{
    public CRAZY_HEAD_DANCEServerTarget(TargetInfo Target) : base(Target) //Change this line according to the name of your project
    {
        Type = TargetType.Server;
        DefaultBuildSettings = BuildSettingsVersion.V2;
        ExtraModuleNames.Add("CRAZY_HEAD_DANCE"); //Change this line according to the name of your project

        bCompileChaos = true;
        bUseChaos = true;
    }
}