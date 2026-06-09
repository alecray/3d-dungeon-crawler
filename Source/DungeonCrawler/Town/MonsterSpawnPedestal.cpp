#include "MonsterSpawnPedestal.h"
#include "MonsterCharacter.h"
#include "MonsterTypes.h"
#include "BossMonster.h"

#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Animation/AnimSequence.h"
#include "UObject/SoftObjectPath.h"

AMonsterSpawnPedestal::AMonsterSpawnPedestal()
{
	PrimaryActorTick.bCanEverTick = false;

	Body = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Body"));
	Body->InitCapsuleSize(50.f, 70.f);
	// Query-only, block only Visibility so the player's interact line-trace can hit it.
	Body->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Body->SetCollisionResponseToAllChannels(ECR_Ignore);
	Body->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	SetRootComponent(Body);

	// Real pedestal model + its spawn animation (assigned in BeginPlay from PedestalMeshPath).
	Mesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Body);
	Mesh->SetRelativeLocation(FVector(0.f, 0.f, -70.f)); // base sits at the capsule bottom
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetVisibility(false); // shown once a real mesh resolves

	// Graybox stand-in cylinder until the FBX is imported.
	Graybox = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Graybox"));
	Graybox->SetupAttachment(Body);
	Graybox->SetRelativeScale3D(FVector(0.7f, 0.7f, 1.4f));
	Graybox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (UStaticMesh* Cyl = Cast<UStaticMesh>(FSoftObjectPath(TEXT("/Engine/BasicShapes/Cylinder.Cylinder")).TryLoad()))
	{
		Graybox->SetStaticMesh(Cyl);
	}
}

void AMonsterSpawnPedestal::BeginPlay()
{
	Super::BeginPlay();

	// Swap the graybox for the real pedestal mesh if it's been imported.
	if (USkeletalMesh* Skel = PedestalMeshPath.IsNull() ? nullptr : PedestalMeshPath.LoadSynchronous())
	{
		Mesh->SetSkeletalMesh(Skel);
		Mesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);
		Mesh->SetVisibility(true);
		Graybox->SetVisibility(false);

		SpawnAnim = SpawnAnimPath.IsNull() ? nullptr : SpawnAnimPath.LoadSynchronous();
		if (SpawnAnim)
		{
			// Rest on the first frame of the spawn pose until a spawn plays it through.
			Mesh->PlayAnimation(SpawnAnim, /*bLooping*/ false);
			Mesh->Stop();
		}
	}
}

FVector AMonsterSpawnPedestal::GetSpawnLocation() const
{
	// In front of the pedestal, nudged up so the capsule settles onto the floor instead of clipping it.
	return GetActorLocation() + GetActorForwardVector() * SpawnForwardOffset + FVector(0.f, 0.f, 60.f);
}

void AMonsterSpawnPedestal::PlaySpawnAnim()
{
	if (Mesh && SpawnAnim && Mesh->GetSkeletalMeshAsset())
	{
		Mesh->PlayAnimation(SpawnAnim, /*bLooping*/ false);
	}
}

AMonsterCharacter* AMonsterSpawnPedestal::FinishSpawn(AMonsterCharacter* Monster, const FVector& Loc)
{
	if (!Monster)
	{
		return nullptr;
	}
	// Face the player side (away from the pedestal) and start wandering passively around the spawn point.
	Monster->SetActorRotation(FRotator(0.f, GetActorRotation().Yaw, 0.f));
	Monster->SetRoamHome(Loc, RoamRadius);
	Monster->SetPassive(true);
	Spawned.Add(Monster);
	PlaySpawnAnim();
	return Monster;
}

AMonsterCharacter* AMonsterSpawnPedestal::SpawnMonsterType(FName TypeId)
{
	UWorld* World = GetWorld();
	if (!World || !MonsterDatabase::Contains(TypeId))
	{
		return nullptr;
	}
	const FMonsterDef& Def = MonsterDatabase::Get(TypeId);
	TSubclassOf<AMonsterCharacter> Cls = Def.MonsterClass;
	if (!Cls) { Cls = AMonsterCharacter::StaticClass(); }

	const FVector Loc = GetSpawnLocation();
	FActorSpawnParameters P;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	AMonsterCharacter* M = World->SpawnActor<AMonsterCharacter>(Cls, FTransform(FRotator::ZeroRotator, Loc), P);
	if (!M)
	{
		return nullptr;
	}
	M->ApplyType(TypeId); // stats + skeletal body
	return FinishSpawn(M, Loc);
}

AMonsterCharacter* AMonsterSpawnPedestal::SpawnBossClass(TSubclassOf<AMonsterCharacter> BossClass)
{
	UWorld* World = GetWorld();
	if (!World || !*BossClass)
	{
		return nullptr;
	}
	const FVector Loc = GetSpawnLocation();
	FActorSpawnParameters P;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	AMonsterCharacter* M = World->SpawnActor<AMonsterCharacter>(BossClass, FTransform(FRotator::ZeroRotator, Loc), P);
	if (!M)
	{
		return nullptr;
	}
	// Bosses configure their own mesh in BeginPlay; just initialise the difficulty tier so stats are valid.
	if (ABossMonster* Boss = Cast<ABossMonster>(M))
	{
		Boss->SetTier(0);
	}
	return FinishSpawn(M, Loc);
}

void AMonsterSpawnPedestal::ClearSpawned()
{
	for (AMonsterCharacter* M : Spawned)
	{
		if (IsValid(M))
		{
			M->Destroy();
		}
	}
	Spawned.Reset();
}
