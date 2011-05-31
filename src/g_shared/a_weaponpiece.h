
#ifndef SRC_G_SHARED_A_WEAPONPIECE_H_INCLUDED
#define SRC_G_SHARED_A_WEAPONPIECE_H_INCLUDED


class AWeaponPiece : public AInventory
{
	DECLARE_CLASS (AWeaponPiece, AInventory)
	HAS_OBJECT_POINTERS
protected:
	bool PrivateShouldStay ();
public:
	void Serialize (FArchive &arc);
	bool TryPickup (AActor *&toucher);
	bool ShouldStay ();
	virtual const char *PickupMessage ();
	virtual void PlayPickupSound (AActor *toucher);

	int PieceValue;
	const PClass * WeaponClass;
	TObjPtr<AWeapon> FullWeapon;
};

// an internal class to hold the information for player class independent weapon piece handling
// [BL] Needs to be available for SBarInfo to check weaponpieces
class AWeaponHolder : public AInventory
{
	DECLARE_CLASS(AWeaponHolder, AInventory)

public:
	int PieceMask;
	const PClass * PieceWeapon;

	void Serialize (FArchive &arc);
};


#endif // SRC_G_SHARED_A_WEAPONPIECE_H_INCLUDED
