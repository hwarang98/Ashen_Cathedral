// Copyright 2024, PrismaticaDev. All rights reserved.


#include "Types/PrismatiscapeProfileBase.h"

#if WITH_EDITOR
void UPrismatiscapeProfileBase::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	OnPropertyChange(PropertyChangedEvent.GetPropertyName());
}
#endif