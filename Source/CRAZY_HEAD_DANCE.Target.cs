// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;
using System.Collections.Generic;

public class CRAZY_HEAD_DANCETarget : TargetRules
{
	public CRAZY_HEAD_DANCETarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V2;

		ExtraModuleNames.AddRange( new string[] { "CRAZY_HEAD_DANCE" } );

		bCompileChaos = true;
		bUseChaos = true;
	}
}
