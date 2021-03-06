// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "IKeyArea.h"
#include "ISequencerKeyCollection.h"


class FSequencerDisplayNode;
class UMovieSceneSection;


/** Keyable structure used to identify a particular FGroupedKeyArea */
struct FIndexKey
{
	FIndexKey(FString InNodePath, UMovieSceneSection* InSection);

	friend bool operator==(const FIndexKey& A, const FIndexKey& B)
	{
		return A.CachedHash == B.CachedHash && A.NodePath == B.NodePath && A.Section == B.Section;
	}

	friend uint32 GetTypeHash( const FIndexKey& IndexKey )
	{
		return IndexKey.CachedHash;
	}

private:

	/** Cached path of the node that we relate to */
	FString NodePath;

	/** The movie scene section that we relate to */
	TWeakObjectPtr<UMovieSceneSection> Section;

	/** Cached hash for efficiency (to avoid having to re-hash strings all the time) */
	uint32 CachedHash;
};


/** Structure that represents a number of keys */
struct FKeyGrouping
{
	/** Structure to define a single key in this group - persisting of a key handle, and an index for its parent key area */
	struct FKeyIndex
	{
		FKeyIndex(int32 InAreaIndex, FKeyHandle InKeyHandle) : AreaIndex(InAreaIndex), KeyHandle(InKeyHandle) {}

		/** Index into an externally owned array of IKeyArea ptrs. Kept this way to avoid having a shared ptr per key */
		int32 AreaIndex;
		/** The actual key handle for this key, obtained from its owner IKeyArea */
		FKeyHandle KeyHandle;
	};

	/** Construct an empty key group */
	FKeyGrouping(float InRepresentativeTime)
		: RepresentativeTime(InRepresentativeTime)
	{
	}

	/** Construct this group with a single key handle */
	FKeyGrouping(float InRepresentativeTime, int32 AreaIndex, FKeyHandle KeyHandle)
		: RepresentativeTime(InRepresentativeTime)
	{
		Keys.Emplace(AreaIndex, KeyHandle);
	}

	/** Move construction/assignment */
	FKeyGrouping(FKeyGrouping&& In) : RepresentativeTime(In.RepresentativeTime), Keys(MoveTemp(In.Keys)) {}
	FKeyGrouping& operator=(FKeyGrouping&& In) { RepresentativeTime = In.RepresentativeTime; Keys = MoveTemp(In.Keys); return *this; }

	/** The representative time at which all keys within this group are located */
	float RepresentativeTime;
	/** Array of keys contained within this group */
	TArray<FKeyIndex> Keys;
};


/** A key area that represents multiple, distinct key areas */
class FGroupedKeyCollection
	: public ISequencerKeyCollection
{
public:

	/** Constructor */
	FGroupedKeyCollection();

public:

	/** Initialize this key collection from the specified nodes. Only gathers keys from those explicitly specified. */
	virtual void InitializeExplicit(const TArray<FSequencerDisplayNode*>& InNodes, float DuplicateThreshold = SMALL_NUMBER) override;

	/** Initialize this key collection from the specified nodes. Gathers keys from all child nodes. */
	virtual void InitializeRecursive(const TArray<FSequencerDisplayNode*>& InNodes, float DuplicateThreshold = SMALL_NUMBER) override;
	
	/** Initialize this key collection from the specified node and section index. */
	virtual void InitializeRecursive(FSequencerDisplayNode& InNode, int32 InSectionIndex, float DuplicateThreshold = SMALL_NUMBER) override;

	/** Iterate the keys contained within this collection */
	virtual void IterateKeys(const TFunctionRef<bool(float)>& Iter) override;

	/** Get a value specifying how close keys need to be in order to be considered equal by this collection */
	virtual float GetKeyGroupingThreshold() const override;

	/** Find the first key in the given range. */
	virtual TOptional<float> FindFirstKeyInRange(const TRange<float>& InRange, EFindKeyDirection Direction) const override;

public:

	/** Get the desired tint for the specified key handle */
	FLinearColor GetKeyTint( FKeyHandle InHandle ) const;

	/** Get the brush name for the specified key handle */
	const FSlateBrush* GetBrush(FKeyHandle InHandle) const;

protected:

	/** Add the specified key area to this collection */
	void AddKeyArea(const TSharedRef<IKeyArea>& InKeyArea);

	/** Remove key groups that reside at similar times */
	void RemoveDuplicateKeys(float DuplicateThreshold);

	/** Update the persistent index with our new key handles */
	void UpdateIndex() const;

	/** Helper function to find the group of keys relating to the specified handle */
	const FKeyGrouping* FindGroup(FKeyHandle InHandle) const;

	/** Helper function to find the group of keys relating to the specified handle */
	FKeyGrouping* FindGroup(FKeyHandle InHandle);

protected:
	/** Array of (child) key areas that we are reflecting */
	TArray<TSharedRef<IKeyArea>> KeyAreas;

	/** Generated array of groups of keys harvested from the above array */
	TArray<FKeyGrouping> Groups;

	/** Key into our index */
	FIndexKey IndexKey;

	/** Value specifying how close keys need to be in order to be considered equal by this collection */
	float GroupingThreshold;
};

/** A key area that represents multiple, distinct key areas */
class FGroupedKeyArea
	: public IKeyArea
	, public FGroupedKeyCollection
{
public:

	/** Construct the area from a display node */
	FGroupedKeyArea(FSequencerDisplayNode& InNode, int32 InSectionIndex);

public:

	// IKeyArea interface

	virtual TArray<FKeyHandle> GetUnsortedKeyHandles() const override;
	virtual void SetKeyTime(FKeyHandle KeyHandle, float NewKeyTime) const override;
	virtual float GetKeyTime(FKeyHandle KeyHandle) const override;
	virtual FKeyHandle MoveKey(FKeyHandle KeyHandle, float DeltaPosition) override;
	virtual void DeleteKey(FKeyHandle KeyHandle) override;
	virtual void SetKeyInterpMode(FKeyHandle KeyHandle, ERichCurveInterpMode InterpMode) override;
	virtual ERichCurveInterpMode GetKeyInterpMode(FKeyHandle KeyHandle) const override;
	virtual void SetKeyTangentMode(FKeyHandle KeyHandle, ERichCurveTangentMode TangentMode) override;
	virtual TSharedPtr<FStructOnScope> GetKeyStruct(FKeyHandle KeyHandle) override;
	virtual ERichCurveTangentMode GetKeyTangentMode(FKeyHandle KeyHandle) const override;
	virtual void SetExtrapolationMode(ERichCurveExtrapolation ExtrapMode, bool bPreInfinity) override;
	virtual ERichCurveExtrapolation GetExtrapolationMode(bool bPreInfinity) const override;
	virtual bool CanSetExtrapolationMode() const override;
	virtual TArray<FKeyHandle> AddKeyUnique(float Time, EMovieSceneKeyInterpolation InKeyInterpolation, float TimeToCopyFrom = FLT_MAX) override;
	virtual TOptional<FKeyHandle> DuplicateKey(FKeyHandle KeyToDuplicate) override;
	virtual FRichCurve* GetRichCurve() override;
	virtual UMovieSceneSection* GetOwningSection() override;
	virtual bool CanCreateKeyEditor() override;
	virtual TSharedRef<SWidget> CreateKeyEditor(ISequencer* Sequencer) override;
	virtual void SetName(FName Name) override {}
	virtual FName GetName() const override { return NAME_None; }
	virtual void CopyKeys(FMovieSceneClipboardBuilder& ClipboardBuilder, const TFunctionRef<bool(FKeyHandle, const IKeyArea&)>& KeyMask) const override;
	virtual void PasteKeys(const FMovieSceneClipboardKeyTrack& KeyTrack, const FMovieSceneClipboardEnvironment& SrcEnvironment, const FSequencerPasteEnvironment& DstEnvironment) override;
	virtual TOptional<FLinearColor> GetColor() override;

protected:
	
	/** Pointer to the section to which this key area relates */
	UMovieSceneSection* Section;
};
