#pragma once

UENUM(BlueprintType)
enum class EAmmoType : uint8
{
	EAT_9mm UMETA(Display = "9mm"),
	EAT_5_56mm UMETA(Display = "5.62mm"),

	EAT_NAX UMETA(DisplayName = "DefaultMAX")
};
